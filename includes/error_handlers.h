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
 * @brief returning fatal if the return value of the function is not equal to var and printing a message
 * with details about the error; used mainly by cache routines.
*/
#define CHECK_NEQ_RET(var, val, fun_ret) \
	if ((var = fun_ret) != val){ \
   fprintf(stderr, "[file %s,@line %d] Fatal error with errno = %d\n", __FILE__, __LINE__, errno); \
   return OP_EXIT_FATAL; \
   }

/**
 * @brief returning fatal if the return value of the function is equal to var and printing a message
 * with details about the error; used mainly by cache routines.
*/
#define CHECK_EQ_RET(var, val, fun_ret) \
	if ((var = fun_ret) == val) { \
   fprintf(stderr, "[file %s,@line %d] Fatal error with errno = %d\n", __FILE__, __LINE__, errno); \
   return OP_EXIT_FATAL; \
   }

/**
 * @brief exiting with EXIT_FAILURE if the return value of the function is not equal to var and printing a message
 * with details about the error; used mainly by server routines.
*/
#define CHECK_NEQ_EXIT(var, val, fun_ret, fun) \
	if ((var = fun_ret) != val) { \
		fprintf(stderr, "[file %s,@line %d] Fatal error, exiting.\n", __FILE__, __LINE__); \
		perror(#fun); \
		exit(EXIT_FAILURE); \
	}

/**
 * @brief exiting with EXIT_FAILURE if the return value of the function is not equal to var and printing a message
 * with details about the error; used mainly by server routines.
*/
#define CHECK_EQ_EXIT(var, val, fun_ret, fun) \
	if ((var = fun_ret) == val) { \
		fprintf(stderr, "[file %s,@line %d] Fatal error, exiting.\n", __FILE__, __LINE__); \
		perror(#fun); \
		exit(EXIT_FAILURE); \
	}
/**
 * @brief goto label if var is not equal to value, errno will be stored in a variable.
*/
#define GOTO_NEQ(var, val, errno_cpy, label) \
	if (var != val) { \
		errno_cpy = errno; \
		goto label; \
	}

/**
 * @brief goto label if var is equal to value, errno will be stored in a variable.
*/
#define GOTO_EQ(var, val, errno_cpy, label) \
	if (var == val) { \
		errno_cpy = errno; \
		goto label; \
	}

#endif