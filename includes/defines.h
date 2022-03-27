/**
 * @brief some utility defines.
 *
*/

#ifndef _DEFINES_H_
#define _DEFINES_H_

#define MBYTE 0.000001f

// string representation of all possible operations
#define OPEN_CONN "openConnection"
#define CLOSE_CONN "closeConnection"
#define OPEN_FILE "openFile"
#define CLOSE_FILE "closeFile"
#define READ_FILE "readFile"
#define READ_N_FILES "readNFiles"
#define WRITE_FILE "writeFile"
#define APPEND_TO_FILE "appendToFile"
#define LOCK_FILE "lockFile"
#define UNLOCK_FILE "unlockFile"
#define REMOVE_FILE "removeFile"

//string representation of success, failure and
//fatal error
#define SUCCESS "SUCCESS"
#define FAILURE "FAILURE"
#define EXIT_FATAL "EXIT_FATAL"

//setting mask with flag and resetting it
#define SET_MASK(mask, flag) mask |= flag
#define RESET_MASK(mask) mask = 0

#define O_CREATE 1
#define O_LOCK 2
//check if O_CREATE and O_LOCK flags are toggled resp.
#define O_CREATE_TGL(mask) (mask & 1)
#define O_LOCK_TGL(mask) ((mask >> 1) & 1)

//save file read
#define SAVE 1
#define DISCARD 0
// codes for outcome of operations
#define OP_EXIT_FATAL -1
#define OP_SUCCESS 0
#define OP_FAILURE 1

#define BUF_LEN_MAX 512
#define ERRNO_LEN_MAX 4 // used for errno strings
#define PATH_LEN_MAX 108 // max length of paths(UNIX standard)
#define SIZE_LEN 32
#define REQ_LEN_MAX 2048
#define ERROR_LEN_MAX 128 // max length for errno description string
#define OP_LEN_MAX 2

// returns the max between two numbers
#define MAX(a, b) ((a >= b) ? (a) : (b))

//prints to stdout if flag is true
#define PRINT_IF(flag, ...) \
	if (flag) fprintf(stdout, __VA_ARGS__);

//server defines
#define SHUTDOWN_WORKER 0
#define PIPE_LEN_MAX 10
#define TASK_LEN_MAX 32
//client defines
#define CMD_LEN_MAX 2
#define NAME_LEN_MAX 128
#define ARG_LEN_MAX 2048
//checking command line options permitted by client
#define CHECK_OPT(character) \
	(character == 'h' || character == 'f' || character == 'w' || \
	character == 'W' || character == 'd' || character == 'D' || \
	character == 'r' || character == 'R' || character == 't' || \
	character == 'l' || character == 'u' || character == 'c' || \
	character == 'p')

//helper message to be displayed by the client
#define H_USAGE \
"Client accepts these command line arguments:\n"\
"-h : prints this message.\n"\
"-f <filename> : connects to given socket.\n"\
"-w <dirname>[,n=0] : sends to server up to n files in given directory and its subdirectories.\n"\
"-W <file1>[,file2] : sends to server given files.\n"\
"-D <dirname> : specifies the folder where evicted files are to be sent to.\n"\
"-r <file1>[,file2] : reads given files from server.\n"\
"-R [n=0] : reads at least n files from server (if unspecified, it reads every file).\n"\
"-d <dirname> : specifies the folder where read files are to be stored in.\n"\
"-t <time> : specifies time to wait between requests.\n"\
"-l <file1>[,file2] : requests lock over given files.\n"\
"-u <file1>[,file2] : releases lock over given files.\n"\
"-c <file1>[,file2] : requests server to remove given files.\n"\
"-p : enables output to stdout.\n"

//used for logging purposes
#define LOG_EVENT(...) \
do { \
	if (pthread_mutex_lock(&log_mutex) != 0) { perror("pthread_mutex_lock"); exit(1); } \
	fprintf(log_file, __VA_ARGS__); \
	if (pthread_mutex_unlock(&log_mutex) != 0) { perror("pthread_mutex_unlock"); exit(1); } \
} while(0);

// enumerates all possible operations
typedef enum _ops{
	OPEN,
	CLOSE,
	READ,
	WRITE,
	APPEND,
	READ_N,
	LOCK,
	UNLOCK,
	REMOVE,
	SHUTDOWN
} ops_t;

// enumerates all possible replacement policies
typedef enum _policy{
	FIFO,
	LRU,
   LFU
} policy_t;


#endif