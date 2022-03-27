/**
 * @brief the server
 *
*/
#define _POSIX_C_SOURCE 200112L
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <unistd.h>

#include <bounded_buffer.h>
#include <parser.h>
#include <defines.h>
#include <cache.h>
#include <utilities.h>
#include <error_handlers.h>
#include <worker.h>

#define CONN_MAX 10
#define TASKS_MAX 4096

volatile sig_atomic_t terminate = 0; // toggled on when server should terminate as soon as possible
volatile sig_atomic_t refuse_new = 0; // toggled on when server must not accept any other client

/**
 * @brief used to handle signals.
 * @param arg to be cast to sigset_t
*/
static void* handle_signal(void* sigs);

int main(int argc, char* argv[]){
   if (argc != 2){
      fprintf(stdout, "Please input a valid config file.\n");
      return 1;
   }
   log_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
   int err;
   fd_set master_read;
   fd_set read_cpy;
   int fd_num = 0; // number of fds in set
   int fd_socket = -1;
   int fd_client;
   //worker-to-manager pipe to pass back fd's ready to be selected again
   int fd_pipe[2];
   bool signal_handler_tgl = false;
   bool pipe_tgl = false;
   char pipe_buf[PIPE_LEN_MAX];
   int pipe_msg;
   char new_task[TASK_LEN_MAX];
   bounded_buffer_t* tasks = NULL;
   char* sockname = NULL;
   struct sockaddr_un saddr;
   parser_t* config = NULL;
   cache_t* cache = NULL;
   pthread_t signal_handler_id;
   struct sigaction sig_action;
   sigset_t sigset;
   pthread_t* workers = NULL; // worker threads pool
   worker_t* worker = NULL;
   unsigned long pool_size; //worker pool
   //specifies the interval in microseconds that select() should block waiting for a fd to become ready
   struct timeval timeout_master = { 0, 100000 };
   struct timeval timeout_cpy;
   char* log_name = NULL;
   FILE* log_file = NULL;
   size_t online = 0; // clients online now
   size_t i = 0;

   //signal handling
   memset(&sig_action, 0, sizeof(sig_action));

   sigemptyset(&sigset);
   sigaddset(&sigset, SIGINT);
   sigaddset(&sigset, SIGHUP);
   sigaddset(&sigset, SIGQUIT);
   sig_action.sa_mask = sigset;

   // SIGPIPE is ignored
   sig_action.sa_handler = SIG_IGN;
   CHECK_NZ_EXIT(err, sigaction(SIGPIPE, &sig_action, NULL), sigaction);
   //  nominate one thread to manage the rest of the signals
   CHECK_NZ_EXIT(err, pthread_sigmask(SIG_BLOCK, &sigset, NULL), pthread_sigmask);
   CHECK_NZ_EXIT(err, pthread_create(&signal_handler_id, NULL, &handle_signal, (void*) &sigset), pthread_create);
   signal_handler_tgl = true;

   //creating a parser for the file config
   config = parser_create();
   if (!config){
      perror("parser_create");
      goto failure;
   }
   //parsing the file config
   err = parser_update(config, argv[1]);
   if (err == -1){
      perror("parser_update");
      goto failure;
   }

   // getting the socket file path from the config file
   err = parser_get_sock_path(config, &sockname);
   if (err == 0){
      perror("parser_get_sock_path");
      goto failure;
   }
   //connecting to an AF_UNIX stream socket
   strncpy(saddr.sun_path, sockname, PATH_LEN_MAX);
   saddr.sun_family = AF_UNIX;
   fd_socket = socket(AF_UNIX, SOCK_STREAM, 0);
   if (fd_socket == -1){
      perror("socket");
      goto failure;
   }
   // bind the address to the socket
   err = bind(fd_socket, (struct sockaddr*)&saddr, sizeof saddr);
   if (err == -1){
      perror("bind");
      goto failure;
   }
   //prepare to accept at most CONN_MAX connections
   err = listen(fd_socket, CONN_MAX);
   if (err == -1){
      perror("listen");
      goto failure;
   }

   // creating the cache with the details read from the config file
   cache = cache_create((size_t) parser_get_files(config), (size_t) parser_get_size(config),
                        parser_get_policy(config));
   if (!cache){
      perror("cache_create");
      goto failure;
   }

   // creating the buffer holding the tasks
   tasks = buffer_create(TASKS_MAX);
   if (!tasks){
      perror("buffer_create");
      goto failure;
   }

   // creating a pipe communication channel
   err = pipe(fd_pipe);
   if (err == -1){
      perror("pipe");
      goto failure;
   }
   pipe_tgl = true;

   // saving the log file path parsed from the config file
   err = (int) parser_get_log_path(config, &log_name);
   if (err == 0){
      perror("parser_get_log_path");
      goto failure;
   }
   //open a log file for reading and writing
   log_file = fopen(log_name, "w+");
   if (!log_file){
      perror("fopen");
      goto failure;
   }

   // creating a worker
   worker = worker_create(cache,tasks,fd_pipe[1],log_file);
   if (!worker){
      perror("malloc");
      goto failure;
   }

   pool_size = parser_get_workers(config);
   workers = malloc(sizeof(pthread_t) * pool_size);
   if (!workers){
      perror("malloc");
      goto failure;
   }
   // creating a pool of worker threads
   for (i = 0; i < (size_t) pool_size; i++){
      err = pthread_create(&(workers[i]), NULL, &do_job, (void*) worker);
      if (err != 0){
         perror("pthread_create");
         goto failure;
      }
   }

   // intialize fd set for monitoring
   fd_num = MAX(fd_num, fd_socket);
   FD_ZERO(&master_read);
   FD_SET(fd_socket, &master_read);
   FD_SET(fd_pipe[0], &master_read);

   while (true){
      //hard exit
      if (terminate) goto cleanup;
      //soft exit
      if (online == 0 && refuse_new) goto cleanup;

      // reinitialise the read set
      read_cpy = master_read;
      timeout_cpy = timeout_master;
      // wait for a fd to be ready for read operation
      if ((select(fd_num + 1, &read_cpy, NULL, NULL, &timeout_cpy)) == -1){
         if (errno == EINTR){
            //soft exit
            if (online == 0 && refuse_new) break;
            //hard exit
            else if (terminate) goto cleanup;
            else continue;
         }else{
            perror("select");
            exit(EXIT_FAILURE);
         }
      }

      // find the file descriptor that is ready
      for (i = 0; i < fd_num + 1; i++){
         //i is part of the set
         if (FD_ISSET(i, &read_cpy)){
            // if it has been put on the pipe, the worker has finished with a task
            if (i == fd_pipe[0]){
               CHECK_FAIL_EXIT(err, readn((long) i, (void*) pipe_buf, PIPE_LEN_MAX), readn);
               CHECK_NEQ_EXIT(err, 1, sscanf(pipe_buf, "%d", &pipe_msg), sscanf);
               if (pipe_msg == 0){
                  //the client left
                  if (online) online--;
                  if (online == 0 && refuse_new) break;
               }else{
                  //no one has left
                  FD_SET(pipe_msg, &master_read);
                  fd_num = MAX(pipe_msg, fd_num);
               }
            // new client not yet part of the set
            }else if (i == fd_socket){
               CHECK_FAIL_EXIT(fd_client, accept(fd_socket, NULL, 0), accept);
               //soft exit, close
               if (refuse_new) {
                  close(fd_client);
               }else{
                  LOG_EVENT("New client accepted : %d.\n", fd_client);
                  FD_SET(fd_client, &master_read);
                  online++;
                  LOG_EVENT("Clients online now : %lu.\n", online);
                  fd_num = MAX(fd_client, fd_num);
               }
            //new task from a client already part of the set
            }else {
               memset(new_task, 0, TASK_LEN_MAX);
               snprintf(new_task, TASK_LEN_MAX, "%lu", i);
               FD_CLR(i, &master_read);
               if (i == fd_num) fd_num--;
               // push ready file descriptor to task queue for workers
               CHECK_FAIL_EXIT(err, buffer_enqueue(tasks, new_task), buffer_enqueue);
            }
         }
      }
   }

   return 0;


   cleanup:
   //notify workers in pool of termination
   snprintf(new_task, TASK_LEN_MAX, "%d", SHUTDOWN_WORKER);
   for (size_t j = 0; j < (size_t) pool_size; j++)
      CHECK_NZ_EXIT(err, buffer_enqueue(tasks, new_task), buffer_enqueue);
   //wait until every worker in pool has died
   for (size_t j = 0; j < (size_t) pool_size; j++)
      pthread_join(workers[j], NULL);
   //wait until the thread handling the signals dies
   pthread_join(signal_handler_id, NULL);
   //write results to log file
   LOG_EVENT("Max size reached by the file storage cache : %3f.\n", cache_get_size_max(cache) * MBYTE);
   LOG_EVENT("Max number of files stored inside the server : %lu.\n", cache_get_files_max(cache));
   //print the contents of the cache
   cache_print(cache);
   //free allocated resources and close
   cache_free(cache);
   parser_free(config);
   buffer_free(tasks);
   if (sockname) {
      unlink(sockname);
      free(sockname); }
   free(log_name);
   free(worker);
   free(workers);
   if (pipe_tgl) {
      close(fd_pipe[0]);
      close(fd_pipe[1]);
   }
   if (log_file) fclose(log_file);
   if (fd_socket != -1) close(fd_socket);
   return 0;

   failure:
   if (workers){
      for (size_t j = 0;j != i;i++){
         pthread_kill(workers[j], SIGKILL);
      }
   }
   free(workers);
   if (signal_handler_tgl) pthread_kill(signal_handler_id, SIGKILL);
   parser_free(config);
   cache_free(cache);
   buffer_free(tasks);
   if (sockname) { 
      unlink(sockname); 
      free(sockname); 
   }
   if (fd_socket != -1) close(fd_socket);
   if (log_file) fclose(log_file);
   if (pipe_tgl) { 
      close(fd_pipe[0]); 
      close(fd_pipe[1]); 
   }
   free(log_name);
   free(worker);
   exit(EXIT_FAILURE);
}


static void* handle_signal(void* sigs){
   //a set of signals to be blocked, unblocked, or waited for
   sigset_t* set = (sigset_t*) sigs;
   int err;
   int signal;
   while (true){
      CHECK_NZ_EXIT(err, sigwait(set, &signal), sigwait);
      switch (signal){
         // immediate shutdown
         case SIGINT:
         case SIGQUIT:
            terminate = 1;
            return NULL;
         // blocking new connections and shutting down after all clients
         //ha closed the connection
         case SIGHUP:
            refuse_new = 1;
            return NULL;
         default:
            break;
      }
   }
}
