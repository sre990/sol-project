/**
 * @brief some utility macros for error handling.
 *
*/

#ifndef _ERROR_HANDLERS_H_
#define _ERROR_HANDLERS_H_

#include <errno.h>
#include <stdarg.h>
#include <defines.h>

/**
 * @brief returning fatal if the return value of the function is not equal to 0 and printing a message
 * with details about the error; used mainly by cache routines.
*/
#define CHECK_NZ_RET(var, fun_ret) \
	if (((var) = (fun_ret)) != 0){ \
   fprintf(stderr, "[file %s,@line %d] Fatal error with errno = %d\n", __FILE__, __LINE__, errno); \
   return OP_EXIT_FATAL; \
   }

/**
 * @brief returning fatal if the return value of the function is equal to NULL and printing a message
 * with details about the error; used mainly by cache routines.
*/
#define CHECK_NULL_RET(var, fun_ret) \
	if (((var) = (fun_ret)) == NULL) { \
   fprintf(stderr, "[file %s,@line %d] Fatal error with errno = %d\n", __FILE__, __LINE__, errno); \
   return OP_EXIT_FATAL; \
   }

/**
 * @brief returning fatal if the return value of the function is equal to -1 and printing a message
 * with details about the error; used mainly by cache routines.
*/
#define CHECK_FAIL_RET(var, fun_ret) \
	if (((var) = (fun_ret)) == -1) { \
   fprintf(stderr, "[file %s,@line %d] Fatal error with errno = %d\n", __FILE__, __LINE__, errno); \
   return OP_EXIT_FATAL; \
   }

/**
 * @brief exiting with EXIT_FAILURE if the return value of the function is not equal to var and printing a message
 * with details about the error; used mainly by server routines.
*/
#define CHECK_NEQ_EXIT(var, val, fun_ret, fun) \
	if (((var) = (fun_ret)) != (val)) { \
		fprintf(stderr, "[file %s,@line %d] Fatal error, exiting.\n", __FILE__, __LINE__); \
		perror(#fun); \
		exit(EXIT_FAILURE); \
	}

/**
 * @brief exiting with EXIT_FAILURE if the return value of the function is not equal to 0 and printing a message
 * with details about the error; used mainly by server routines.
*/
#define CHECK_NZ_EXIT(var, fun_ret, fun) \
	if (((var) = (fun_ret)) != 0) { \
		fprintf(stderr, "[file %s,@line %d] Fatal error, exiting.\n", __FILE__, __LINE__); \
		perror(#fun); \
		exit(EXIT_FAILURE); \
	}

/**
 * @brief exiting with EXIT_FAILURE if the return value of the function is  equal to NULL and printing a message
 * with details about the error; used mainly by server routines.
*/
#define CHECK_NULL_EXIT(var, fun_ret, fun) \
	if (((var) = (fun_ret)) == NULL) { \
		fprintf(stderr, "[file %s,@line %d] Fatal error, exiting.\n", __FILE__, __LINE__); \
		perror(#fun); \
		exit(EXIT_FAILURE); \
	}

/**
 * @brief exiting with EXIT_FAILURE if the return value of the function is equal to -1 and printing a message
 * with details about the error; used mainly by server routines.
*/
#define CHECK_FAIL_EXIT(var, fun_ret, fun) \
	if (((var) = (fun_ret)) == -1) { \
		fprintf(stderr, "[file %s,@line %d] Fatal error, exiting.\n", __FILE__, __LINE__); \
		perror(#fun); \
		exit(EXIT_FAILURE); \
	}

/**
 * @brief goto label if var is equal to NULL, errno will be stored in a variable.
*/
#define GOTO_NULL(var, errno_cpy, label) \
	if ((var) == NULL) { \
		(errno_cpy) = errno; \
		goto label; \
	}


/**
 * @brief goto label if var is not equal to 0, errno will be stored in a variable.
*/
#define GOTO_NZ(var, errno_cpy, label) \
	if ((var) != 0) { \
		(errno_cpy) = errno; \
		goto label; \
	}

#endif
