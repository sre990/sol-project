/**
 * @brief some utility defines.
 *
*/

#ifndef _OPS_H_
#define _OPS_H_

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
#define SET_FLAG(mask, flag) mask |= flag
#define RESET_MASK(mask) mask = 0

#define O_CREATE 1
#define O_LOCK 2
//check if O_CREATE and O_LOCK flags are toggled resp.
#define O_CREATE_TGL(mask) (mask & 1)
#define O_LOCK_TGL(mask) ((mask >> 1) & 1)

// codes for outcome of operations
#define OP_EXIT_FATAL -1
#define OP_SUCCESS 0
#define OP_FAILURE 1

#define ERRNO_LEN_MAX 4 // used for errno strings
#define PATH_LEN_MAX 108 // max length of paths(UNIX standard)
#define SIZE_LEN 32
#define REQ_LEN_MAX 2048
#define ERROR_LEN_MAX 128 // max length for errno description string
#define OP_LEN_MAX 2

/**
 * @brief printing to stdout if flag is true.
*/
#define PRINT_IF(flag, ...) \
	if (flag) fprintf(stdout, __VA_ARGS__);


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
	LFU,
	LRU
} policy_t;


// returns the maximum between two
#define MAX(a, b) ((a >= b) ? (a) : (b))

#endif