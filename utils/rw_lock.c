/**
 * @brief implementation of the read-write lock to be used for the file storage cache.
 *
*/

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include "rw_lock.h"
#include "error_handlers.h"

struct _rw_lock{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned int readers;
	bool writer;
};

rw_lock_t* lock_create(){
	int err, errno_cpy;
	rw_lock_t* new = NULL;
	pthread_mutex_t peek;
	pthread_cond_t cond;
	bool mutex_set = false;
   bool cond_set = false;

	err = pthread_mutex_init(&peek, NULL);
	GOTO_NZ(err, errno_cpy, cleanup);
	mutex_set = true;
	err = pthread_cond_init(&cond, NULL);
	GOTO_NZ(err, errno_cpy, cleanup);
	cond_set = true;
	new = malloc(sizeof(rw_lock_t));
	GOTO_NULL(new, errno_cpy, cleanup);

	new->cond = cond;
	new->mutex = peek;
	new->writer = false;
	new->readers = 0;
	return new;

	cleanup:
		if (mutex_set) pthread_mutex_destroy(&peek);
		if (cond_set) pthread_cond_destroy(&cond);
		free(new);
		errno = errno_cpy;
		return NULL;
}

int lock_for_reading(rw_lock_t* lock){
	if (!lock){
		errno = EINVAL;
		return -1;
	}
	int err;
   //acquire lock over the structure
	err = pthread_mutex_lock(&(lock->mutex));
	if (err != 0) return -1;
   //read operations can happen concurrently, while write operations require
   //exclusive and writing operations have the priority
	while (lock->writer) {
      //if there is a writer, we wait
		pthread_cond_wait(&(lock->cond), &(lock->mutex));
   }
   //increment number of readers
   lock->readers++;
   //release lock over the structure
	err = pthread_mutex_unlock(&(lock->mutex));
	if (err != 0) return -1;
	else return 0;
}

int unlock_for_reading(rw_lock_t* lock){
	if (!lock){
		errno = EINVAL;
		return -1;
	}
	int err;
   //acquire lock over the structure
	err = pthread_mutex_lock(&(lock->mutex));
	if (err != 0) return -1;
   //decrement number of readers
	lock->readers--;
   //once all readers are finished, wake up everyone
	if (lock->readers == 0) {
      pthread_cond_broadcast(&(lock->cond));
   }
   //release lock over the structure
	err = pthread_mutex_unlock(&(lock->mutex));
	if (err != 0) return -1;
	else return 0;
}

int lock_for_writing(rw_lock_t* lock){
	if (!lock){
		errno = EINVAL;
		return -1;
	}
	int err;
   //acquire lock over the structure
	err = pthread_mutex_lock(&(lock->mutex));
	if (err != 0) return -1;
   //if there is another writer holding the lock, we wait
	while (lock->writer) {
      pthread_cond_wait(&(lock->cond), &(lock->mutex));
   }
   //the writer will hold the lock over the structure next
	lock->writer = true;
   //wait until all readers are finished
	while (lock->readers) {
      pthread_cond_wait(&(lock->cond), &(lock->mutex));
   }
   //release the lock over the structure
	err = pthread_mutex_unlock(&(lock->mutex));
	if (err != 0) return -1;
	else return 0;
}

int unlock_for_writing(rw_lock_t* lock){
	if (!lock){
		errno = EINVAL;
		return -1;
	}
	int err;
   //acquire the lock over the structure
	err = pthread_mutex_lock(&(lock->mutex));
	if (err != 0) return -1;
   //the writer no longer holds the lock
	lock->writer = false;
   //wake up everyone waiting
	pthread_cond_broadcast(&(lock->cond));
   //release the lock over the whole structre
	err = pthread_mutex_unlock(&(lock->mutex));
	if (err != 0) return -1;
	return 0;
}

void lock_free(rw_lock_t* lock){
	if (!lock) return;
	pthread_mutex_destroy(&(lock->mutex));
	pthread_cond_destroy(&(lock->cond));
	free(lock);
}