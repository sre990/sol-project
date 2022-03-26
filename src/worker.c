/**
 * @brief the worker handling a task
 *
*/
#define _POSIX_C_SOURCE 200112L
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <bounded_buffer.h>
#include <utilities.h>
#include <error_handlers.h>
#include <worker.h>
#include <cache.h>

/**
 * @brief notifies via pipe of the completion of a task.
*/
#define NOTIFY_DONE \
do{ \
	memset(pipe_buf, 0, PIPE_LEN_MAX); \
	snprintf(pipe_buf, PIPE_LEN_MAX, "%d", fd_ready); \
	CHECK_FAIL_EXIT(err, writen((long) pipe, (void*) pipe_buf, PIPE_LEN_MAX), writen); \
	break; \
}while(0);


struct _worker{
   cache_t* cache;
   bounded_buffer_t* tasks;
   int pipe;
   FILE* log_file;
};

worker_t *worker_create(cache_t *cache, bounded_buffer_t *tasks, int pipe, FILE *file){
   if(!cache || !tasks || !file) {
      errno = EINVAL;
      return NULL;
   }
   worker_t* worker = NULL;
   worker = malloc(sizeof(worker_t));
   if (!worker){
      errno = ENOMEM;
      return NULL;
   }
   worker->cache = cache;
   worker->tasks = tasks;
   worker->pipe = pipe;
   worker->log_file = file;

   return worker;
}

void* do_job(void* wkr){
   //setting up declarations for processing tasks
   char* req;
   CHECK_NULL_EXIT(req, malloc(sizeof(char) * REQ_LEN_MAX), malloc);
   char* new_req;
   ops_t req_op;
   worker_t* worker = (worker_t*) wkr;
   bounded_buffer_t* tasks = worker->tasks;
   cache_t* cache = worker->cache;
   FILE* log_file = worker->log_file;
   int pipe = worker->pipe;
   int err;
   int new_err;
   int errno_cpy;
   //the file descriptor of the client
   int fd_ready;
   char* fd_ready_string;
   char* token = NULL;
   char* save_ptr = NULL;
   char file_path[REQ_LEN_MAX];
   char pipe_buf[PIPE_LEN_MAX];
   char msg_size[SIZE_LEN];

   //setting up declarations for handling the cache
   linked_list_t* evicted = NULL;
   char* evicted_name = NULL;
   char* evicted_content = NULL;
   size_t evicted_size = 0;
   void* read_buf;
   size_t read_size;
   int flags = 0;
   size_t N = 0;
   linked_list_t* read_files = NULL;
   char* read_file_name = NULL;
   char* read_file_content = NULL;
   size_t read_file_size = 0;
   size_t tot_read_size = 0;
   void* append_buf = NULL;
   size_t append_size = 0;
   size_t write_size = 0;
   char* write_contents = NULL;

   //enters an infinite loop and processes tasks received via buffer, one at a time
   while(true){
      fd_ready_string = NULL;
      // get ready fd from task buffer
      CHECK_NZ_EXIT(err, buffer_dequeue(tasks, &fd_ready_string), buffer_dequeue);
      CHECK_NEQ_EXIT(err, 1, sscanf(fd_ready_string, "%d", &fd_ready), sscanf);
      if (fd_ready == SHUTDOWN_WORKER) {
         free(fd_ready_string);
         break;
      }
      memset(req, 0, TASK_LEN_MAX);
      //trying to read a request
      CHECK_FAIL_EXIT(err, readn((long) fd_ready, (void*) req, REQ_LEN_MAX), readn);
      new_req = req;
      // get a token string from the request and saves it to save_ptr
      token = strtok_r(new_req, " ", &save_ptr);
      if (!token) {
         free(fd_ready_string);
	      continue;
      }
      //getting the operation requested
      CHECK_NEQ_EXIT(err, 1, sscanf(token, "%d", (int*) &req_op), sscanf);
      switch (req_op){
         case OPEN:
            //reading the file path
            memset(file_path, 0, REQ_LEN_MAX);
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%s", file_path), sscanf);
            //reading the flag part of the request
            flags = 0;
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%d", &flags), sscanf);
            //opening the file located at <file_path> with flags as per
            //client's request
            err = cache_openFile(cache, file_path, flags, fd_ready);
            errno_cpy = errno;
            //sending the return value of the operation to the
            //client's fd and logging the operation
            memset(req, 0, REQ_LEN_MAX);
            snprintf(req, REQ_LEN_MAX, "%d", err);
            LOG_EVENT("[%d] openFile %s %d : %d.\n", (int) pthread_self(), file_path, flags, err);
            CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req, strlen(req) + 1), writen);
            //handling the outcome of the operation
            if (err==OP_FAILURE) {
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) req, ERRNO_LEN_MAX), writen);
            }else if(err == OP_EXIT_FATAL) {
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) req, ERRNO_LEN_MAX), writen);
               exit(1);
            }
            //else err==OP_SUCCESS
            NOTIFY_DONE;
            break;
         case READ:
            read_buf = NULL;
            read_size = 0;
            //reading the file_path
            memset(file_path, 0, REQ_LEN_MAX);
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%s", file_path), sscanf);
            //reading of the flag specifying if the file read should be saved
            flags = 0;
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%d", &flags), sscanf);
            if (flags == SAVE){
               //reading the file located at <file_path> as per
               //client's request and saving it to read_buf
               err = cache_readFile(cache, file_path, &read_buf, &read_size, fd_ready);
               errno_cpy = errno;
               //sending the return value of the operation to the
               //client's fd and logging the operation
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", err);
               LOG_EVENT("[%d] readFile %s : %d -> %lu.\n", (int) pthread_self(), file_path, err, read_size);
               CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req, strlen(req) + 1), writen);
               if (err==OP_FAILURE) {
                  memset(req, 0, REQ_LEN_MAX);
                  snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
                  CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void *) req, ERRNO_LEN_MAX), writen);
               }else if(err==OP_EXIT_FATAL){
                  memset(req, 0, REQ_LEN_MAX);
                  snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
                  CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) req, ERRNO_LEN_MAX), writen);
                  exit(1);
               }//else err==OP_SUCCESS
               //sending the size of the read file to be saved
               memset(msg_size, 0, SIZE_LEN);
               snprintf(msg_size, SIZE_LEN, "%lu", read_size);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) msg_size, SIZE_LEN), writen);
               if (read_size != 0) {
                  //sending the file contents of the file to be saved
                  CHECK_FAIL_EXIT(err, writen((long) fd_ready, read_buf, read_size), writen);
               }
               //deallocating resources for the file just read and saved
               free(read_buf);
               read_buf = NULL;
            }else{
               //else flags==DISCARD, the file read will be discarded
               //reading the file located at <file_path> as per
               //client's request without saving it
               err = cache_readFile(cache, file_path, NULL, NULL, fd_ready);
               errno_cpy = errno;
               //sending the return value of the operation to the
               //client's fd and logging the operation
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", err);
               LOG_EVENT("[%d] readFile %s NULL: %d -> %lu.\n", (int) pthread_self(), file_path, err, read_size);
               CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req, strlen(req) + 1), writen);
               if(err==OP_FAILURE) {
                  memset(req, 0, REQ_LEN_MAX);
                  snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
                  CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void *) req, ERRNO_LEN_MAX), writen);
               }else if(err==OP_EXIT_FATAL){
                     memset(req, 0, REQ_LEN_MAX);
                     snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
                     CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) req, ERRNO_LEN_MAX), writen);
                     exit(1);
               }
               //else err==OP_SUCCESS
            }
            //reset flags
            flags = 0;
            NOTIFY_DONE;
            break;
         case READ_N:
            read_files = NULL;
            N = 0;
            //reading the N part of the request, corresponding to the number of files to be read
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%lu", &N), sscanf);
            //reading N files as per client's request
            err = cache_readNFiles(cache, &read_files, N, fd_ready);
            errno_cpy = errno;
            //sending the return value of the operation to the
            //client's fd
            memset(req, 0, REQ_LEN_MAX);
            snprintf(req, REQ_LEN_MAX, "%d", err);
            CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req, strlen(req) + 1), writen);
            if (err==OP_FAILURE) {
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void *) req, ERRNO_LEN_MAX), writen);
            }else if(err==OP_EXIT_FATAL){
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) req, ERRNO_LEN_MAX), writen);
               //exit with 1 only after all files to be read are handled
            }//else err==OP_SUCCESS
            //sending the number of files read
            memset(msg_size, 0, SIZE_LEN);
            snprintf(msg_size, SIZE_LEN, "%lu", list_get_size(read_files));
            CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) msg_size, SIZE_LEN), writen);
            //sending the actual files read
            while (list_get_size(read_files) != 0){
               errno = 0;
               //getting the first read file from the list and saving its name and contents
               read_file_size = list_pop_from_front(read_files, &read_file_name, (void**) &read_file_content);
               if (read_file_size == 0 && errno == ENOMEM) exit(1);
               tot_read_size += read_file_size;
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%s", read_file_name);
               //sending the return value of the operation to the
               //client's fd and logging the operation
               CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req, REQ_LEN_MAX), writen);
               memset(msg_size, 0, SIZE_LEN);
               snprintf(msg_size, SIZE_LEN, "%lu", read_file_size);
               CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) msg_size, SIZE_LEN), writen);
               CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) read_file_content, read_file_size), writen);
               //deallocating resources for the file just read
               free(read_file_name);
               read_file_name = NULL;
               free(read_file_content);
               read_file_content = NULL;
            }//log event
            LOG_EVENT("[%d] readNFiles %lu : %d -> %lu.\n", (int) pthread_self(), N, err, tot_read_size);
            //all read files are handled, deallocate resources
            list_free(read_files);
            read_files = NULL;
            //read files were handled, if a fatal error has occurred exit with 1
            if (err == OP_EXIT_FATAL) exit(1);
            NOTIFY_DONE;
            break;
         case WRITE:
            evicted = NULL;
            evicted_name = NULL;
            evicted_content = NULL;
            write_contents = NULL;
            //reading the file_path
            memset(file_path, 0, REQ_LEN_MAX);
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%s", file_path), sscanf);
            //reading the file size
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%lu", &write_size), sscanf);
            //allocate resources for the file to be written
            if (write_size != 0){
               CHECK_NULL_EXIT(write_contents, (char*) malloc(write_size + 1), malloc);
               memset(write_contents, 0, write_size + 1);
               CHECK_FAIL_EXIT(err, readn((long) fd_ready, (void*) write_contents, write_size), readn);
            }
            //writing the file located at <file_path> as per
            //client's request
            err = cache_writeFile(cache, file_path, write_size, write_contents, &evicted, fd_ready);
            errno_cpy = errno;
            free(write_contents);
            //sending the return value of the operation to the
            //client's fd and logging the operation
            memset(req, 0, REQ_LEN_MAX);
            snprintf(req, REQ_LEN_MAX, "%d", err);
            LOG_EVENT("[%d] writeFile %s : %d -> %lu.\n\tVictims : %lu.\n", (int) pthread_self(), file_path, err,
                      write_size, list_get_size(evicted));
            CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req, strlen(req) + 1), writen);
            if (err==OP_FAILURE) {
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void *) req, ERRNO_LEN_MAX), writen);
            }else if(err==OP_EXIT_FATAL){
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) req, ERRNO_LEN_MAX), writen);
               //exit with 1 only after capacity misses are handled
            }//else err==OP_SUCCESS
            //sending the number of files evicted because of capacity misses
            memset(msg_size, 0, SIZE_LEN);
            snprintf(msg_size, SIZE_LEN, "%lu", list_get_size(evicted));
            CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) msg_size, SIZE_LEN), writen);
            //sending the files evicted after capacity misses
            while(list_get_size(evicted) != 0){
               errno = 0;
               //getting the first evicted file from the list and saving its name and contents
               evicted_size = list_pop_from_front(evicted, &evicted_name, (void**) &evicted_content);
               if (evicted_size == 0 && errno == ENOMEM) exit(1);
               //sending the return value of the operation to the
               //client's fd and logging the operation
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%s", evicted_name);
               CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req, REQ_LEN_MAX), writen);
               LOG_EVENT("\tVictim name: %s.\n", evicted_name);
               //sending the evicted file size and contents to the client's fd
               memset(msg_size, 0, SIZE_LEN);
               snprintf(msg_size, SIZE_LEN, "%lu", evicted_size);
               CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) msg_size, SIZE_LEN), writen);
               CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) evicted_content, evicted_size), writen);
               //deallocating resources for the evicted file
               free(evicted_name);
               evicted_name = NULL;
               free(evicted_content);
               evicted_content = NULL;
            }//all evicted files are handled, deallocate resources
            list_free(evicted);
            evicted = NULL;
            //evicted files were handled, if a fatal error has occurred exit with 1
            if (err == OP_EXIT_FATAL) exit(1);
            NOTIFY_DONE;
            break;
         case APPEND:
            evicted = NULL;
            append_buf = NULL;
            append_size = 0;
            //reading the file path
            memset(file_path, 0, REQ_LEN_MAX);
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%s", file_path), sscanf);
            //reading the size of the buffer to write contents to and allocate
            //memory for it
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%lu", &append_size), sscanf);
            if (append_size != 0){
               CHECK_NULL_EXIT(append_buf, malloc(append_size + 1), malloc);
               memset(append_buf, 0, append_size + 1);
               CHECK_FAIL_EXIT(err, readn((long) fd_ready, append_buf, append_size), readn);
            }
            //appending the file located at <file_path> the contents of the buffer as per
            //client's request
            err = cache_appendToFile(cache, file_path, append_buf, append_size, &evicted, fd_ready);
            errno_cpy = errno;
            //deallocating resources for the buffer
            free(append_buf);
            //sending the return value of the operation to the
            //client's fd and logging the operation
            memset(req, 0, REQ_LEN_MAX);
            snprintf(req, REQ_LEN_MAX, "%d", err);
            LOG_EVENT("[%d] appendToFile %s : %d -> %lu.\n\tVictims : %lu.\n", (int) pthread_self(), file_path,
                      err, append_size, list_get_size(evicted));
            CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req, strlen(req) + 1), writen);
            if (err==OP_FAILURE) {
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void *) req, ERRNO_LEN_MAX), writen);
            }else if(err==OP_EXIT_FATAL){
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) req, ERRNO_LEN_MAX), writen);
               //exit with 1 only after capacity misses are handled
            }//else err==OP_SUCCESS
            //sending the number of files evicted because of capacity misses
            memset(msg_size, 0, SIZE_LEN);
            snprintf(msg_size, SIZE_LEN, "%lu", list_get_size(evicted));
            CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) msg_size, SIZE_LEN), writen);
            //sending the files evicted after capacity misses
            while (list_get_size(evicted) != 0){
               errno = 0;
               //getting the first evicted file from the list and saving its name and contents
               evicted_size = list_pop_from_front(evicted, &evicted_name, (void**) &evicted_content);
               if (evicted_size == 0 && errno == ENOMEM) exit(1);
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%s", evicted_name);
               //sending the return value of the operation to the
               //client's fd and logging the operation
               CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req, REQ_LEN_MAX), writen);
               LOG_EVENT("\tVictim name: %s.\n", evicted_name);
               memset(msg_size, 0, SIZE_LEN);
               snprintf(msg_size, SIZE_LEN, "%lu", evicted_size);
               CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) msg_size, SIZE_LEN), writen);
               CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) evicted_content, evicted_size), writen);
               //deallocating resources for the evicted file
               free(evicted_name);
               evicted_name = NULL;
               free(evicted_content);
               evicted_content = NULL;
            }//all evicted files are handled, deallocate resources
            list_free(evicted);
            evicted = NULL;
            //evicted files were handled, if a fatal error has occurred exit with 1
            if (err == OP_EXIT_FATAL) exit(1);
            NOTIFY_DONE;
            break;
         case CLOSE:
            //reading the file path
            memset(file_path, 0, REQ_LEN_MAX);
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%s", file_path), sscanf);
            //closing the file located at <file_path> as per
            //client's request
            err = cache_closeFile(cache, file_path, fd_ready);
            errno_cpy = errno;
            //sending the return value of the operation to the
            //client's fd and logging the operation
            memset(req, 0, REQ_LEN_MAX);
            snprintf(req, REQ_LEN_MAX, "%d", err);
            LOG_EVENT("[%d] closeFile %s : %d.\n", (int) pthread_self(), file_path, err);
            CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req, strlen(req) + 1), writen);
            if (err==OP_FAILURE) {
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void *) req, ERRNO_LEN_MAX), writen);
            }else if(err==OP_EXIT_FATAL){
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) req, ERRNO_LEN_MAX), writen);
               exit(1);
            }
            //else err==OP_SUCCESS
            NOTIFY_DONE;
            break;
         case LOCK:
            //reading the file path
            memset(file_path, 0, REQ_LEN_MAX);
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%s", file_path), sscanf);
            //locking the file located at <file_path> as per
            //client's request
            err = cache_lockFile(cache, file_path, fd_ready);
            errno_cpy = errno;
            //sending the return value of the operation to the
            //client's fd and logging the operation
            memset(req, 0, REQ_LEN_MAX);
            snprintf(req, REQ_LEN_MAX, "%d", err);
            LOG_EVENT("[%d] lockFile %s %d : %d.\n", (int) pthread_self(), file_path, flags, err);
            CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req,strlen(req) + 1), writen);
            //handling the outcome of the operation
            if(err==OP_FAILURE) {
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void *) req,
                                           ERRNO_LEN_MAX), writen);
            }else if(err==OP_EXIT_FATAL){
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) req, ERRNO_LEN_MAX), writen);
               exit(1);
            }//else err==OP_SUCCESS
            NOTIFY_DONE;
            break;
         case UNLOCK:
            //reading the file path
            memset(file_path, 0, REQ_LEN_MAX);
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%s", file_path), sscanf);
            //unlocking the file located at <file_path> as per
            //client's request
            err = cache_unlockFile(cache, file_path, fd_ready);
            errno_cpy = errno;
            //sending the return value of the operation to the
            //client's fd and logging the operation
            memset(req, 0, REQ_LEN_MAX);
            snprintf(req, REQ_LEN_MAX, "%d", err);
            LOG_EVENT("[%d] unlockFile %s %d : %d.\n", (int) pthread_self(), file_path, flags, err);
            CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req,strlen(req) + 1), writen);
            //handling the outcome of the operation
            if (err==OP_FAILURE) {
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void *) req,
                                           ERRNO_LEN_MAX), writen);
            }else if(err==OP_EXIT_FATAL){
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) req,ERRNO_LEN_MAX), writen);
               exit(1);
            }//else err==OP_SUCCESS
            NOTIFY_DONE;
            break;
         case REMOVE:
            //reading the file path
            memset(file_path, 0, REQ_LEN_MAX);
            CHECK_NULL_EXIT(token, strtok_r(NULL, " ", &save_ptr), strtok_r);
            CHECK_NEQ_EXIT(err, 1, sscanf(token, "%s", file_path), sscanf);
            err = cache_removeFile(cache, file_path, fd_ready);
            errno_cpy = errno;
            //removing the file located at <file_path> as per
            //client's request and logging the operation
            memset(req, 0, REQ_LEN_MAX);
            snprintf(req, REQ_LEN_MAX, "%d", err);
            LOG_EVENT("[%d] removeFile %s : %d.\n", (int) pthread_self(), file_path, err);
            CHECK_FAIL_EXIT(new_err, writen((long) fd_ready, (void*) req,strlen(req) + 1), writen);
            //handling the outcome of the operation
            if(err==OP_FAILURE) {
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void *) req,
                                           ERRNO_LEN_MAX), writen);
            }else if(err==OP_EXIT_FATAL){
               memset(req, 0, REQ_LEN_MAX);
               snprintf(req, REQ_LEN_MAX, "%d", errno_cpy);
               CHECK_FAIL_EXIT(err, writen((long) fd_ready, (void*) req,ERRNO_LEN_MAX), writen);
               exit(1);
            }//else err==OP_SUCCESS
            NOTIFY_DONE;
            break;
         case SHUTDOWN:
            //sending the shutdown message via a buffer and logging the operation
            memset(pipe_buf, 0, PIPE_LEN_MAX);
            snprintf(pipe_buf, PIPE_LEN_MAX, "%d", SHUTDOWN_WORKER);
            CHECK_FAIL_EXIT(err, writen((long) pipe, (void*) pipe_buf, PIPE_LEN_MAX), writen);
            LOG_EVENT("Client left %d.\n", fd_ready);
            break;
      }
      free(fd_ready_string);
   }
   free(req);
   return NULL;
}
