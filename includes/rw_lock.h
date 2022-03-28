/**
 * @brief header file for read-write lock to be used in the file storage cache.
 *
*/

#ifndef _RW_LOCK_H_
#define _RW_LOCK_H_

typedef struct _rw_lock rw_lock_t;

/**
 * @brief creates a read-write lock.
 * @returns read-write lock on success, NULL on failure.
 * @exception errno is set to ERRNOMEM for malloc failure.
*/
rw_lock_t* lock_create();

/**
 * @brief lock for reading.
 * @returns 0 on success, -1 on failure.
 * @param lock must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int lock_for_reading(rw_lock_t* lock);

/**
 * @brief unlock for reading.
 * @returns 0 on success, -1 on failure.
 * @param lock must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int unlock_for_reading(rw_lock_t* lock);

/**
 * @brief lock for writing.
 * @returns 0 on success, -1 on failure.
 * @param lock must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int lock_for_writing(rw_lock_t* lock);

/**
 * @brief unlock for writing.
 * @returns 0 on success, -1 on failure.
 * @param lock must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int unlock_for_writing(rw_lock_t* lock);

/**
 * @brief frees resources allocated for the read-write lock.
*/
void
lock_free(rw_lock_t* lock);

#endif
