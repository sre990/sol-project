/**
 * @brief header file for the worker handling tasks.
 *
*/

#ifndef _WORKER_H_
#define _WORKER_H_

#include <stdlib.h>
#include <cache.h>
#include <defines.h>


typedef struct _worker worker_t;

/**
 * @brief creates a worker to be used to consume tasks assigne by the server.
 * @return an initialised worker on success, null on failure
 * @param cache file storage cache
 * @param tasks tasks to be handled
 * @param pipe for communication
 * @param file the log file for logging purposes
 * @exception errno is set to EINVAL for invalid params to ENOMEM for malloc failures.
 */
worker_t *worker_create(cache_t *cache, bounded_buffer_t *tasks, int pipe, FILE *file);

/**
 * @brief handles the tasks passed by the server, logs its progress and notifies the server of
 * their completion.
 * @param arg to be cast to a worker_t structure.
 * @returns NULL.
*/
void* do_job(void* arg);

#endif