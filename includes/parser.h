/**
 * @brief header file for the parser used by the server for interpreting the config text file.
 *
*/

#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdlib.h>

#include <defines.h>

typedef struct _parser parser_t;

/**
 * @brief creates a new parser.
 * @returns new parser on success, NULL on failure.
 * @exception errno is set to ENOMEM for malloc failure.
*/
parser_t* parser_create();

/**
 * @brief updates the parser with the informations provided in the config file.
 * @returns 0 on success, -1 on failure.
 * @param parser must be != NULL.
 * @param config_path must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int parser_update(parser_t* parser, const char* config_path);

/**
 * @brief gets the number of workers from the parser.
 * @returns number of workers on success, 0 on failure.
 * @param parser must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
unsigned long parser_get_workers(const parser_t* parser);

/**
 * @brief gets the file storage cache max capacity for files.
 * @returns cache max capacity on success, 0 on failure.
 * @param parser must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
unsigned long parser_get_files(const parser_t* parser);

/**
 * @brief gets the file storage cache max capacity for size.
 * @returns cache max capacity in Mbytes on success, 0 on failure.
 * @param parser must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
unsigned long parser_get_size(const parser_t* parser);

/**
 * @brief saves the log file path in log_path_ptr.
 * @returns length of the log file path on success, 0 on failure.
 *r @param parser must be != NULL.
 * @param log_path_ptr must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
unsigned long parser_get_log_path(const parser_t* parser, char** log_path_ptr);

/**
 * @brief saves the socket file path in socket_path_ptr.
 * @returns length of the socket file path on success, 0 on failure.
 * @param parser must be != NULL.
 * @param socket_path_ptr must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
unsigned long parser_get_sock_path(const parser_t* parser, char** socket_path_ptr);

/**
 * @brief gets the replacement policy to be used on capacity misses.
 * @returns replacement policy on success on success, 0 on failure.
 * @param parser must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
policy_t parser_get_policy(const parser_t* parser);

/**
 * @brief frees resources allocated for the parser.
*/
void parser_free(parser_t* parser);

#endif