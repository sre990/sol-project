/**
 * @brief implementation for the configuration file parser.
 *
*/

#include <parser.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#include <defines.h>


#define FIELDS 6
#define WORKERS "NUMBER OF WORKER THREADS = "
#define FILES_MAX "MAX NUMBER OF FILES ACCEPTED = "
#define CACHE_SIZE "MAX CACHE SIZE = "
#define SOCKET_PATH "SOCKET FILE PATH = "
#define LOG_PATH "LOG FILE PATH = "
#define POLICY "REPLACEMENT POLICY = "

#define CHECK_LIMIT(x,label) \
if((x)==ULONG_MAX  && errno == ERANGE){ \
   goto label;\
}

struct _parser{
	unsigned long workers;
   unsigned long files_max;
	unsigned long cache_size;
	char socket_path[PATH_LEN_MAX];
	char log_path[PATH_LEN_MAX];
	policy_t policy;
};

parser_t* parser_create(){
	parser_t* parser = malloc(sizeof(parser_t));
	if (!parser){
		errno = ENOMEM;
		return NULL;
	}
	parser->workers = 0;
	parser->files_max = 0;
	parser->cache_size = 0;
	memset(parser->socket_path, 0, PATH_LEN_MAX);
	memset(parser->log_path, 0, PATH_LEN_MAX);

	return parser;
}

int parser_update(parser_t* parser, const char* config_file_path){
	if (!parser || !config_file_path) {
      errno = EINVAL;
      parser->workers = 0;
      parser->files_max = 0;
      parser->cache_size = 0;
      memset(parser->socket_path, 0, PATH_LEN_MAX);
      memset(parser->log_path, 0, PATH_LEN_MAX);

      return -1;
   }

	FILE* config_file;
	if ((config_file = fopen(config_file_path, "r")) == NULL) return -1;

	char buffer[BUF_LEN_MAX];
	char* stub;
	bool workers_set = false;
   bool files_set = false;
   bool cache_set = false;
   bool socket_set = false;
   bool log_set = false;
   bool pol_set = false;
	unsigned long new;

	for (size_t i = 0;i < FIELDS;i++){
      //read each line of the config file
		stub = fgets(buffer, BUF_LEN_MAX, config_file);
		if (!stub && !feof(config_file)) return -1;
      //get number of workers
      if (strncmp(buffer, WORKERS, strlen(WORKERS)) == 0){
         //checking that the number of workers has not been
         //set more than once on the config file
			if (!workers_set) workers_set = true;
			else goto failure;
         //get number of workers from config file, negative
         //values are converted to positive
			new = strtoul(buffer + strlen(WORKERS), NULL, 10);
			if (new != 0){
            //check for overflow
            CHECK_LIMIT(new,failure);
				parser->workers = new;
			} else goto failure;
      //get max number of files
      }else if (strncmp(buffer, FILES_MAX, strlen(FILES_MAX)) == 0){
         //checking that the number of files has not been
         //set more than once on the config file
         if (!files_set) files_set = true;
			else goto failure;
         //get number of files from config file, negative 
         //values are converted to positive
			new = strtoul(buffer + strlen(FILES_MAX), NULL, 10);
			if (new != 0) {
            //check for overflow
            CHECK_LIMIT(new,failure);
            parser->files_max = new;

			} else goto failure;
		}else if (strncmp(buffer, CACHE_SIZE, strlen(CACHE_SIZE)) == 0){
         //checking that the cache size has not been
         //set more than once on the config file			
         if (!cache_set) cache_set = true;
			else goto failure;
         //get cache size from config file, negative values
         //are converted to positive
			new = strtoul(buffer + strlen(CACHE_SIZE), NULL, 10);
			if (new != 0) {
            //check for overflow
            CHECK_LIMIT(new,failure);
            parser->cache_size = new;
			}
			else goto failure;
		}else if (strncmp(buffer, SOCKET_PATH, strlen(SOCKET_PATH)) == 0){
         //checking that the socket path has not been
         //set more than once on the config file
			if (!socket_set) socket_set = true;
			else goto failure;
         //get the socket path from config file
         strncpy(parser->socket_path, buffer + strlen(SOCKET_PATH), PATH_LEN_MAX);
			parser->socket_path[strcspn(parser->socket_path, "\n")] = '\0';
		}else if (strncmp(buffer, LOG_PATH, strlen(LOG_PATH)) == 0){
         //checking that the log file path has not been
         //set more than once on the config file
			if (!log_set) log_set = true;
			else goto failure;
		   //get the log file path from config file
			strncpy(parser->log_path, buffer + strlen(LOG_PATH), PATH_LEN_MAX);
			parser->log_path[strcspn(parser->log_path, "\n")] = '\0';
		}else if (strncmp(buffer, POLICY, strlen(POLICY)) == 0){
         //checking that the replacement policy has not been
         //set more than once on the config file
			if (!pol_set) pol_set = true;
			else goto failure;
		   //get the replacement policy from config file
			new = strtoul(buffer + strlen(POLICY), NULL, 10);
         //the replacement policy only goes from 0 to 2,
         //overflow impossible
			if (new <= 2){
				parser->policy = new;
			}else {
            goto failure;
         }
		}
	}
	if (fclose(config_file) != 0) return -1;
	return 0;

	failure:
		parser->workers = 0;
		parser->files_max = 0;
		parser->cache_size = 0;
		memset(parser->socket_path, 0, PATH_LEN_MAX);
		memset(parser->log_path, 0, PATH_LEN_MAX);
		fclose(config_file);
		errno = EINVAL;

		return -1;
}

unsigned long parser_get_workers(const parser_t* parser){
	if (!parser){
		errno = EINVAL;
		return 1;
	}
	return parser->workers;
}

unsigned long parser_get_files(const parser_t* parser){
	if (!parser){
		errno = EINVAL;
		return 1;
	}
	return parser->files_max;
}

unsigned long parser_get_size(const parser_t* parser){
	if (!parser){
		errno = EINVAL;
		return 1;
	}
	return parser->cache_size;
}

unsigned long parser_get_log_path(const parser_t* parser, char** log_path_ptr){
	if (!parser || !log_path_ptr){
		errno = EINVAL;
		return 0;
	}
	char* new = malloc(sizeof(char) * PATH_LEN_MAX);
	if (!new){
		errno = ENOMEM;
		return 0;
	}
	strncpy(new, parser->log_path, PATH_LEN_MAX);
	*log_path_ptr = new;
	return strlen(new);
}


unsigned long parser_get_sock_path(const parser_t* parser, char** socket_path_ptr){
	if (!parser || !socket_path_ptr){
		errno = EINVAL;
		return 0;
	}
	char* new = malloc(sizeof(char) * PATH_LEN_MAX);
	if (!new){
		errno = ENOMEM;
		return 0;
	}
	strncpy(new, parser->socket_path, PATH_LEN_MAX);
	*socket_path_ptr = new;
	return strlen(new);
}

policy_t parser_get_policy(const parser_t* parser){
	if (!parser){
		errno = EINVAL;
		return 0;
	}
	return parser->policy;
}

void parser_free(parser_t* parser){
	free(parser);
}