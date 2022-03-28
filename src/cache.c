/**
 * @brief implementation of the file storage cache
 *
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>

#include <hash_table.h>
#include <linked_list.h>
#include <defines.h>
#include <cache.h>
#include <rw_lock.h>
#include <error_handlers.h>


// structure implementing a file to be used by the cache
typedef struct _cache_file{
   char* name;
   void* contents;
   size_t contents_size;
   //the lock to be used on single files
   rw_lock_t* lock;
   //the file descriptor of the owner of the lock over the file
   int locker;
   //the file descriptor of the owner of writing permissions
   int writer;
   //list of file descriptors of the openers of the file
   linked_list_t* openers;

   //to be used for implementing the replacement policy
   time_t last_recen;
   int least_freq;
} cache_file_t;

//structure implementing usage information useful
//for the replacement policy
typedef struct usage{
   char* name;
   time_t last_recen;
   int least_freq;
} usage_t;

//structure implementing the file storage cache
struct _cache{
   //table of files stored in the cache
   hash_table_t* files;
   //list of names of files stored inside the cache
   linked_list_t* names;
   //replacement policy
   policy_t pol;
   //the lock to be used on the whole structure
   rw_lock_t* lock;

   // files number and size and number of evictions reached by the cache
   size_t files_reached;
   size_t size_reached;
   size_t evictions;
   //current file number and size inside the cache
   size_t files_num;
   size_t cache_size;
   //files number and size max capacity of the cache
   size_t files_max;
   size_t size_max;

};

// compare function used in lru for sorting
int cmp_lru(const void* a, const void* b){
   usage_t a1 = *(usage_t*) a;
   usage_t b1 = *(usage_t*) b;
   return difftime(a1.last_recen, b1.last_recen);
}

// compare function used in lfu for sorting
int cmp_lfu(const void* a, const void* b){
   usage_t a1 = *(usage_t*) a;
   usage_t b1 = *(usage_t*) b;
   return (a1.least_freq - b1.least_freq);
}

/**
 * @brief creates a file storage cache file.
 * @param name must be != NULL.
 * @returns file storage cache file on success, NULL on failure.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
static cache_file_t* file_create(const char* name, const void* contents, size_t contents_size){
   if (!name){
      errno = EINVAL;
      return NULL;
   }

   cache_file_t* new = NULL;
   char* new_name = NULL;
   void* new_contents = NULL;
   linked_list_t* new_openers = NULL;
   rw_lock_t* new_lock = NULL;
   int err;

   //for malloc failures save errno and
   //go to label cleanup
   new = malloc(sizeof(cache_file_t));
   GOTO_NULL(new, err, cleanup);
   new_name = malloc(strlen(name) + 1);
   GOTO_NULL(new_name, err, cleanup);
   if (contents_size != 0 && contents){
      new_contents = malloc(contents_size);
      GOTO_NULL(new_contents, err, cleanup);
   }
   new_openers = list_create(free);
   GOTO_NULL(new_openers, err, cleanup);
   new_lock = lock_create();
   GOTO_NULL(new_lock, err, cleanup);

   //if no errors have occurred, initialise a new file
   //with a name and contents.
   strncpy(new_name, name, strlen(name) + 1);
   memcpy(new_contents, contents, contents_size);
   new->name = new_name;
   new->contents = new_contents;
   new->contents_size = contents_size;
   new->lock = new_lock;
   new->openers = new_openers;
   new->locker = 0;
   new->writer = 0;
   new->least_freq = 0;
   new->last_recen = time(NULL);

   //return new created file on success
   return new;

   //free resources update errno and return
   //NULL for failure
   cleanup:
   free(new_name);
   free(new_contents);
   list_free(new_openers);
   lock_free(new_lock);
   free(new);
   errno = err;
   return NULL;
}

/**
 * @brief frees resources allocated for the file storage cache file.
 * @param data to be converted to a cache file.
*/
static void file_free(void* data){
   if (!data) return;
   cache_file_t* file = (cache_file_t*) data;
   list_free(file->openers);
   lock_free(file->lock);
   free(file->contents);
   free(file->name);
   free(file);
}

cache_t* cache_create(size_t files_max, size_t size_max, policy_t pol){
   if (files_max == 0 || size_max == 0){
      errno = EINVAL;
      return NULL;
   }
   int err;
   cache_t*  new = NULL;
   linked_list_t*  new_names = NULL;
   hash_table_t*  new_files = NULL;
   rw_lock_t*  new_lock = NULL;

   //for malloc failures save errno and
   //go to label cleanup
   new_lock = lock_create();
   GOTO_NULL(new_lock, err, cleanup);
   new = malloc(sizeof(cache_t));
   GOTO_NULL(new, err,  cleanup);
   new_names = list_create(free);
   GOTO_NULL(new_names, err,  cleanup);
   new_files = table_create(files_max, NULL, NULL, file_free);
   GOTO_NULL(new_files, err,  cleanup);

   //if no errors have occurred, initialise a new cache
   //with a name and contents.
   new->files =  new_files;
   new->names =  new_names;
   new->pol = pol;
   new->lock =  new_lock;
   new->files_max = files_max;
   new->size_max = size_max;
   new->cache_size = 0;
   new->files_num = 0;
   new->files_reached = 0;
   new->size_reached = 0;
   new->evictions = 0;

   //return new created cache on success
   return  new;

   cleanup:
   err = errno;
   table_free(new_files);
   list_free(new_names);
   lock_free(new_lock);
   free(new);
   errno = err;
   return NULL;
}

/**
 * @brief saves the name of the evicted file (dependent on the replacement policy).
 * @returns 0 on success, -1 on failure.
 * @param cache must be != NULL.
 * @param evicted must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
static int cache_get_evicted(cache_t* cache, char** evicted){
   if (!evicted || !cache){
      errno = EINVAL;
      return -1;
   }
   int err;
   size_t i=0,j=0,res = 0;
   usage_t* info = NULL;
   const cache_file_t* new = NULL;
   linked_list_t* names_cpy = NULL;
   char* name = NULL;
   char* victim = NULL;
   switch (cache->pol){
      //in the FIFO case, the evicted file is the first file in,
      //meaning the last name in the list of names.
      case FIFO:
         errno = 0;
         //removes evicted file from the back of the list
         res = list_pop_from_back(cache->names, evicted, NULL);
         //checking if the value has been correctly popped
         if (res == 0 && errno == ENOMEM) return -1;
         else return 0;
         //in the LRU case, the evicted file is the least frequently used,
         //meaning the first name in the list of names.
      case LRU:
         // it is useful to sort by frequency of usage
         info = malloc(sizeof(usage_t) * cache->files_num);
         GOTO_NULL(info,err,cleanup);
         names_cpy = list_save_keys(cache->names);
         GOTO_NULL(names_cpy,err,cleanup);
         while (list_get_size(names_cpy) != 0){
            errno = 0;
            if ((list_pop_from_front(names_cpy, &name, NULL) == 0) && errno == ENOMEM) goto cleanup;
            info[i].name = malloc(sizeof(char) * (strlen(name) + 1));
            GOTO_NULL(info[i].name,err,cleanup);
            new = (const cache_file_t*) table_get_value(cache->files, name);
            GOTO_NULL(new,err,cleanup);
            //we update usage information for the evicted file
            strcpy(info[i].name, name);
            info[i].last_recen = new->last_recen;
            info[i].least_freq = new->least_freq;
            //we are done with this evicted file, we can deallocate
            // the resources assigned to its name
            free(name);
            i++;
         }
         //sorting by last_recen of use
         qsort((void*) info, i, sizeof(usage_t), cmp_lru);
         victim = malloc(sizeof(char) * (strlen(info[0].name) + 1));
         GOTO_NULL(victim,err,cleanup);
         strcpy(victim, info[0].name);
         // remove the victim's name from the list of names inside the
         // cache and update the evicted file list
         list_remove(cache->names, victim);
         *evicted = victim;

         //DEBUG
         //fprintf(stdout,"--------------------------------------------------------------\n");
         //fprintf(stdout,"EVICTED %s\n", victim);
         //fprintf(stdout,"--------------------------------------------------------------\n");

         //free all resources allocated to the victim
         for(j=0;j != i;j++) free(info[j].name);
         free(info);
         list_free(names_cpy);
         //success
         return 0;
         //in the LFU case, the evicted file is the last recently used,
         //meaning the first name in the list of names
      case LFU:
         // it is useful to sort by usage time
         info = malloc(sizeof(usage_t) * cache->files_num);
         GOTO_NULL(info,err,cleanup);
         names_cpy = list_save_keys(cache->names);
         GOTO_NULL(names_cpy,err,cleanup);
         while (list_get_size(names_cpy) != 0){
            errno = 0;
            if ((list_pop_from_front(names_cpy, &name, NULL) == 0) && errno == ENOMEM) goto cleanup;
            info[i].name = malloc(sizeof(char) * (strlen(name) + 1));
            GOTO_NULL(info[i].name,err,cleanup);
            new = (const cache_file_t*) table_get_value(cache->files, name);
            GOTO_NULL(new, err, cleanup);
            strcpy(info[i].name, name);
            info[i].last_recen = new->last_recen;
            info[i].least_freq = new->least_freq;
            //we are done with this evicted file, we can deallocate
            // the resources assigned to its name
            free(name);
            i++;
         }
         //sorting by least_freq of use
         qsort((void*) info, i, sizeof(usage_t), cmp_lfu);
         victim = malloc(sizeof(char) * (strlen(info[0].name) + 1));
         GOTO_NULL(victim,err,cleanup);
         strcpy(victim, info[0].name);
         // remove the victim's name from the list of names inside the
         // cache and update the evicted file list
         list_remove(cache->names, victim);
         *evicted = victim;

         //DEBUG
         //fprintf(stdout,"--------------------------------------------------------------\n");
         //fprintf(stdout,"EVICTED %s\n", victim);
         //fprintf(stdout,"--------------------------------------------------------------\n");

         //free all resources allocated to the victim
         for(j=0;j != i;j++) free(info[j].name);
         free(info);
         list_free(names_cpy);
         return 0;
   }

   cleanup:
   err = errno;
   for (j=0;j != i;j++) free(info[j].name);
   free(info);
   list_free(names_cpy);
   free(name);
   errno = err;
   return -1;
}

size_t cache_get_files_max(cache_t* cache){
   if (!cache){
      errno = EINVAL;
      return 0;
   }

   size_t num = 0;
   //critical section
   if (lock_for_writing(cache->lock) != 0) return 0;
   cache->files_reached = MAX(cache->files_num, cache->files_reached);
   num = cache->files_reached;
   if (unlock_for_writing(cache->lock) != 0) return 0;

   return num;
}

size_t cache_get_size_max(cache_t* cache){
   if (!cache){
      errno = EINVAL;
      return 0;
   }
   size_t size = 0;
   //critical section
   if (lock_for_writing(cache->lock) != 0) return 0;
   cache->size_reached = MAX(cache->size_reached, cache->cache_size);
   size = cache->size_reached;
   if (unlock_for_writing(cache->lock) != 0) return 0;

   return size;
}

void cache_print(cache_t* cache){
   cache->files_reached = MAX(cache->files_reached, cache->files_num);
   cache->size_reached = MAX(cache->size_reached, cache->cache_size);
   printf("\n------------CACHE SUMMARY INFORMATION------------\n");
   printf("Max number of files stored inside the server: %lu.\n", cache->files_reached);
   printf("Max size reached by the file storage cache: %5f / %5fMB.\n",
          cache->size_reached * MBYTE, cache->size_max * MBYTE);
   printf("The replacement algorithm was executed: %lu time(s).\n", cache->evictions);
   printf("List of files inside the storage after server shutdown:\n");
   list_print(cache->names);
}

void cache_free(cache_t* cache){
   if (!cache) return;
   lock_free(cache->lock);
   list_free(cache->names);
   table_free(cache->files);
   free(cache);
}

int cache_openFile(cache_t* cache, const char* file_path, int flags, int client) {
   if (!cache || !file_path) {
      errno = EINVAL;
      return OP_FAILURE;
   }

   int err, created;
   cache_file_t *file;
   char client_str[SIZE_LEN];
   int len = snprintf(client_str, SIZE_LEN, "%d", client);

   // start of critical section
   bool w_lock = O_CREATE_TGL(flags);
   //acquire lock over the whole structure
   if (!w_lock) {
      CHECK_NZ_RET(err, lock_for_reading(cache->lock));
   } else {
      CHECK_NZ_RET(err, lock_for_writing(cache->lock));
   }
   //unable to check if the file is in the cache, return
   CHECK_FAIL_RET(created, table_is_in(cache->files, (void *) file_path));
   //if the file is present and O_CREATE is toggled, return
   if (created == 1 && w_lock) {
      //release lock over the whole structure
      CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
      errno = EEXIST;
      return OP_FAILURE;
   }else if(created == 1 && !w_lock){
      // file already created and O_CREATED not toggled 
      CHECK_NULL_RET(file, (cache_file_t*) table_get_value(cache->files, (void*) file_path));
      // acquire lock for reading
      CHECK_NZ_RET(err, lock_for_reading(file->lock));
      // unable to check if the client is in the list of openers of the file, return
      CHECK_FAIL_RET(err, list_is_in(file->openers, client_str));
      // the client has already opened the file, return
      if (err == 1){
         //release the lock over the file and the whole structure
         CHECK_NZ_RET(err, unlock_for_reading(file->lock));
         CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
         errno = EBADF;
         return OP_FAILURE;
      }else{
         //the client has not already opened the file, it tries to acquire the lock over the file
         if (O_LOCK_TGL(flags)) {
            //there were no previous locks over the file
            if (file->locker == 0){
               //release lock for reading
               CHECK_NZ_RET(err, unlock_for_reading(file->lock));
               //acquire lock for writing
               CHECK_NZ_RET(err, lock_for_writing(file->lock));
               //if the lock is successful, add the client as the owner of the lock over the file
               file->locker = client;
               //release lock for writing
               CHECK_NZ_RET(err, unlock_for_writing(file->lock));
            }else{
               //a different client already owns the lock over the file, return
               //release reading lock over the file and the whole structure
               CHECK_NZ_RET(err, unlock_for_reading(file->lock));
               CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
               errno = EPERM;
               return OP_FAILURE;
            }
         }else{
            //lock not toggled, release lock over the file for reading
            CHECK_NZ_RET(err, unlock_for_reading(file->lock));
         }
         //acquire lock for writing
         CHECK_NZ_RET(err, lock_for_writing(file->lock));
         // add the client to the list of openers of the file
         CHECK_NZ_RET(err, list_push_to_front(file->openers, client_str, len + 1, NULL, 0));
         // update usage information
         file->last_recen = time(NULL);
         file->least_freq++;
         //release the lock over the file for writing
         CHECK_NZ_RET(err, unlock_for_writing(file->lock));
      }
   }else{
      //file not already created
      //if the file is not already created and O_CREATE is not toggled, return
      if (!w_lock) {
         //release the lock over the whole structure
         CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
         errno = ENOENT;
         return OP_FAILURE;
      }
         //if the maximum capacity has been reached, return
      else if (cache->files_num == cache->files_max){
         //release the lock over the whole structure
         CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
         errno = ENOSPC;
         return OP_FAILURE;
      }else{
         //file not present, O_CREATE toggled and there is enough space, open new file
         cache->files_num++;
         CHECK_NULL_RET(file, file_create(file_path, NULL, 0));
         //the client requests lock over the newly created file
         if (O_LOCK_TGL(flags)) file->locker = client;
         //the client requests the lock for writing
         if (O_LOCK_TGL(flags) && w_lock) file->writer = client;
         //add the client to the list of names that have the file open and insert the file
         //in the file storage cache
         CHECK_NZ_RET(err, list_push_to_front(file->openers, client_str, len+1, NULL, 0));
         CHECK_FAIL_RET(err, table_insert(cache->files, (void*) file_path, strlen(file_path) + 1,
                                            (void*) file, sizeof(*file)));
         CHECK_FAIL_RET(err, list_push_to_front(cache->names, file_path,
                                                  strlen(file_path) + 1, NULL, 0));
         // file creation successful, deallocate resources
         free(file);
      }
   }
   // release lock over the whole structure
   if (!w_lock){
      CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
   }else{
      CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
   }
   return OP_SUCCESS;
}

int cache_readFile(cache_t* cache, const char* file_path, void** buf, size_t* size, int client){
   if (!cache || !file_path || !buf || !size){
      errno = EINVAL;
      return OP_FAILURE;
   }

   int err, created;
   cache_file_t* file;
   char client_str[SIZE_LEN];
   void* new_contents = NULL;
   size_t new_size = 0;
   *buf = NULL; *size = 0;
   snprintf(client_str, SIZE_LEN, "%d", client);

   //acquire lock for reading over the whole structure
   CHECK_NZ_RET(err, lock_for_reading(cache->lock));
   //unable to check if the file is present in the cache, return
   CHECK_FAIL_RET(created, table_is_in(cache->files, (void*) file_path));
   //there is no file in the cache to be read, return
   if (created == 0) {
      CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
      errno = ENOENT;
      return OP_FAILURE;
   }else{
      //the file is present in the cache
      //retrieve the file to be read from the cache
      CHECK_NULL_RET(file, (cache_file_t*) table_get_value(cache->files, (void*) file_path));
      //acquire lock over the file
      CHECK_NZ_RET(err, lock_for_reading(file->lock));
      //the file lock is owned by another client, return
      if (file->locker != 0 && file->locker != client){
         //release the lock over the file and the whole structure
         CHECK_NZ_RET(err, unlock_for_reading(file->lock));
         CHECK_NZ_RET(err, unlock_for_reading(cache->lock));;
         errno = EPERM;
         return OP_FAILURE;
      }
      //unable to check if the file ha been opened by the client, return
      CHECK_FAIL_RET(err, list_is_in(file->openers, client_str));
      //the file has not previously been opened by the client and therefore
      //it cannot be read, return
      if (err == 0){
         //release lock over the file and the whole structure
         CHECK_NZ_RET(err, unlock_for_reading(file->lock));
         CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
         errno = EACCES;
         return OP_FAILURE;
      }else{
         //the file has been opened by this client
         //the file has no contents to be read, return
         if (file->contents_size == 0 || !file->contents){
            //release the lock over the file and the whole structure
            CHECK_NZ_RET(err, unlock_for_reading(file->lock));
            CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
            return OP_SUCCESS;
         }else{
            //the file has been opened by this client and it is not empty, copy
            //its contents
            new_size = file->contents_size;
            CHECK_NULL_RET(new_contents, malloc(new_size));
            memcpy(new_contents, file->contents, new_size);
            //release lock over the file for reading
            CHECK_NZ_RET(err, unlock_for_reading(file->lock));
            //acquire the lock over the file for writing
            CHECK_NZ_RET(err, lock_for_writing(file->lock));
            //no writing permissions over this file
            file->writer = 0;
            //update usage information
            file->last_recen = time(NULL);
            file->least_freq++;
            //release the lock over the file and the whole structure
            CHECK_NZ_RET(err, unlock_for_writing(file->lock));
            CHECK_NZ_RET(err, unlock_for_reading(cache->lock));

         }
      }
   }
   // copy the contents of the file read to the buffer
   *size = new_size;
   *buf = new_contents;
   return OP_SUCCESS;
}

int cache_readNFiles(cache_t* cache, linked_list_t** read_files, size_t n, int client){
   if (!cache || !read_files){
      errno = EINVAL;
      return OP_FAILURE;
   }

   int err;
   char client_str[SIZE_LEN];
   cache_file_t* file = NULL;
   linked_list_t* names = NULL;
   linked_list_t* new = NULL;
   snprintf(client_str, SIZE_LEN, "%d", client);

   //start of critical section
   //acquire lock over the whole structure
   CHECK_NZ_RET(err, lock_for_reading(cache->lock));

   //there are no files to be read, return
   if (cache->files_num == 0){
      *read_files = NULL;
      //release lock over the whole structure
      CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
      return OP_SUCCESS;
   }
   //save the names of the files stored in the cache in a new list
   CHECK_NULL_RET(names, list_save_keys(cache->names));
   //successful and failed reads counters
   int successful = 0;
   int failed = 0;
   bool read_all_files = false;
   //if n<=0 or less than n files are present, read all files
   if(n<=0 || (cache->files_num<n)) read_all_files = true;
   CHECK_NULL_RET(new, list_create(NULL));
   while((!read_all_files && successful+failed !=n) || (read_all_files && successful+failed != cache->files_num)){
      char* file_path = NULL;
      //get the first file in the list and attempt to acquire the lock over it
      CHECK_FAIL_RET(err, list_pop_from_front(names, &file_path, NULL));
      CHECK_NULL_RET(file, (cache_file_t*) table_get_value(cache->files, (void*) file_path));
      CHECK_NZ_RET(err, lock_for_reading(file->lock));
      //the lock is already owned by another client and the file cannot be read
      if (file->locker != 0 && file->locker != client){
         //release lock over file and deallocate resources for the file
         CHECK_NZ_RET(err, unlock_for_reading(file->lock));
         free(file_path);
         failed++;
      }else if (file->contents_size == 0 || !file->contents){
         // the file is empty
         CHECK_NZ_RET(err, list_push_to_back(new, file_path, strlen(file_path) + 1, NULL, 0));
         //release reading lock and acquire writing lock over file
         CHECK_NZ_RET(err, unlock_for_reading(file->lock));
         CHECK_NZ_RET(err, lock_for_writing(file->lock));
         //no writing permissions over this file
         file->writer = 0;
         //update usage information
         file->last_recen = time(NULL);
         file->least_freq++;
         //release writing lock over the file
         CHECK_NZ_RET(err, unlock_for_writing(file->lock));
         free(file_path);
         failed++;
      }else{
         //file is not empty
         CHECK_NZ_RET(err, list_push_to_back(new, file_path, strlen(file_path) + 1, file->contents,
                                                 file->contents_size + 1));
         //release reading lock and acquire writing lock over file
         CHECK_NZ_RET(err, unlock_for_reading(file->lock));
         CHECK_NZ_RET(err, lock_for_writing(file->lock));
         //no writing permissions over this file
         file->writer = 0;
         //update usage information
         file->last_recen = time(NULL);
         file->least_freq++;
         //release writing lock over the file
         CHECK_NZ_RET(err, unlock_for_writing(file->lock));
         free(file_path);
         successful++;
      }
   }
   list_free(names);
   *read_files = new;
   //release the reading lock over the whole structure
   CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
   return OP_SUCCESS;
}

int cache_writeFile(cache_t* cache, const char* file_path, size_t length, const char* contents,
                    linked_list_t** evictions, int client){
   if (!cache || !file_path){
      errno = EINVAL;
      return OP_FAILURE;
   }

   int err, created;
   bool failed = false;
   char* new_contents = NULL;
   char* evicted = NULL;
   cache_file_t* file = NULL;
   cache_file_t* victim = NULL;
   linked_list_t* new_evictions = NULL;

   // file to be written is too big, return
   if (length > cache->size_max){
      if (evictions) *evictions = new_evictions;
      errno = EFBIG;
      return OP_FAILURE;
   }

   // copy the file contents to a buffer
   if (length != 0){
      CHECK_NULL_RET(new_contents, malloc(sizeof(char) * (length + 1)));
      memcpy(new_contents, contents, length);
      new_contents[length] = '\0';
   }

   // start of critical section
   //acquire lock for writing
   CHECK_NZ_RET(err, lock_for_writing(cache->lock));
   // update cache information
   cache->files_reached = MAX(cache->files_reached, cache->files_num);
   cache->size_reached = MAX(cache->size_reached, cache->cache_size);
   //unable to check if the file is inside the cache
   CHECK_FAIL_RET(created, table_is_in(cache->files, (void*) file_path));
   //if the file is not inside the cache
   if (created == 0){
      free(new_contents);
      //release the lock over the whole structure
      CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
      errno = ENOENT;
      return OP_FAILURE;

   }else{
      //the file is inside the cache
      //get the file to be written to server
      CHECK_NULL_RET(file, (cache_file_t*) table_get_value(cache->files, (void*) file_path));
      //if the client has no writing privileges, return
      if (file->writer != client) {
         if (evictions) *evictions = new_evictions;
         free(new_contents);
         //release lock over whole structure for writing
         CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
         errno = EACCES;
         return OP_FAILURE;
      }
      //there is a capacity miss, a file will be evicted
      if (cache->cache_size + length > cache->size_max){
         cache->evictions++;
         if (evictions){
            CHECK_NULL_RET(new_evictions, list_create(NULL));
         }
         while (!failed){
            if (cache->cache_size + length <= cache->size_max) break;
            CHECK_NZ_RET(err, cache_get_evicted(cache, &evicted));
            //the file was evicted before being written
            if (strcmp(evicted, file_path) == 0) {
               failed = true;
            }
            //update evictions list and remove the evicted file from cache
            CHECK_NULL_RET(victim, (cache_file_t*)
               table_get_value(cache->files, (void*) evicted));
            if (evictions){
               CHECK_NZ_RET(err, list_push_to_front(new_evictions, evicted,
                                                        strlen(evicted) + 1, victim->contents, victim->contents_size));
            }
            cache->cache_size -= victim->contents_size;
            cache->files_num--;
            CHECK_NZ_RET(err, table_remove(cache->files, (void*) evicted));
            free(evicted);
         }
         if (evictions) *evictions = new_evictions;
         //if the file was evicted before being written, return
         if (failed) {
            free(new_contents);
            //release the lock over the whole structure
            CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
            errno = EIDRM;
            return OP_FAILURE;
         }
      }
      //the file will be written to the server
      if (new_contents){
         file->contents_size = length;
         file->contents = (void*) new_contents;
      }
      //no writing permissions over this file
      file->writer = 0;
      cache->cache_size += length;
      //release the lock over the whole structure
      CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
   }
   return OP_SUCCESS;
}

int cache_appendToFile(cache_t* cache, const char* file_path, void* buf, size_t size, linked_list_t** evictions, int client){
   if (!cache || !file_path){
      errno = EINVAL;
      return OP_FAILURE;
   }

   int err;
   int created;
   bool failed = false;
   char* evicted;
   cache_file_t* file;
   linked_list_t* new_evictions = NULL;
   void* new_contents;
   char client_str[SIZE_LEN];
   snprintf(client_str, SIZE_LEN, "%d", client);
   //acquire the lock over the whole structure
   CHECK_NZ_RET(err, lock_for_writing(cache->lock));

   // update cache information
   cache->files_reached = MAX(cache->files_reached, cache->files_num);
   cache->size_reached = MAX(cache->size_reached, cache->cache_size);
   //unable to check if the file is inside the cache, return
   CHECK_FAIL_RET(created, table_is_in(cache->files, (void*) file_path));
   //the file is not inside the cache
   if (created == 0) {
      //release the lock over the whole structure
      CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
      errno = ENOENT;
      return OP_FAILURE;
   }else{
      //the file is inside the cache
      CHECK_NULL_RET(file, (cache_file_t*) table_get_value(cache->files, (void*) file_path));
      //unable to check if the client is one of the openers for the file, return
      CHECK_FAIL_RET(err, list_is_in(file->openers, client_str));
      //the file is not open by this client, return
      if (err == 0) {
         //release the lock over the whole structure
         CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
         errno = EACCES;
         return OP_FAILURE;
      }
      //the lock over the file is owned by another client
      if (file->locker != client && file->locker != 0) {
         //release the lock over the whole structure
         CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
         errno = EPERM;
         return OP_FAILURE;
      }
      //there are no bytes to be written to the file, return with success
      if (size == 0 || !buf) {
         //release the lock over the whole structure
         CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
         return OP_SUCCESS;
      }
      //there is a capacity miss, a file will be evicted
      if (cache->cache_size + size > cache->size_max){
         cache->evictions++;
         if (evictions) CHECK_NULL_RET(new_evictions, list_create(NULL));
         while (!failed){
            if (cache->cache_size + size <= cache->size_max) break;
            //update evicted list
            CHECK_NZ_RET(err, cache_get_evicted(cache, &evicted));
            //the file was evicted before being written to
            if (strcmp(evicted, file_path) == 0) failed = true;
            //update evictions list and remove the evicted file from cache
            CHECK_NULL_RET(file, (cache_file_t*) table_get_value(cache->files, (void*) evicted));
            if (evictions){
               CHECK_NZ_RET(err, list_push_to_front(new_evictions, evicted,
                                                        strlen(evicted) + 1, file->contents, strlen(file->contents) + 1));
            }
            cache->cache_size -= file->contents_size;
            cache->files_num--;
            CHECK_NZ_RET(err, table_remove(cache->files, (void*) evicted));
            free(evicted);
         }
         if (evictions) *evictions = new_evictions;
         //if the file was evicted before being written, return
         if (failed){
            //release the lock over the whole structure
            CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
            errno = EIDRM;
            return OP_FAILURE;
         }
      }
      //the file will be written to server
      new_contents = realloc(file->contents, file->contents_size + size);
      if (!new_contents){
         //release the lock over the whole structure
         CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
         errno = ENOMEM;
         return OP_EXIT_FATAL;
      }
      file->contents = new_contents;
      memcpy(file->contents + file->contents_size, buf, size);
      file->contents_size += size;
      //no writing permission over this file
      file->writer = 0;
      cache->cache_size += size;
      //release the lock over the whole structure
      CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
   }
   return OP_SUCCESS;
}

int cache_lockFile(cache_t* cache, const char* file_path, int client){
   if (!cache || !file_path){
      errno = EINVAL;
      return OP_FAILURE;
   }

   int err, created;
   cache_file_t* file;
   char client_str[SIZE_LEN];
   snprintf(client_str, SIZE_LEN, "%d", client);
   //acquire the reading lock over the whole structure
   CHECK_NZ_RET(err, lock_for_reading(cache->lock));
   //unable to check if the file is in the cache, return
   CHECK_FAIL_RET(created, table_is_in(cache->files, (void*) file_path));

   //the file is not inside the cache
   if (created == 0){
      //release the lock over the whole structure
      CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
      errno = ENOENT;
      return OP_FAILURE;

   }else{
      //the file is inside the cache
      CHECK_NULL_RET(file, (cache_file_t*) table_get_value(cache->files, (void*) file_path));
      //acquire the lock over the file
      CHECK_NZ_RET(err, lock_for_reading(file->lock));
      CHECK_FAIL_RET(err, list_is_in(file->openers, client_str));
      //the file has not been opened by the client
      if (err == 0){
         //release the lock over the file and the whole structure
         CHECK_NZ_RET(err, unlock_for_reading(file->lock));
         CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
         errno = EACCES;
         return OP_FAILURE;
      }else{
         //the file has been opened by the client

         //the client has already locked the file, return
         if (client == file->locker){
            //release the lock over the file and the whole structure
            CHECK_NZ_RET(err, unlock_for_reading(file->lock));
            CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
            return OP_SUCCESS;
         }
         // release the reading lock and acquire the writing lock over the file
         CHECK_NZ_RET(err, unlock_for_reading(file->lock));
         CHECK_NZ_RET(err, lock_for_writing(file->lock));
         //the lock is owned by another client, return
         if (file->locker != 0 && file->locker != client) {
            //release the lock over the file and the whole structure
            CHECK_NZ_RET(err, unlock_for_writing(file->lock));
            CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
            errno = EPERM;
            return OP_FAILURE;
         }
         //the file lock is owned by the client
         file->locker = client;
         //no writing permissions over the file
         file->writer = 0;
         //update usage informations
         file->last_recen = time(NULL);
         file->least_freq++;
         //release the lock over the file and the whole structure
         CHECK_NZ_RET(err, unlock_for_writing(file->lock));
         CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
      }
   }
   return OP_SUCCESS;
}

int cache_unlockFile(cache_t* cache, const char* file_path, int client){
   if (!cache || !file_path){
      errno = EINVAL;
      return OP_FAILURE;
   }

   int err, created;
   cache_file_t* file;
   char client_str[SIZE_LEN];
   snprintf(client_str, SIZE_LEN, "%d", client);
   //acquire the lock over the whole structure
   CHECK_NZ_RET(err, lock_for_reading(cache->lock));
   //unable to check if the file is inside the cache, return
   CHECK_FAIL_RET(created, table_is_in(cache->files, (void*) file_path));
   //the file is not inside the cache
   if (created == 0) {
      //release the lock over the whole structure
      CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
      errno = ENOENT;
      return OP_FAILURE;
   }else{
      //the file is inside the cache
      CHECK_NULL_RET(file, (cache_file_t*) table_get_value(cache->files, (void*) file_path));
      //acquire the lock over the file
      CHECK_NZ_RET(err, lock_for_reading(file->lock));
      //unable to check if the file is opened by the client, return
      CHECK_FAIL_RET(err, list_is_in(file->openers, client_str));
      //the file has not been opened by the client
      if (err == 0) {
         //release the lock over the file and the whole structure
         CHECK_NZ_RET(err, unlock_for_reading(file->lock));
         CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
         errno = EACCES;
         return OP_FAILURE;
      }else{
         //the file is opened by the client
         //the client is not the owner of the lock over the file, return
         if (client != file->locker){
            //release the lock over the file and the whole structure
            CHECK_NZ_RET(err, unlock_for_reading(file->lock));
            CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
            errno = EPERM;
            return OP_FAILURE;
         }
         //release the reading lock and acquire the writing lock over the file
         CHECK_NZ_RET(err, unlock_for_reading(file->lock));
         CHECK_NZ_RET(err, lock_for_writing(file->lock));
         //the client is not the owner of the lock
         file->locker = 0;
         //no writing permission over the file
         file->writer = 0;
         //update usage information
         file->last_recen = time(NULL);
         file->least_freq++;
         //release the lock over the file and the whole structure
         CHECK_NZ_RET(err, unlock_for_writing(file->lock));
         CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
      }
   }
   return OP_SUCCESS;
}

int cache_closeFile(cache_t* cache, const char* file_path, int client){
   if (!cache || !file_path){
      errno = EINVAL;
      return OP_FAILURE;
   }

   int err, created;
   cache_file_t* file;
   char client_str[SIZE_LEN];
   snprintf(client_str, SIZE_LEN, "%d", client);
   //acquire the lock over the whole structure
   CHECK_NZ_RET(err, lock_for_reading(cache->lock));
   //unable to check if the file is in the cache
   CHECK_FAIL_RET(created, table_is_in(cache->files, (void*) file_path));
   //the file is not inside the cache, return
   if (created == 0){
      //release the lock over the file
      CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
      errno = ENOENT;
      return OP_FAILURE;
   }else{
      //the file is inside the cache
      CHECK_NULL_RET(file, (cache_file_t*) table_get_value(cache->files, (void*) file_path));
      //acquire the lock over the file
      CHECK_NZ_RET(err, lock_for_reading(file->lock));
      //unable to check if the client has opened the file, return
      CHECK_FAIL_RET(err, list_is_in(file->openers, client_str));
      //the file has not been opened by the client, return
      if (err == 0) {
         //release the lock over the file and the whole structure
         CHECK_NZ_RET(err, unlock_for_reading((file->lock)));
         CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
         errno = EACCES;
         return OP_FAILURE;
      }else{
         //the file is opened by the client
         //release the reading lock and acquire the writing lock over the file
         CHECK_NZ_RET(err, unlock_for_reading((file->lock)));
         CHECK_NZ_RET(err, lock_for_writing(file->lock));
         //remove the client from the list of owners of the file
         CHECK_NZ_RET(err, list_remove(file->openers, client_str));
         //no writing permissions over the file
         file->writer = 0;
         //update usage information
         file->last_recen = time(NULL);
         file->least_freq++;
         //release the lock over the file
         CHECK_NZ_RET(err, unlock_for_writing(file->lock));
      }
   }
   //release the lock over the whole structure
   CHECK_NZ_RET(err, unlock_for_reading(cache->lock));
   return OP_SUCCESS;
}

int cache_removeFile(cache_t* cache, const char* file_path, int client){
   if (!cache || !file_path){
      errno = EINVAL;
      return OP_FAILURE;
   }
   int err;
   int created;
   cache_file_t* file;
   char client_str[SIZE_LEN];
   snprintf(client_str, SIZE_LEN, "%d", client);
   //acquire the lock over the whole structure
   CHECK_NZ_RET(err, lock_for_writing(cache->lock));
   //update the cache information
   cache->files_reached = MAX(cache->files_reached, cache->files_num);
   cache->size_reached = MAX(cache->size_reached, cache->cache_size);
   //unable to check if the file is in the cache, return
   CHECK_FAIL_RET(created, table_is_in(cache->files, (void*) file_path));
   //the file is not inside the cache
   if (created == 0){
      //release the lock over the whole structure
      CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
      errno = ENOENT;
      return OP_FAILURE;
   }else{
      //the file is inside the cache
      CHECK_NULL_RET(file, (cache_file_t*) table_get_value(cache->files, (void*) file_path));
      //unable to check if the file is opened by the client, return
      CHECK_FAIL_RET(err, list_is_in(file->openers, client_str));
      //the file is not opened by the client, return
      if (err == 0){
         //release the lock over the whole structure
         CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
         errno = EACCES;
         return OP_FAILURE;
      }
      //the lock over the file is not owned by the client, return
      if (file->locker != client){
         //release the lock over the whole structure
         CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
         errno = EPERM;
         return OP_FAILURE;
      }
      //remove the file from the cache
      cache->cache_size -= file->contents_size;
      cache->files_num--;
      //unable to remove due to failure, return
      CHECK_FAIL_RET(err, table_remove(cache->files, (void*) file_path));
      CHECK_FAIL_RET(err, list_remove(cache->names, file_path));
      //release the lock over the whole structure
      CHECK_NZ_RET(err, unlock_for_writing(cache->lock));
   }
   return OP_SUCCESS;
}
