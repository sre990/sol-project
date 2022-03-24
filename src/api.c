/**
 * @brief implentation of the communication api between client and server
 *
*/

#define _DEFAULT_SOURCE

#include <errno.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <defines.h>
#include <api.h>
#include <utilities.h>

static int fd_socket = -1;
static int default_flags = -1;
static int default_N = -1;
static char socket_path[PATH_LEN_MAX];
static char file_path[PATH_LEN_MAX];
static char dir_path[PATH_LEN_MAX];

bool verbose_mode = true;
bool strict_mode = true;

int openConnection(const char* sockname, int msec, const struct timespec abstime){
	int err;
	char err_str[REQ_LEN_MAX];
   strcpy(socket_path, sockname);

	if (fd_socket != -1){
		err = EISCONN;
		fail_with(OPEN_CONN,default_flags,default_N,socket_path,err_str,err);
	}

	if (!sockname || strlen(sockname) > PATH_LEN_MAX || msec < 0){
		err = EINVAL;
      fail_with(OPEN_CONN,default_flags,default_N,socket_path,err_str,err);
	}

	fd_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd_socket == -1){
		err = errno;
      fail_with(OPEN_CONN,default_flags,default_N,socket_path,err_str,err);
	}

	struct sockaddr_un sock_addr;
	strncpy(sock_addr.sun_path, sockname, strlen(sockname) + 1);
	sock_addr.sun_family = AF_UNIX;

	errno = 0;
   //try to connect every msec milliseconds and stop after abstime
	while (connect(fd_socket, (struct sockaddr*) &sock_addr, sizeof(sock_addr)) == -1){
		if (errno != ENOENT){
			err = errno;
         fail_with(OPEN_CONN,default_flags,default_N,socket_path,err_str,err);
		}
		time_t curr = time(NULL);
		if (abstime.tv_sec<curr){
			fd_socket = -1;
			err = EAGAIN;
         fail_with(OPEN_CONN,default_flags,default_N,socket_path,err_str,err);
      }
      // time between connections attempts is expressed in milliseconds
		usleep(msec * 1000);
		errno = 0;
	}

   return succeed_with(OPEN_CONN,default_flags,default_N,socket_path);

}

int closeConnection(const char* sockname){
	int err;
	char err_str[REQ_LEN_MAX];

	if (!sockname){
		err = EINVAL;
      fail_with(CLOSE_CONN,default_flags,default_N,socket_path,err_str,err);
   }
	if (strcmp(sockname, socket_path) != 0){
		err = ENOTCONN;
      fail_with(CLOSE_CONN,default_flags,default_N,socket_path,err_str,err);
	}
   strcpy(socket_path,sockname);

   // sends the server a shutdown request message
	char buffer[REQ_LEN_MAX];
	memset(buffer, 0, REQ_LEN_MAX);
	snprintf(buffer, REQ_LEN_MAX, "%d", SHUTDOWN);

   // a write operation can return less than we specified, so we use
   // writen to write the remainder of the data.
	if (writen((long) fd_socket, (void*) buffer, REQ_LEN_MAX) == -1){
		err = errno;
      fail_with(CLOSE_CONN,default_flags,default_N,socket_path,err_str,err);
	}

	if (close(fd_socket) == -1){
		err = errno;
		fd_socket = -1;
      fail_with(CLOSE_CONN,default_flags,default_N,socket_path,err_str,err);
	}

	fd_socket = -1;

   return succeed_with(CLOSE_CONN,default_flags,default_N,socket_path);

}

int openFile(const char* pathname, int flags){
	int err;
	char err_str[ERROR_LEN_MAX];
   strcpy(file_path,pathname);

	if (!pathname || strlen(pathname) > PATH_LEN_MAX){
		err = EINVAL;
      fail_with(OPEN_FILE,flags,default_N,file_path,err_str,err);

   }
	if (fd_socket == -1){
		err = ENOTCONN;
      fail_with(OPEN_FILE,flags,default_N,file_path,err_str,err);
	}


   // sending an open file request to the server inside a buffer
	char buffer[REQ_LEN_MAX];
	memset(buffer, 0, REQ_LEN_MAX);
	snprintf(buffer, REQ_LEN_MAX, "%d %s %d", OPEN, pathname, flags);

   // a write operation can return less than we specified, so we use
   // writen to write the remainder of the data.
	if (writen((long) fd_socket, (void*) buffer, REQ_LEN_MAX) == -1){
		err = errno;
      fail_with(OPEN_FILE,flags,default_N,file_path,err_str,err);
	}

   //reading the response from the server
	char feedback_str[OP_LEN_MAX];
	memset(feedback_str, 0, OP_LEN_MAX);
   // a read operation can return less than we asked for, so we use
   // readn to read the remainder of the data.
	if (readn((long) fd_socket, (void*) feedback_str, OP_LEN_MAX) == -1){
		err = errno;
      fail_with(OPEN_FILE,flags,default_N,file_path,err_str,err);
	}

	int feedback;
	if (sscanf(feedback_str, "%d", &feedback) != 1){
		err = EBADMSG;
      fail_with(OPEN_FILE,flags,default_N,file_path,err_str,err);
	}
	char errno_str[ERRNO_LEN_MAX];
	// handling the response from the server
	switch (feedback){
		case OP_SUCCESS:
			break;
		case OP_FAILURE:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(OPEN_FILE,flags,default_N,file_path,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(OPEN_FILE,flags,default_N,file_path,err_str,err);
			}
         strerror_r(err, err_str, REQ_LEN_MAX);
         PRINT_IF(verbose_mode, "%s-> %s %s %d with errno = %s.\n", FAILURE, OPEN_FILE,
                  file_path, flags, err_str);
         errno = err;
         return -1;
      case OP_EXIT_FATAL:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(OPEN_FILE,flags,default_N,file_path,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(OPEN_FILE,flags,default_N,file_path,err_str,err);
			}
         abort_with(OPEN_FILE,flags,default_N,file_path,err_str,err);
      default:
         break;
	}

	return succeed_with(OPEN_FILE,flags,default_N,file_path);
}

int readFile(const char* pathname, void** buf, size_t* size){
	int err;
	char err_str[REQ_LEN_MAX];
   strcpy(file_path,pathname);

	if (!pathname || strlen(pathname) > PATH_LEN_MAX){
		err = EINVAL;
      fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
	}
	if (fd_socket == -1){
		err = ENOTCONN;
      fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
	}

	if (buf) *buf = NULL;
	if (size) *size = 0;

   // sending a read file request to the server inside a buffer
	char buffer[REQ_LEN_MAX];
	memset(buffer, 0, REQ_LEN_MAX);
	if (buf && size)
		snprintf(buffer, REQ_LEN_MAX, "%d %s 1", READ, pathname);
	else
		snprintf(buffer, REQ_LEN_MAX, "%d %s 0", READ, pathname);

   // a write operation can return less than we specified, so we use
   // writen to write the remainder of the data.
	if (writen((long) fd_socket, (void*) buffer, REQ_LEN_MAX) == -1){
		err = errno;
      fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
	}
	// reading the response from server
	char feedback_str[OP_LEN_MAX];
	memset(feedback_str, 0, OP_LEN_MAX);
	if (readn((long) fd_socket, (void*) feedback_str, OP_LEN_MAX) == -1){
		err = errno;
      fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
	}
	int feedback;
	if (sscanf(feedback_str, "%d", &feedback) != 1){
		err = EBADMSG;
      fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
	}
	char errno_str[ERRNO_LEN_MAX];
	bool failure = false, fatal = false;
	// handling the response from server
	switch (feedback){
		case OP_SUCCESS:
			break;
		case OP_FAILURE:
         // a read operation can return less than we asked for, so we use
         // readn to read the remainder of the data.
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
			}
			failure = true;
			break;
		case OP_EXIT_FATAL:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
			}
			fatal = true;
			break;
      default:
         break;
	}

	char* read_buffer = NULL;
	size_t read_size = 0;
   //storing the contents of the read in a buffer
	char msg_size[SIZE_LEN];
	memset(msg_size, 0, SIZE_LEN);
	if (readn((long) fd_socket, (void*) msg_size, SIZE_LEN) == -1){
		err = errno;
      fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
	}
	if (sscanf(msg_size, "%lu", &read_size) != 1){
		err = EBADMSG;
      fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
	}
	if (read_size !=  0){
		read_buffer = (char*) malloc(sizeof(char) * (read_size + 1));
		if (!read_buffer){
			err = errno;
         abort_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
		}
		memset(read_buffer, 0, read_size + 1);
		if (readn((long) fd_socket, (void*) read_buffer, read_size) == -1){
			err = errno;
         fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
		}
		read_buffer[read_size] = '\0';
	}
	*size = read_size;
	*buf = (void*) read_buffer;

	if (failure) {
      fail_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
   }else if (fatal){
      abort_with(READ_FILE,default_flags,default_N,file_path,err_str,err);
   }

	return succeed_with(READ_FILE,default_flags,default_N,file_path);

}

int readNFiles(int N, const char* dirname){
	int err;
	char err_str[REQ_LEN_MAX];
   if(!dirname || strlen(dirname) > PATH_LEN_MAX){
      errno = EINVAL;
      fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,errno);
   }
   strcpy(dir_path,dirname);

	if (fd_socket == -1){
		err = ENOTCONN;
      fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
	}

   // sending a read N files request to the server inside a buffer
   char buffer[REQ_LEN_MAX];
	memset(buffer, 0, REQ_LEN_MAX);
	snprintf(buffer, REQ_LEN_MAX, "%d %d", READ_N, N);

   // a write operation can return less than we specified, so we use
   // writen to write the remainder of the data.
	if (writen((long) fd_socket, (void*) buffer, REQ_LEN_MAX) == -1){
		err = errno;
      fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
	}
   // reading the response from server
	char feedback_str[OP_LEN_MAX];
	memset(feedback_str, 0, OP_LEN_MAX);
   // a read operation can return less than we asked for, so we use
   // readn to read the remainder of the data.
	if (readn((long) fd_socket, (void*) feedback_str, OP_LEN_MAX) == -1){
		err = errno;
      fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
	}
	int feedback;
	if (sscanf(feedback_str, "%d", &feedback) != 1){
		err = EBADMSG;
      fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
	}
	char errno_str[ERRNO_LEN_MAX];
	bool failure = false, fatal = false;
   // handling the response from server
	switch (feedback){
		case OP_SUCCESS:
			break;
		case OP_FAILURE:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
			}
			failure = true;
			break;
		case OP_EXIT_FATAL:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
			}
			fatal = true;
			break;
      default:
         break;
	}
   //storing the contents of the read in a buffer
	char msg_size[SIZE_LEN];
	memset(msg_size, 0, SIZE_LEN);
	if (readn((long) fd_socket, (void*) msg_size, SIZE_LEN) == -1){
		err = errno;
      fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
	}
   //getting the number of files to be read
	size_t reads = 0;
	if (sscanf(msg_size, "%lu", &reads) != 1){
		err = EBADMSG;
      fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
	}

   for (int i=0; i<reads; i++){
      memset(buffer, 0, REQ_LEN_MAX);
      if (readn((long) fd_socket, buffer, REQ_LEN_MAX) == -1){
         err = errno;
         fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
      }

      memset(msg_size, 0, SIZE_LEN);
      if (readn((long) fd_socket, msg_size, SIZE_LEN) == -1){
         err = errno;
         fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
      }
      //get size of content to be read
      size_t content_size;
      if (sscanf(msg_size, "%lu", &content_size) != 1){
         err = EBADMSG;
         fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
      }
      //read the actual content
      char* contents = NULL;
      if (content_size != 0){
         contents = (char*) malloc(content_size + 1);
         if (!contents){
            err = errno;
            fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
         }
         memset(contents, 0, content_size + 1);
         if (readn((long) fd_socket, (void*) contents, content_size) == -1){
            err = errno;
            fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
         }
      }
      // if a directory name has been specified, read files are saved to cache
      if (dirname){
         //getting the length of the path of the directory
         size_t dir_len = strlen(dirname);
         //checking that the absolute path of the file is not too long
         if (dir_len + strlen(buffer) > PATH_MAX){
            err = ENAMETOOLONG;
            fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
         }
         memmove(buffer + dir_len, buffer, strlen(buffer) + 1);
         memcpy(buffer, dirname, dir_len);
         // saving the file to cache
         if (save_file(buffer, contents) == -1){
            err = errno;
            fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
         }
      }
      free(contents); contents = NULL;
	}

	if (failure) {
      fail_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
   }else if (fatal) {
      abort_with(READ_N_FILES,default_flags,N,dir_path,err_str,err);
   }

   return succeed_with(READ_N_FILES,default_flags,N,dir_path);
}


int writeFile(const char* pathname, const char* dirname){
	int err;
	char err_str[REQ_LEN_MAX];
	if (!pathname || strlen(pathname) > PATH_LEN_MAX){
		err = EINVAL;
		if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
	}

	if (fd_socket == -1){
		err = ENOTCONN;
      if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }

	long length = 0;
   // checks if the file is valid
	err = is_file(pathname);
	if (err == -1){
		err = errno;
      if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
	if (err == 0){
		err = EINVAL;
      if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }

   // opening the file located in pathname
   FILE* file = fopen(pathname, "r");
	char* contents = NULL;
	if (!file){
		err = errno;
      if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }

   //getting the length of file's contents
   err = fseek(file, 0, SEEK_END);
	if (err != 0){
		err = errno;
		fclose(file);
      if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
   //getting the length of file's contents
	length = ftell(file);
	if (fseek(file, 0, SEEK_SET) != 0){
		err = errno;
		fclose(file);
      if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
	// reading file contents
	if (length != 0){
		contents = (char*) malloc(sizeof(char) * (length + 1));
		if (!contents){
			err = errno;
			fclose(file);
         if(dirname){
            goto failure;
         }else{
            fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
         }
      }
		size_t read_length = fread(contents, sizeof(char), length, file);
		if (read_length != length && ferror(file)){
			fclose(file);
			free(contents);
			err = EBADE;
         if(dirname){
            goto failure;
         }else{
            fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
         }
      }
		contents[length] = '\0'; // string needs to be null terminated
	}
	fclose(file);

   //write file request from client to server using a buffer
	char buffer[REQ_LEN_MAX];
	memset(buffer, 0, REQ_LEN_MAX);
	snprintf(buffer, REQ_LEN_MAX, "%d %s %lu", WRITE, pathname, length);

   // a write operation can return less than we specified, so we use
   // writen to write the remainder of the data.
	if (writen((long) fd_socket, (void*) buffer, REQ_LEN_MAX) == -1){
		err = errno;
      if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
   // sending the contents just read to be stored in dirname
	if (length != 0){
		if (writen((long) fd_socket, (void*) contents, length) == -1){
			err = errno;
			free(contents);
         if(dirname){
            goto failure;
         }else{
            fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
         }
      }
		free(contents);
	}
   // feedback response from server
   char feedback_str[OP_LEN_MAX];
	memset(feedback_str, 0, OP_LEN_MAX);
	if (readn((long) fd_socket, (void*) feedback_str, OP_LEN_MAX) == -1){
		err = errno;
      if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
	int feedback;
	if (sscanf(feedback_str, "%d", &feedback) != 1){
		err = EBADMSG;
      if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
	char errno_str[ERRNO_LEN_MAX];
	bool failure = false, fatal = false;
	// handling server response
	switch (feedback){
		case OP_SUCCESS:
			break;
		case OP_FAILURE:
         // read operations may return less than we asked for, therefore
         // we must check that the data has been read fully
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            if(dirname){
               goto failure;
            }else{
               fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
            }
         }
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            if(dirname){
               goto failure;
            }else{
               fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
            }
         }
			failure = true;
			break;

		case OP_EXIT_FATAL:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            if(dirname){
               goto failure;
            }else{
               fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
            }
         }
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            if(dirname){
               goto failure;
            }else{
               fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
            }
         }
			fatal = true;
			break;
      default:
         break;
	}
   //handling capacity misses
	char msg_size[SIZE_LEN];
	memset(msg_size, 0, SIZE_LEN);
	if (readn((long) fd_socket, (void*) msg_size, SIZE_LEN) == -1){
		err = errno;
      if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
   //getting number of evicted files(i.e. 'victims')
	size_t evicted = 0;
	if (sscanf(msg_size, "%lu", &evicted) != 1){
		err = EBADMSG;
      if(dirname){
         goto failure;
      }else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }

   for (int i=0;i<evicted;i++){
      // getting destination file name to save the victim to
      memset(buffer, 0, REQ_LEN_MAX);
      if (readn((long) fd_socket, buffer, REQ_LEN_MAX) == -1){
         err = errno;
         if(dirname){
            goto failure;
         }else{
            fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
         }
      }
      // get size of content
      memset(msg_size, 0, SIZE_LEN);
      if (readn((long) fd_socket, msg_size, SIZE_LEN) == -1){
         err = errno;
         if(dirname){
            goto failure;
         }else{
            fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
         }
      }
      size_t content_size;
      if (sscanf(msg_size, "%lu", &content_size) != 1){
         err = EBADMSG;
         if(dirname){
            goto failure;
         }else{
            fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
         }
      }
      // getting file contents
      char* contents = NULL;
      if (content_size != 0){
         contents = (char*) malloc(content_size + 1);
         if (!contents){
            err = errno;
            if(dirname){
               goto fatal;
            }else{
               abort_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
            }
         }
         memset(contents, 0, content_size + 1);
         if (readn((long) fd_socket, (void*) contents, content_size) == -1){
            err = errno;
            if(dirname){
               goto failure;
            }else{
               fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
            }
         }
      }
      // files read from server must be stored in a destination directory
      if (dirname){
         // directory path where victims will be stored
         size_t dir_len = strlen(dirname);
         if (dir_len + strlen(buffer) > PATH_MAX){
            err = ENAMETOOLONG;
            goto failure;
         }
         memmove(buffer + dir_len, buffer, strlen(buffer) + 1);
         memcpy(buffer, dirname, dir_len);
         // saving file to cache
         if (save_file(buffer, contents) == -1){
            err = errno;
            goto failure;
         }
      }
      free(contents);
      contents = NULL;
   }

	if (failure) goto failure;
	if (fatal) goto fatal;

	if (dirname){
		PRINT_IF(verbose_mode, "%s-> %s %s %s.\n",SUCCESS, WRITE_FILE, pathname, dirname);
      return 0;
	}else{
      return succeed_with(WRITE_FILE, default_flags,default_N,pathname);
   }

	failure:
		strerror_r(err, err_str, REQ_LEN_MAX);
		if (dirname){
         PRINT_IF(verbose_mode, "%s-> %s %s %s with errno = %s.\n",FAILURE, WRITE_FILE,
                  pathname, dirname, err_str);

		}else{
         fail_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);
      }
		errno = err;
		return -1;

	fatal:
		strerror_r(err, err_str, REQ_LEN_MAX);
		if (dirname){
         PRINT_IF(verbose_mode, "%s-> %s %s %s with errno = %s.\n",EXIT_FATAL, WRITE_FILE,
                  pathname, dirname, err_str);
		}else{
         abort_with(WRITE_FILE,default_flags,default_N,dir_path,err_str,err);

		}
		errno = err;
		if (strict_mode) exit(errno);
		else return -1;
}


int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
	int err;
	char err_str[REQ_LEN_MAX];

	if (!pathname || strlen(pathname) > PATH_LEN_MAX){
		err = EINVAL;
      if(dirname){
         goto failure;
      }else{
         fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }

	if (fd_socket == -1){
		err = ENOTCONN;
      if(dirname){
         goto failure;
      }else{
         fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }

   //append to file request from client to server stored inside a buffer
	char buffer[REQ_LEN_MAX];
	memset(buffer, 0, REQ_LEN_MAX);
	snprintf(buffer, REQ_LEN_MAX, "%d %s %lu", APPEND, pathname, size);

   // a write operation can return less than we specified, so we use
   // writen to write the remainder of the data.
	if (writen((long) fd_socket, (void*) buffer, REQ_LEN_MAX) == -1){
		err = errno;
      if(dirname){
         goto failure;
      }else{
         fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
   // sending the contents of the buffer to append
   if (size != 0){
		if (writen((long) fd_socket, (void*) buf, size) == -1){
			err = errno;
         if(dirname){
            goto failure;
         }else{
            fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
         }
      }
	}
	// reading response from server
	char feedback_str[OP_LEN_MAX];
	memset(feedback_str, 0, OP_LEN_MAX);
   // read operations may return less than we asked for, therefore
   // we must check that the data has been read fully
	if (readn((long) fd_socket, (void*) feedback_str, OP_LEN_MAX) == -1){
		err = errno;
      if(dirname){
         goto failure;
      }else{
         fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
	int feedback;
	if (sscanf(feedback_str, "%d", &feedback) != 1){
		err = EBADMSG;
      if(dirname){
         goto failure;
      }else{
         fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
	char errno_str[ERRNO_LEN_MAX];
	bool failure = false, fatal = false;
	// handling response from server
	switch (feedback){
		case OP_SUCCESS:
			break;
		case OP_FAILURE:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            if(dirname){
               goto failure;
            }else{
               fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
            }
         }
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            if(dirname){
               goto failure;
            }else{
               fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
            }
         }
			failure = true;
			break;

		case OP_EXIT_FATAL:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            if(dirname){
               goto failure;
            }else{
               fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
            }
         }
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            if(dirname){
               goto failure;
            }else{
               fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
            }
         }
			fatal = true;
			break;
      default:
         break;
	}
   //handling capacity misses
	char msg_size[SIZE_LEN];
	memset(msg_size, 0, SIZE_LEN);
	if (readn((long) fd_socket, (void*) msg_size, SIZE_LEN) == -1){
		err = errno;
      if(dirname){
         goto failure;
      }else{
         fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
   //getting number of evicted files(i.e. 'victims')
	size_t evicted = 0;
	if (sscanf(msg_size, "%lu", &evicted) != 1){
		err = EBADMSG;
      if(dirname){
         goto failure;
      }else{
         fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
      }
   }
   for (int i=0;i<evicted;i++){
      // getting destination file name
      memset(buffer, 0, REQ_LEN_MAX);
      if (readn((long) fd_socket, buffer, REQ_LEN_MAX) == -1){
         err = errno;
         if(dirname){
            goto failure;
         }else{
            fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
         }
      }
      // getting evicted file contents length
      memset(msg_size, 0, SIZE_LEN);
      if (readn((long) fd_socket, msg_size, SIZE_LEN) == -1){
         err = errno;
         if(dirname){
            goto failure;
         }else{
            fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
         }
      }
      size_t content_size;
      if (sscanf(msg_size, "%lu", &content_size) != 1){
         err = EBADMSG;
         if(dirname){
            goto failure;
         }else{
            fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
         }
      }
      //getting file contents
      char* contents = NULL;
      if (content_size != 0){
         contents = (char*) malloc(content_size + 1);
         if (!contents){
            err = errno;
            goto fatal;
         }
         memset(contents, 0, content_size + 1);
         if (readn((long) fd_socket, (void*) contents, content_size) == -1){
            err = errno;
            if(dirname){
               goto failure;
            }else{
               fail_with(APPEND_TO_FILE,default_flags,default_N,dir_path,err_str,err);
            }
         }
      }
      // files read from server must be stored in a destination directory
      if (dirname){
         // directory path where victims will be stored
         size_t dir_len = strlen(dirname);
         if (dir_len + strlen(buffer) > PATH_MAX){
            err = ENAMETOOLONG;
            goto failure;

         }
         memmove(buffer + dir_len, buffer, strlen(buffer) + 1);
         memcpy(buffer, dirname, dir_len);
         // saving the file in the destination directory
         if (save_file(buffer, contents) == -1){
            err = errno;
            goto failure;
         }
      }
      free(contents);
      contents = NULL;
   }


	if (failure) goto failure;
	if (fatal) goto fatal;

   if(dirname){
      PRINT_IF(verbose_mode, "%s-> %s %s %s.\n", SUCCESS,APPEND_TO_FILE,pathname, dirname);
      return 0;
   }else{
      return succeed_with(APPEND_TO_FILE,default_flags,default_N,dir_path);
   }


failure:
		strerror_r(err, err_str, REQ_LEN_MAX);
		PRINT_IF(verbose_mode, "%s-> %s %s %s with errno = %s.\n", FAILURE,
               APPEND_TO_FILE,pathname,dirname, err_str);
		errno = err;
		return -1;

	fatal:
		strerror_r(err, err_str, REQ_LEN_MAX);
		PRINT_IF(verbose_mode, "%s-> %s %s %s with errno = %s.\n", EXIT_FATAL,
               APPEND_TO_FILE,pathname,dirname, err_str);
		errno = err;
		if (strict_mode) exit(errno);
		else return -1;
}

int lockFile(const char* pathname){
	int err;
	char err_str[REQ_LEN_MAX];
	if (!pathname || strlen(pathname) > PATH_LEN_MAX){
		err = EINVAL;
      fail_with(LOCK_FILE,default_flags,default_N,pathname,err_str,err);
	}
   strcpy(file_path,pathname);
	if (fd_socket == -1){
		err = ENOTCONN;
      fail_with(LOCK_FILE,default_flags,default_N,file_path,err_str,err);
	}

   //sending lock file request to server in a buffer
	char buffer[REQ_LEN_MAX];
	while (1){
		memset(buffer, 0, REQ_LEN_MAX);
		snprintf(buffer, REQ_LEN_MAX, "%d %s", LOCK, pathname);

      // a write operation can return less than we specified, so we use
      // writen to write the remainder of the data.
		if (writen((long) fd_socket, (void*) buffer, REQ_LEN_MAX) == -1){
			err = errno;
         fail_with(LOCK_FILE,default_flags,default_N,file_path,err_str,err);
		}
		// reading the server response
		char feedback_str[OP_LEN_MAX];
		memset(feedback_str, 0, OP_LEN_MAX);
      // read operations may return less than we asked for, therefore
      // we must check that the data has been read fully
		if (readn((long) fd_socket, (void*) feedback_str, OP_LEN_MAX) == -1){
			err = errno;
         fail_with(LOCK_FILE,default_flags,default_N,file_path,err_str,err);
		}
		int feedback;
		if (sscanf(feedback_str, "%d", &feedback) != 1){
			err = EBADMSG;
         fail_with(LOCK_FILE,default_flags,default_N,file_path,err_str,err);
		}
		char errno_str[ERRNO_LEN_MAX];
		// handling the server response
		switch (feedback){
			case OP_SUCCESS:
            return succeed_with(LOCK_FILE,default_flags,default_N,file_path);
         case OP_FAILURE:
				if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
					err = errno;
               fail_with(LOCK_FILE,default_flags,default_N,file_path,err_str,err);
				}
				if (sscanf(errno_str, "%d", &err) != 1){
					err = EBADMSG;
               fail_with(LOCK_FILE,default_flags,default_N,file_path,err_str,err);
				}
				if (err != EPERM){
               fail_with(LOCK_FILE,default_flags,default_N,file_path,err_str,err);
            }
         case OP_EXIT_FATAL:
				if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
					err = errno;
               fail_with(LOCK_FILE,default_flags,default_N,file_path,err_str,err);
				}
				if (sscanf(errno_str, "%d", &err) != 1){
					err = EBADMSG;
               fail_with(LOCK_FILE,default_flags,default_N,file_path,err_str,err);
				}
            abort_with(LOCK_FILE,default_flags,default_N,file_path,err_str,err);
         default:
            break;
		}
	}
}

int unlockFile(const char* pathname){
	int err;
	char err_str[REQ_LEN_MAX];
   strcpy(file_path, pathname);
	if (!pathname || strlen(pathname) > PATH_LEN_MAX){
		err = EINVAL;
      fail_with(UNLOCK_FILE,default_flags,default_N,pathname,err_str,err);
	}

	if (fd_socket == -1){
		err = ENOTCONN;
      fail_with(UNLOCK_FILE,default_flags,default_N,file_path,err_str,err);
	}

   //sending unlock file request to server in a buffer
	char buffer[REQ_LEN_MAX];
	memset(buffer, 0, REQ_LEN_MAX);
	snprintf(buffer, REQ_LEN_MAX, "%d %s", UNLOCK, pathname);

   // a write operation can return less than we specified, so we use
   // writen to write the remainder of the data.
	if (writen((long) fd_socket, (void*) buffer, REQ_LEN_MAX) == -1){
		err = errno;
      fail_with(UNLOCK_FILE,default_flags,default_N,file_path,err_str,err);
	}
	// reading the response from server
	char feedback_str[OP_LEN_MAX];
	memset(feedback_str, 0, OP_LEN_MAX);
	if (readn((long) fd_socket, (void*) feedback_str, OP_LEN_MAX) == -1){
		err = errno;
      fail_with(UNLOCK_FILE,default_flags,default_N,file_path,err_str,err);
	}
	int feedback;
	if (sscanf(feedback_str, "%d", &feedback) != 1){
		err = EBADMSG;
      fail_with(UNLOCK_FILE,default_flags,default_N,file_path,err_str,err);
	}
	char errno_str[ERRNO_LEN_MAX];
	// handling the server response
	switch (feedback){
		case OP_SUCCESS:
			break;
		case OP_FAILURE:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(UNLOCK_FILE,default_flags,default_N,file_path,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(UNLOCK_FILE,default_flags,default_N,file_path,err_str,err);
			}
         strerror_r(err, err_str, REQ_LEN_MAX);
         PRINT_IF(verbose_mode, "%s-> %s %s with errno = %s.\n", FAILURE,
                  UNLOCK_FILE,pathname,err_str);
         errno = err;
         return -1;
		case OP_EXIT_FATAL:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(UNLOCK_FILE,default_flags,default_N,file_path,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(UNLOCK_FILE,default_flags,default_N,file_path,err_str,err);
			}
         abort_with(UNLOCK_FILE,default_flags,default_N,file_path,err_str,err);
      default:
         break;
   }

   return succeed_with(UNLOCK_FILE,default_flags,default_N,file_path);
}

int closeFile(const char* pathname){
	int err;
	char err_str[ERROR_LEN_MAX];
   strcpy(file_path,pathname);
	if (!pathname || strlen(pathname) > PATH_LEN_MAX){
		err = EINVAL;
      fail_with(CLOSE_FILE,default_flags,default_N,pathname,err_str,err);
	}

	if (fd_socket == -1){
		err = ENOTCONN;
      fail_with(CLOSE_FILE,default_flags,default_N,pathname,err_str,err);
	}

   //close file request to server with a buffer
	char buffer[REQ_LEN_MAX];
	memset(buffer, 0, REQ_LEN_MAX);
	snprintf(buffer, REQ_LEN_MAX, "%d %s", CLOSE, pathname);

   // a write operation can return less than we specified, so we use
   // writen to write the remainder of the data.
	if (writen((long) fd_socket, (void*) buffer, REQ_LEN_MAX) == -1){
		err = errno;
      fail_with(CLOSE_FILE,default_flags,default_N,pathname,err_str,err);
	}
   //reading the response from server
	char feedback_str[OP_LEN_MAX];
	memset(feedback_str, 0, OP_LEN_MAX);
   // read operations may return less than we asked for, therefore
   // we must check that the data has been read fully
	if (readn((long) fd_socket, (void*) feedback_str, OP_LEN_MAX) == -1){
		err = errno;
      fail_with(CLOSE_FILE,default_flags,default_N,pathname,err_str,err);
	}
	int feedback;
	if (sscanf(feedback_str, "%d", &feedback) != 1){
		err = EBADMSG;
      fail_with(CLOSE_FILE,default_flags,default_N,pathname,err_str,err);
	}
	char errno_str[ERRNO_LEN_MAX];
	// handling the response
	switch (feedback){
		case OP_SUCCESS:
         break;
		case OP_FAILURE:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(CLOSE_FILE,default_flags,default_N,pathname,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(CLOSE_FILE,default_flags,default_N,pathname,err_str,err);
			}
         strerror_r(err, err_str, REQ_LEN_MAX);
         PRINT_IF(verbose_mode, "%s-> %s %s with errno = %s.\n", FAILURE,
                  CLOSE_FILE,pathname,err_str);
         errno = err;
         return -1;
		case OP_EXIT_FATAL:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(CLOSE_FILE,default_flags,default_N,pathname,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(CLOSE_FILE,default_flags,default_N,pathname,err_str,err);
			}
         abort_with(CLOSE_FILE,default_flags,default_N,pathname,err_str,err);
      default:
         break;
   }
   return succeed_with(CLOSE_FILE,default_flags,default_N,pathname);
}

int removeFile(const char* pathname){
	int err;
	char err_str[REQ_LEN_MAX];
	if (!pathname || strlen(pathname) > PATH_LEN_MAX){
		err = EINVAL;
      fail_with(REMOVE_FILE,default_flags,default_N,pathname,err_str,err);
	}

	if (fd_socket == -1){
		err = ENOTCONN;
      fail_with(REMOVE_FILE,default_flags,default_N,pathname,err_str,err);
	}

   //remove file request to server sent in a buffer
	char buffer[REQ_LEN_MAX];
	memset(buffer, 0, REQ_LEN_MAX);
	snprintf(buffer, REQ_LEN_MAX, "%d %s", REMOVE, pathname);

   // a write operation can return less than we specified, so we use
   // writen to write the remainder of the data.
	if (writen((long) fd_socket, (void*) buffer, REQ_LEN_MAX) == -1){
		err = errno;
      fail_with(REMOVE_FILE,default_flags,default_N,pathname,err_str,err);
	}
	//reading the response from server
	char feedback_str[OP_LEN_MAX];
	memset(feedback_str, 0, OP_LEN_MAX);
	if (readn((long) fd_socket, (void*) feedback_str, OP_LEN_MAX) == -1) {
		err = errno;
      fail_with(REMOVE_FILE,default_flags,default_N,pathname,err_str,err);
	}
	int feedback;
	if (sscanf(feedback_str, "%d", &feedback) != 1){
		err = EBADMSG;
      fail_with(REMOVE_FILE,default_flags,default_N,pathname,err_str,err);
	}
	char errno_str[ERRNO_LEN_MAX];
   //handling the response from server
	switch (feedback){
		case OP_SUCCESS:
         break;
		case OP_FAILURE:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(REMOVE_FILE,default_flags,default_N,pathname,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(REMOVE_FILE,default_flags,default_N,pathname,err_str,err);
			}
         strerror_r(err, err_str, REQ_LEN_MAX);
         PRINT_IF(verbose_mode, "%s-> %s %s with errno = %s.\n", FAILURE,
                  REMOVE_FILE,pathname,err_str);
         errno = err;
         return -1;
		case OP_EXIT_FATAL:
			if (readn((long) fd_socket, (void*) errno_str, ERRNO_LEN_MAX) == -1){
				err = errno;
            fail_with(REMOVE_FILE,default_flags,default_N,pathname,err_str,err);
			}
			if (sscanf(errno_str, "%d", &err) != 1){
				err = EBADMSG;
            fail_with(REMOVE_FILE,default_flags,default_N,pathname,err_str,err);
			}
         fail_with(REMOVE_FILE,default_flags,default_N,pathname,err_str,err);
      default:
         break;
	}
   return succeed_with(REMOVE_FILE,default_flags,default_N,pathname);

}