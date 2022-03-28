/**
 * @brief the client using the file storage cache
 *
*/

#define _DEFAULT_SOURCE

#include <dirent.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <linked_list.h>
#include <defines.h>
#include <api.h>
#include <utilities.h>
#include <error_handlers.h>

#define RETRY_AFTER 1000
#define GIVEUP_AFTER 10

/**
 * @brief checks if the input string has is all dots. In UNIX,
 * single dot represents the directory you are in and a double dot
 * represents the parent directory.
 * @return 0 if true, -1 if false
 * @note taken from http://didawiki.di.unipi.it/doku.php/informatica/sol/laboratorio21/
*/
static int isdot(const char dir[]);

/**
 * @brief mimics the ls -R command in UNIX, lists the contents of the current directory and its subdirectories
 * saving the files to a doubly linked list
 * @returns 0 on success, -1 on failure.
 * @param dir_path must be != NULL.
 * @param files must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
 * @note taken from http://didawiki.di.unipi.it/doku.php/informatica/sol/laboratorio21/
*/
static int lsR(const char dir_path[], linked_list_t* list);

/**
 * @brief parses the commands and its arguments
 * @return 0 on success, -1 on failure
 * @param cmds must be != NULL.
 * @param opts must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
static int parse_cmdline(const char** cmds, const char** opts, int len);

/**
 * @brief frees resources allocated for the parser.
*/
void client_free();


bool freed = false;
bool connected = false;
char** cmds = NULL;
char** opts = NULL;
//list of files to write to server with argument -w
linked_list_t* written_files = NULL;
char* read_file = NULL;
int len = 0;
bool helper_tgl = false;
char socket_name[PATH_LEN_MAX];
char* file_name = NULL;


int main(int argc, char* argv[]){
       if(atexit(client_free) == -1){
        perror("client_free");
        return 1;
    }
	if (argc == 1){
		fprintf(stderr, "No argument, exiting.\n");
		fprintf(stdout, H_USAGE);
		return 1;
	}

	len = argc - 1;
	char opt;
   //openConnection related
   time_t now = time(0);
   int msec = RETRY_AFTER;
   struct timespec abstime = { .tv_nsec = 0, .tv_sec = now + GIVEUP_AFTER };
   // time in milliseconds between two consecutive requests
	int sleep_in_msec = 0;
	char* new;
	char* token;
	char* save_ptr;
   //openFile related
	int flags = 0;
   //related to reading operations
	size_t read_size = 0;
   //readNFiles related
   int N = 0;
	char* cwd = NULL;
	char file_path[PATH_MAX];
	int err;
	int i = 0;

   //makings space for commands and options read from command line
	cmds = malloc(sizeof(char*) * (argc - 1));
	if (!cmds){
		perror("malloc");
		goto failure;
	}
	for (i = 0; i < argc - 1; i++){
		cmds[i] = malloc(sizeof(char) * CMD_LEN_MAX);
		if (!cmds[i]){
			perror("malloc");
			goto failure;
		}
		memset(cmds[i], 0, CMD_LEN_MAX);
	}
	opts = malloc(sizeof(char*) * (argc - 1));
	if (!opts){
		perror("malloc");
		goto failure;
	}
	for (i = 0; i < argc - 1; i++){
		opts[i] = malloc(sizeof(char) * ARG_LEN_MAX);
		if (!opts[i]){
			perror("malloc");
			goto failure;
		}
		memset(opts[i], 0, ARG_LEN_MAX);
	}

   //getting the commands
	for (int i = 1; i < argc; i++){
		if (argv[i][0] == '-'){
			int len = strlen(argv[i]);
			int j = 0;
			for (;j < len;j++){
				if (argv[i][j] == '-'){
					continue;
				}else{
               //checking if the option is valid
					if (j == len-1 && CHECK_OPT(argv[i][j]))
						snprintf(cmds[i-1], CMD_LEN_MAX, "%s", argv[i] + j);
					break;
				}
			}
		}
	}
   //there is no command
	if (cmds[0][0] == '\0') return 0;
	// getting the options from argv
	for (int i = 1; i < argc; i++){
		if (cmds[i-1][0] != '\0') continue;
		snprintf(opts[i-2], ARG_LEN_MAX, "%s", argv[i]);
	}
	verbose_mode = false;

   //validate commands and its options
	err = parse_cmdline((const char**) cmds, (const char**) opts, argc - 1);
	if (err != 0){
		fprintf(stderr, "[file %s,@line %d] Error! Invalid commands. \n", __FILE__, __LINE__);
		return 0;
	}

	// print to stdout a helper message with a list of options
	if (helper_tgl){
		fprintf(stdout, H_USAGE);
		return 0;
	}
   // loop executing all commands
	for (int i = 0; i < argc - 1; i++){
		if (cmds[i][0] == '\0') continue;
		else opt = cmds[i][0];
		switch (opt){
         //specifies the socket name
			case 'f':
				if (openConnection(socket_name, msec, abstime) == 0)
					connected = true;
				break;
         //specifies the directory of the files to be written
         case 'w':
				new = opts[i];
				written_files = list_create(NULL);
				if (!written_files){
					perror("list_create");
					return 0;
				}
            cwd = malloc(NAME_LEN_MAX * sizeof(char));
            if (!cwd){
               perror("malloc");
               return 0;
            }
	         getcwd(cwd, NAME_LEN_MAX);
				//handling the number of files from cwd to be written
            //specified after the comma as a number
				if (strrchr(new, ',')) {
               token = strtok_r(new, ",", &save_ptr);
               errno = 0;
               //get a list of files to be written using ls -R
               if (lsR(token, written_files) == -1){
                  if (errno == ENOMEM){
                     perror("lsR");
                     return 0;
                  }
                  else break;
               }
               //change to cwd
               if (chdir(cwd) == -1){
                  perror("chdir");
                  return 0;
               }
               free(cwd);
               cwd = NULL;
               //retrieve N fiels to be written
               token = strtok_r(NULL, ",", &save_ptr);
               if (sscanf(token, "%d", &N) != 1){
                  perror("sscanf");
                  return 0;
               }
               for (i=0;i<N;i++){
                  if (list_get_size(written_files) == 0) break;
                  errno = 0;
                  if (list_pop_from_front(written_files, &file_name, NULL) == 0 && errno == ENOMEM){
                     perror("list_pop_from_front");
                     return 0;
                  }
                  //open a new file, lock it and write is contents to server
                  SET_MASK(flags, O_CREATE);
                  SET_MASK(flags, O_LOCK);
                  openFile(file_name, flags);
                  RESET_MASK(flags);
                  //if the option -D has been specified,
                  //save the evicted files in that directory
                  if (i + 2 < argc - 1){
                     if (cmds[i+2][0] == 'D')
                        writeFile(file_name, opts[i+2]);
                  }else writeFile(file_name, NULL);
                  //writing successful, unlock and close file
                  unlockFile(file_name);
                  closeFile(file_name);
                  //sleep some milliseconds before another request
                  usleep(1000 * sleep_in_msec);
                  free(file_name);
                  file_name = NULL;
               }
               list_free(written_files); 
               written_files = NULL;
               break;
				}else{
               //no limit of files to be written has been set,
               //write them all

               //get all files in the directory using ls -R
               if (lsR(opts[i], written_files) == -1){
                  if (errno == ENOMEM){
                     perror("lsR");
                     return 0;
                  }
                  else break;
               }
               //change to cwd
               if (chdir(cwd) == -1){
                  perror("chdir");
                  return 0;
               }
               free(cwd);
               cwd = NULL;
               //write all files to server
               while (list_get_size(written_files) != 0){
                  errno = 0;
                  if (list_pop_from_front(written_files, &file_name, NULL) == 0 && errno == ENOMEM){
                     perror("list_pop_from_front");
                     return 0;
                  }
                  //a file will be created and locked
                  SET_MASK(flags, O_CREATE);
                  SET_MASK(flags, O_LOCK);
                  openFile(file_name, flags);
                  RESET_MASK(flags);
                  if (i + 2 < argc - 1){
                     if (cmds[i+2][0] == 'D')
                        writeFile(file_name, opts[i+2]);
                  }else {
                     writeFile(file_name, NULL);
                  }
                  unlockFile(file_name);
                  closeFile(file_name);
                  usleep(1000 * sleep_in_msec);
                  free(file_name); file_name = NULL;
               }
               list_free(written_files); written_files = NULL;
               break;
				}
				break;
			case 'W':
				// write files separated by comma
				new = opts[i];
				if (strchr(new, ',')){
					//there is a list of files to be written
               token = strtok_r(new, ",", &save_ptr);
               while (token){
                  //a file will be created and locked
                  SET_MASK(flags, O_CREATE);
                  SET_MASK(flags, O_LOCK);
                  openFile(token, flags);
                  RESET_MASK(flags);
                  if (i + 2 < argc - 1 && cmds[i+2][0] == 'D')
                     writeFile(token, opts[i+2]);
                  else {
                     writeFile(token, NULL);
                  }
                  unlockFile(token);
                  closeFile(token);
                  usleep(sleep_in_msec * 1000);
                  token = strtok_r(NULL, ",", &save_ptr);
               }
            }else{
               //there are no commas, only one file will be written
               SET_MASK(flags, O_CREATE);
               SET_MASK(flags, O_LOCK);
               openFile(new, flags);
               RESET_MASK(flags);
               if (i + 2 < argc - 1){
                  if (cmds[i+2][0] == 'D')
                     writeFile(new, opts[i+2]);
               }else {
                  writeFile(new, NULL);
               }
               unlockFile(new);
               closeFile(new);
               usleep(1000 * sleep_in_msec);
               break;
				}
				break;
			case 'r':
				// list of files to read separated by comma
				new = opts[i];
				if (strchr(new, ',')) {
               //there is a list of files to be read
               token = strtok_r(new, ",", &save_ptr);
               while (token){
                  //the file will be opened without flags
                  RESET_MASK(flags);
                  openFile(token, flags);
                  readFile(token, (void**) &read_file, &read_size);
                  closeFile(token);
                  //the option d has been specified, all files read
                  //will be saved to file_path
                  if (i + 2 < argc - 1 && cmds[i+2][0] == 'd'){
                     //build the absolute path of the file to be saved,
                     //the contents of token, separated by the file path
                     memset(file_path, 0, PATH_MAX);
                     err = snprintf(file_path, PATH_MAX, "%s/%s", token, opts[i+2]);
                     if (err <= 0 || err > PATH_MAX){
                        perror("snprintf");
                        return 0;
                     }
                     //save the file read
                     if (save_file(file_path, read_file) == -1){
                        perror("save_file");
                        return 0;
                     }

                  }
                  free(read_file);
                  read_file = NULL;
                  //sleep some milliseconds before another request
                  usleep(1000 * sleep_in_msec);
                  token = strtok_r(NULL, ",", &save_ptr);
               }
				}else{
               //there is a single file to be read
               //open the file without flags
               RESET_MASK(flags);
               openFile(new, flags);
               readFile(new, (void** )&read_file, &read_size);
               closeFile(new);
               if (i + 2 < argc - 1 && cmds[i+2][0] == 'd'){
                  //build the absolute path
                  memset(file_path, 0, PATH_MAX);
                  err = snprintf(file_path, PATH_MAX, "%s/%s", opts[i+2], new);
                  if (err <= 0 || err > PATH_MAX){
                     perror("snprintf");
                     return 0;
                  }
                  //save the file
                  if (save_file(file_path, read_file) == -1){
                     perror("save_file");
                     return 0;
                  }
               }
               free(read_file);
               read_file = NULL;
               //sleep some milliseconds before another request
               usleep(1000 * sleep_in_msec);
               break;
				}
				break;
			case 'R':
				// read N files
				new = opts[i];
				//if no number has been specified, read all files
				if (new[0] != '\0') {
               sscanf(opts[i], "%d", &N);
            }else {
               N = 0;
            }
            //the option d has been specified, all files read
            //will be saved there
				if (i + 2 < argc - 1 && cmds[i+2][0] == 'd') {
               readNFiles(N, opts[i + 2]);
            }else {
               readNFiles(N, NULL);
            }
            //sleep some milliseconds before the next request
				usleep(1000 * sleep_in_msec);
				break;
			case 't':
				// set time to sleep before requests
				sscanf(opts[i], "%d", &sleep_in_msec);
				break;
			case 'l':
            //lock the file(s)
				new = opts[i];
            //open the file with no flags set
				RESET_MASK(flags);
				if (strchr(new, ',')){
               //there are multiple files to be locked
               token = strtok_r(new, ",", &save_ptr);
               while (token){
                  openFile(token, flags);
                  lockFile(token);
                  closeFile(token);
                  //sleep some milliseconds before the next request
                  usleep(1000 * sleep_in_msec);
                  token = strtok_r(NULL, ",", &save_ptr);
               }
				}else{
               //only one file to be locked
               openFile(new, flags);
               lockFile(new);
               closeFile(new);
               usleep(1000 * sleep_in_msec);
				}
				break;
			case 'u':
            //unlock the file(s)
				new = opts[i];
            //open the file with no flags
				RESET_MASK(flags);

				if (strchr(new, ',') ) {
               //there are multiple files to be unlocked
               token = strtok_r(new, ",", &save_ptr);
               while (token){
                  //there are multiple files to be unlocked
                  openFile(token, flags);
                  unlockFile(token);
                  closeFile(token);
                  token = strtok_r(NULL, ",", &save_ptr);
                  //sleep some milliseconds between requests
                  usleep(1000 * sleep_in_msec);
               }
				}else{
               //there is only one file to be unlocked
               openFile(new, flags);
               unlockFile(new);
               closeFile(new);
               //sleep some milliseconds between requests
               usleep(1000 * sleep_in_msec);
				}
				break;
			case 'c':
            //cancel the file(s)
				new = opts[i];
            //open the file with no flags
				RESET_MASK(flags);
				if (strchr(new, ',')){
               //there are multiple files to be canceled
               token = strtok_r(new, ",", &save_ptr);
               while (token){
                  openFile(token, flags);
                  removeFile(token);
                  closeFile(token);
                  token = strtok_r(NULL, ",", &save_ptr);
                  //sleep some milliseconds between requests
                  usleep(1000 * sleep_in_msec);
               }
				}else{
               //there is only one file to be canceled
               openFile(new, flags);
               removeFile(new);
               //sleep some milliseconds between requests
               usleep(1000 * sleep_in_msec);
				}
				break;
         case 'p':
            verbose_mode = true;
            break;
			default:
				break;
		}
	}
	return 0;

	failure:
		if (opts){
			int j = 0;
			while (j != i){
				free(cmds[j]);
				j++;
			}
			i = argc - 1;
		}
		free(opts);
		if (cmds){
			int j = 0;
			while (j != i){
				free(cmds[j]);
				j++;
			}
		}
		free(cmds);
		freed = true;
		return 1;
}

static int isdot(const char dir[]){
   int l = strlen(dir);

   if ( (l>0 && dir[l-1] == '.') ) return 1;
   return 0;
}


static int lsR(const char dir_path[], linked_list_t* files){
	if (!dir_path || !files){
		errno = EINVAL;
		return -1;
	}
   //changes the process' working directory to dir_path
	if (chdir(dir_path) == -1) return 0;
	DIR* dir;
   //opening the directory
	if ((dir=opendir("."))){
		struct dirent *file;
      //reads files inside the directory
		while(errno = 0, (file = readdir(dir))){
         //if it's a regulare file get its path
			if (file->d_type == DT_REG) {
            char* buf = malloc(NAME_LEN_MAX * sizeof(char));
	         if (!buf) return -1;
	         getcwd(buf, NAME_LEN_MAX);
				if (!buf) return -1;
				char* file_path = malloc(sizeof(char) *  BUF_LEN_MAX);
				if (!file_path) return -1;
				memset(file_path, 0,  BUF_LEN_MAX);
				snprintf(file_path,  BUF_LEN_MAX, "%s/%s", buf, file->d_name);
            //add the file to the linked list of files
				if (list_push_to_back(files, file_path, strlen(file_path) + 1, NULL, 0) == -1) return -1;
				//free allocated resources
            free(file_path);
				free(buf);
			}else if (isdot(file->d_name)==-1){
            //if it'a a normal directory(not '.') call lsR and return to the parent directory
				if (lsR(file->d_name, files) != 0){
					if (chdir("..") == -1) return -1;
				}
			}
		}
		if (errno != 0) return -1;
		closedir(dir);
	}else return -1;//error while opening the directory

	return 0;
}

static int parse_cmdline(const char** cmds, const char** opts, int len){
	if (!cmds || !opts || len <= 0){
		errno = EINVAL;
		return 1;
	}

	if (cmds[0][0] == '\0'){
		errno = EINVAL;
		return 1;
	}
   //parse all possible commands
	for (int i = 0; i < len; i++){
		if (cmds[i][0] == '\0' && opts[i][0] != '\0'){
			errno = EINVAL;
			return 1;
		}
		if (cmds[i][0] == '\0') continue;

		if (cmds[i][0] == 'h'){
			//the -h option takes no arguments
			if (opts[i][0] != '\0'){
				errno = EINVAL;
				return 1;
			}
         //if set to true, the helper message will be printed
			helper_tgl = true;
			continue;
		}
		if (cmds[i][0] == 'f'){
			// -f takes only an argument
			if (opts[i][0] == '\0'){
				errno = EINVAL;
				return 1;
			}

			memset(socket_name, 0, PATH_LEN_MAX);
			snprintf(socket_name, PATH_LEN_MAX, "%s", opts[i]);
			continue;
		}
      
		if (cmds[i][0] == 'p'){
			// -p takes no argument
			if (opts[i][0] != '\0'){
				errno = EINVAL;
				return 1;
			}
			//if specified, all operations will be printed to stdout
			verbose_mode = true;
			continue;
		}

		if (cmds[i][0] == 'w'){
         //takes one or more arguments
			if (opts[i][0] == '\0'){
				errno = EINVAL;
				return 1;
			}
			if (!(strrchr(opts[i], ','))) continue;
			else{
				char* copy = (char*) malloc(sizeof(char) * (strlen(opts[i]) + 1));
				if (!copy) return 1;
				strcpy(copy, opts[i]);
				char* new = copy;
				char* save_ptr;
				char* token = strtok_r(copy, ",", &save_ptr);
				// a file must always be specified after a ','
				token = strtok_r(NULL, ",", &save_ptr);
				if (!token){
					free(new);
					return 1;
				}
				int n;
				// checking for a number after the file(s)
				if (sscanf(token, "%d", &n) != 1){
					free(new);
					errno = EINVAL;
					return 1;
				}
				token = strtok_r(NULL, ",", &save_ptr);
				if (token){
					free(new);
					errno = EINVAL;
					return 1;
				}
				free(new);
				continue;
			}
		}

      if (cmds[i][0] == 'R'){
         if (opts[i][0] == '\0') continue;
         int new;
         //checking for a number
         if (sscanf(opts[i], "%d", &new) != 1){
            errno = EINVAL;
            return 1;
         }
         continue;
      }
      
      if (cmds[i][0] == 't'){
         if (opts[i][0] == '\0'){
            errno = EINVAL;
            return 1;
         }
         // checking for a number
         int new;
         if (sscanf(opts[i], "%d", &new) != 1){
            errno = EINVAL;
            return 1;
         }
         continue;
      }

      if (cmds[i][0] == 'd') {
         if (i == 0){
            errno = EINVAL;
            return 1;
         }
         if (opts[i][0] == '\0'){
            errno = EINVAL;
            return 1;
         }
         //the -d command must follow reading operations
         if (cmds[i-2][0] != 'r' && cmds[i-2][0] != 'R' && cmds[i-1][0] != 'R'){
            errno = EINVAL;
            return 1;
         }
         continue;
      }

      if (cmds[i][0] == 'D') {

         if (i == 0){
            errno = EINVAL;
            return 1;
         }
         if (opts[i][0] == '\0'){
            errno = EINVAL;
            return 1;
         }
         //the -D command must follow writing operations
         if (cmds[i-2][0] != 'w' && cmds[i-2][0] != 'W'){
            errno = EINVAL;
            return 1;
         }
         continue;
      }

		if (cmds[i][0] == 'W' || cmds[i][0] == 'r' || cmds[i][0] == 'c' ||
					cmds[i][0] == 'l' || cmds[i][0] == 'u'){
			if (opts[i][0] == '\0'){
				errno = EINVAL;
				return 1;
			}
         //a list of one or more files must be specified, separated by comma
			if (!strrchr(opts[i], ',')) continue;
			if (opts[i][strlen(opts[i]) - 1] == ','){
				errno = EINVAL;
				return 1;
			}
		}

	}
	return 0;
}

void client_free(){
	if (freed) return;
	if (connected) closeConnection(socket_name);
	for (int i = 0; i < len; i++){
		free(cmds[i]);
		free(opts[i]);
	}
	free(cmds);
	free(opts);
	list_free(written_files);
	free(file_name);
	free(read_file);
	return;
}

