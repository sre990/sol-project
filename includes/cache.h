/**
 * @brief header file for the file storage cache.
 *
*/

#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdlib.h>

#include <linked_list.h>
#include <defines.h>

typedef struct _cache cache_t;

/**
 * @brief creates a cache of limited size with a certain policy.
 * @returns a cache on success, NULL on failure.
 * @param max_files_no must be != 0.
 * @param max_cache_size must be != 0.
 * @exception errno is set to EINVAL for invalid params.
*/
cache_t* cache_create(size_t max_files_no, size_t max_cache_size, policy_t pol);

/**
 * @brief opening of a file by a client with flags.
 * @returns 0 on success, 1 on failure, -1 on fatal errors.
 * @param cache must be != NULL.
 * @param pathname must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to ENOSPC if the cache is at maximum capacity,
 * to EBADF if the client has the file already open, to EPERM if the O_LOCK is set but the ownership
 * of the lock belongs to another client, to EEXIST if O_CREATE is set but the file already exists, to
 * ENOENT if O_CREATE is not set and the file does not exist already.
*/
int cache_openFile(cache_t* cache, const char* pathname, int flags, int client);

/**
 * @brief reading of a file from server.
 * @returns 0 on success, 1 on failure, -1 on fatal errors.
 * @param cache must be != NULL.
 * @param pathname must be != NULL.
 * @param buf must be != NULL.
 * @param size must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to ENOENT if the file is not present,
 * to EPERM if the file is locked is set but the ownership of the lock belongs to another client,
 * to EACCES if the files has not been opened by the client beforehand.
*/
int cache_readFile(cache_t* cache, const char* pathname, void** buf, size_t* size, int client);

/**
 * @brief reading of n files from server.
 * @returns 0 on success, 1 on failure, -1 on fatal errors.
 * @param cache must be != NULL.
 * @param read_files must be != NULL.
 * @param n == 0 or less files than n are present, all files will be read.
 * @exception errno is set to EINVAL for invalid params.
 * @note opening the file beforehand is not required, if a file is locked by another client the
 * file will not be read.
*/
int cache_readNFiles(cache_t* cache, linked_list_t** read_files, size_t n, int client);

/**
 * @brief writing of files to the server, with eviction of files on capacity misses.
 * @returns 0 on success, 1 on failure, -1 on fatal errors.
 * @param cache must be != NULL.
 * @param pathname must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to EACCES if the client has not writing
 * privileges over the file, to ENOENT if the file is not present, to EFBIG if size of the file
 * exceeds the cache's capacity, to EIDRM if the file to be written was evicted.
*/
int cache_writeFile(cache_t* cache, const char* pathname, size_t length, const char* contents, linked_list_t** evicted, int client);

/**
 * @brief appending of bytes to a file inside the server, with eviction of files on capacity misses.
 * @returns 0 on success, 1 on failure, -1 on fatal errors.
 * @param cache must be != NULL.
 * @param pathname must be != NULL and must be a regular file.
 * @exception errno is set to EINVAL for invalid params, to EPERM if the file is locked is set
 * but the ownership of the lock belongs to another client, to EACCES if the client has not writing
 * privileges over the file, to ENOENT if the file is not present, to EIDRM if the file to be written
 * was evicted, to ENOMEM if malloc has failed.
*/
int cache_appendToFile(cache_t* cache, const char* pathname, void* buf, size_t size, linked_list_t** evicted, int client);

/**
 * @brief locking of a file by a client.
 * @returns 0 on success, 1 on failure, -1 on fatal errors.
 * @param cache must be != NULL.
 * @param pathname must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to EPERM if the file is locked is set
 * but the ownership of the lock belongs to another client, to EACCES if the file is not currently opened
 * by the client, to ENOENT if the file is not present.
*/
int cache_lockFile(cache_t* cache, const char* pathname, int client);

/**
 * @brief unlocking of a file by a client.
 * @returns 0 on success, 1 on failure, -1 on fatal errors.
 * @exception errno is set to EINVAL for invalid params, to EPERM if the file is locked is set
 * but the ownership of the lock belongs to another client, to EACCES if the file is not currently opened
 * by the client, to ENOENT if the file is not present.
*/
int cache_unlockFile(cache_t* cache, const char* pathname, int client);

/**
 * @brief closing of a file by a client.
 * @returns 0 on success, 1 on failure, -1 on fatal errors.
 * @exception errno is set to EINVAL for invalid params, to EPERM if the file is locked is set
 * but the ownership of the lock belongs to another client, to EACCES if the file is not currently opened
 * by the client, to ENOENT if the file is not present.
*/
int cache_closeFile(cache_t* cache, const char* pathname, int client);

/**
 * @brief removing of a file by a client.
 * @returns 0 on success, 1 on failure, -1 on fatal errors.
 * @exception errno is set to EINVAL for invalid params, to EPERM if the file is locked is set
 * but the ownership of the lock belongs to another client, to EACCES if the file is not currently opened
 * by the client, to ENOENT if the file is not present.
*/
int cache_removeFile(cache_t* cache, const char* pathname, int client);

/**
 * @brief Gets maximum amount of files stored.
 * @param cache must be != NULL.
 * @returns max number of files reached by the cache on success, 0 on failure.
 * @exception errno is set to EINVAL for invalid params.
*/
size_t cache_get_files_max(cache_t* cache);

/**
 * @brief Gets maximum total size.
 * @param cache must be != NULL.
 * @returns max size reached by the cache on success, 0 on failure.
 * @exception errno is set to EINVAL for invalid params.
*/
size_t cache_get_size_max(cache_t* cache);

/**
 * @brief printing of a summary of informations about the file storage cache.
 * @param cache
*/
void cache_print(cache_t* cache);

/**
 * @brief frees resources allocated for the file storage cache.
 * @param cache
*/
void cache_free(cache_t* cache);

#endif