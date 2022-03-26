/**
 * @brief implementation for the bounded buffer with limited capacity
 *
*/

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "bounded_buffer.h"
#include "linked_list.h"
#include "error_handlers.h"

struct _bounded_buffer{
   size_t capacity;
   linked_list_t* tasks;
   pthread_mutex_t mutex;
   pthread_cond_t full;
   pthread_cond_t empty;
};

bounded_buffer_t* buffer_create(size_t capacity){
   if (capacity == 0){
      errno = EINVAL;
      return NULL;
   }
   int err, errno_cpy;
   pthread_mutex_t peek;
   linked_list_t* tasks = NULL;
   pthread_cond_t full;
   pthread_cond_t empty;
   bounded_buffer_t* new = NULL;
   bool is_full = false, is_empty = false;

   pthread_mutex_init(&peek,NULL);
   err = pthread_cond_init(&full, NULL);
   GOTO_NZ(err, errno_cpy, cleanup);
   is_full = true;
   err = pthread_cond_init(&empty, NULL);
   GOTO_NZ(err, errno_cpy, cleanup);
   is_empty = true;
   tasks = list_create(NULL);
   GOTO_NULL(tasks, errno_cpy, cleanup);
   new = malloc(sizeof(bounded_buffer_t));
   GOTO_NULL(new, errno_cpy, cleanup);

   new->capacity = capacity;
   new->tasks = tasks;
   new->mutex = peek;
   new->full = full;
   new->empty = empty;

   return new;

   cleanup:
   pthread_mutex_destroy(&peek);
   if (is_full) pthread_cond_destroy(&full);
   if (is_empty) pthread_cond_destroy(&full);
   list_free(tasks);
   errno = errno_cpy;
   return NULL;
}

int buffer_enqueue(bounded_buffer_t* buffer, const char* task){
   if (!buffer || !task){
      errno = EINVAL;
      return -1;
   }
   int err;
   //acquire lock over the buffer
   err = pthread_mutex_lock(&(buffer->mutex));
   if (err != 0) return -1;
   //wait until the buffer is not at full capacity
   while (buffer->capacity == list_get_size(buffer->tasks)) {
      pthread_cond_wait(&(buffer->full), &(buffer->mutex));
   }
   //add a new task at the end of the buffer
   err = list_push_to_back(buffer->tasks, task, strlen(task) + 1, NULL, 0);
   if (err != 0) return -1;
   if (list_get_size(buffer->tasks) == 1)
      pthread_cond_broadcast(&(buffer->empty));
   //release lock over the buffer
   err = pthread_mutex_unlock(&(buffer->mutex));
   if (err != 0) return -1;

   return 0;
}

int buffer_dequeue(bounded_buffer_t* buffer, char** data_ptr){
   if (!buffer){
      errno = EINVAL;
      return -1;
   }
   int err;
   char* new = NULL;

   //acquire the lock over the buffer
   err = pthread_mutex_lock(&(buffer->mutex));
   if (err != 0) return -1;
   while (list_get_size(buffer->tasks) == 0) {
      pthread_cond_wait(&(buffer->empty), &(buffer->mutex));
   }
   errno = 0;
   //grab the first task from the front of the buffer
   if (list_pop_from_front(buffer->tasks, &new, NULL) == 0 && errno == ENOMEM) return -1;
   if (list_get_size(buffer->tasks) == (buffer->capacity - 1)) {
      pthread_cond_broadcast(&(buffer->full));
   }
   //release the lock over the buffer
   err = pthread_mutex_unlock(&(buffer->mutex));
   if (err != 0) return -1;

   if (data_ptr) *data_ptr = new;
   return 0;
}

void buffer_free(bounded_buffer_t* buffer){
   if (!buffer) return;
   pthread_mutex_destroy(&(buffer->mutex));
   pthread_cond_destroy(&(buffer->empty));
   pthread_cond_destroy(&(buffer->full));
   list_free(buffer->tasks);
   free(buffer);
}