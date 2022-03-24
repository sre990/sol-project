/**
 * @brief header file for the bounded buffer used to handle tasks performed by worker threads
 *
*/

#ifndef _BOUNDED_BUFFER_H_
#define _BOUNDED_BUFFER_H_

#include <stdlib.h>

typedef struct _bounded_buffer bounded_buffer_t;

/**
 * @brief creates a bounded buffer of limited capacity.
 * @returns a bounded buffer on success, NULL on failure.
 * @param capacity must be != 0.
 * @exception errno is set to EINVAL for invalid params.
*/
bounded_buffer_t* buffer_create(size_t capacity);

/**
 * @brief enqueues data to bounded buffer.
 * @returns 0 on success, -1 on failure.
 * @param buffer must be != NULL.
 * @param data must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int buffer_enqueue(bounded_buffer_t* buffer, const char* data);

/**
 * @brief dequeues first elemeent from buffer and saves it to data_ptr.
 * @returns 0 on success, -1 on failure.
 * @param buffer must be != NULL.
 * @param data_ptr must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int buffer_dequeue(bounded_buffer_t* buffer, char** data_ptr);

/**
 * @brief frees resources allocated for the bounded buffer.
 * @param buffer.
*/
void buffer_free(bounded_buffer_t* buffer);

#endif