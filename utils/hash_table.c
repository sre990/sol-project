/**
 * @brief implementation for the hashtable data structure used for the file storage cache
 *
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "hash_table.h"
#include "linked_list.h"

/**
 * @file icl_hash.c
 * Dependency free hash table implementation.
 * @author Jakub Kurzak
 *
 * $Id: icl_hash.c 2838 2011-11-22 04:25:02Z mfaverge $ 
 * $UTK_Copyright: $ 
 *
 * A simple string hash.
 *
 * An adaptation of Peter Weinberger's (PJW) generic hashing
 * algorithm based on Allen Holub's version. Accepts a pointer
 * to a datum to be hashed and returns an unsigned integer.
 * From: Keith Seymour's proxy library code
 *
 * @param[in] key -- the string to be hashed
 *
 * @returns the hash index
 */
#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))
static size_t table_hash_fun(const void* key){
   char* datum = (char*)key;
   size_t hash_value, i;

   if (!datum) return 0;

   for (hash_value = 0; *datum; ++datum) {
      hash_value = (hash_value << ONE_EIGHTH) + *datum;
      if ((i = hash_value & HIGH_BITS) != 0)
         hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
   }
   return (hash_value);
}


/**
 * ------------------------------------------------------------
*/
struct _hash_table{
	size_t bucket_num;
	linked_list_t** buckets;
	size_t (*hash_fun) (const void*);
	int (*hash_cmp) (const void*, const void*);
   //pointer to function for deallocating resources
	void (*free_data) (void*);
};

/**
 * @brief string comparison function.
*/
static int table_cmp(const void* a, const void* b){
	return strcmp((char*) a, (char*) b);
}

hash_table_t* table_create(size_t bucket_num, size_t (*hash_fun) (const void*),
		int (*hash_cmp) (const void*, const void*), void (*free_data) (void*)){
	//allocating space for the whole table and for the buckets
   hash_table_t* table =  malloc(sizeof(hash_table_t));
	if (!table){
		errno = ENOMEM;
		return NULL;
	}
	table->bucket_num = bucket_num;
	table->buckets =  malloc(sizeof(linked_list_t*) * bucket_num);
	if (!(table->buckets)){
		errno = ENOMEM;
		free(table);
		return NULL;
	}

	size_t i;

   //create empty buckets (doubly linked lists)
	for (i = 0; i < bucket_num; i++){
      //check if there is a custom function for
      //deallocating resources inside the table,
      //otherwise use the normal free function
		if (!free_data)
			(table->buckets)[i] = list_create(free);
		else
			(table->buckets)[i] = list_create(free_data);
      //creation of bucket failed, deallocate resources
		if (!((table->buckets)[i])){
			size_t j = 0;
			while (j != i){
				list_free((table->buckets)[j]);
				j++;
			}
			free(table->buckets);
			free(table);
			errno = ENOMEM;
			return NULL;
		}
	}
   //successful creation of a hash table structure
	table->hash_fun = ((!hash_fun) ? (table_hash_fun) : (hash_fun));
	table->hash_cmp = ((!hash_cmp) ? (table_cmp) : (hash_cmp));
	table->free_data = ((!free_data) ? (free) : (free_data));
	return table;
}

int table_insert(hash_table_t* table, const void* key,
		size_t key_size, const void* data, size_t data_size){
	if (!table || !key || key_size == 0){
		errno = EINVAL;
		return -1;
	}
	int err;
	const node_t* curr;
	char* curr_key = NULL;
   size_t hash = (*table->hash_fun)(key) % table->bucket_num;

   for (curr = list_get_first((table->buckets)[hash]); curr; curr = node_get_next(curr)){
		err = node_save_key(curr, &curr_key);
      //anerror has occurred
		if (err != 0) return -1;
      //the key is already present
		if (table->hash_cmp(key, curr_key) == 0){
			free(curr_key);
			return 0;
		}
		free(curr_key);
	}
   // add the new node at the end of the bucket
	err = list_push_to_back((table->buckets)[hash], key, key_size, data, data_size);
	if (err != 0) return -1;
	return 1;
}

int table_is_in(const hash_table_t* table, const void* key){
	if (!table || !key){
		errno = EINVAL;
		return -1;
	}
   size_t hash = (*table->hash_fun)(key) % table->bucket_num;

	const node_t* curr;
	char* curr_key;
	int err;
	for (curr = list_get_first((table->buckets)[hash]); curr; curr = node_get_next(curr)){
		err = node_save_key(curr, &curr_key);
		//error
      if (err != 0) return -1;
      //found
		if (table->hash_cmp(key, curr_key) == 0){
			free(curr_key);
			return 1;
		}
		free(curr_key);
	}
   //not found
	return 0;
}

const void* table_get_value(const hash_table_t* table, const void* key){
	if (!table || !key){
		errno = EINVAL;
		return NULL;
	}
	size_t hash = table->hash_fun((void*) key) % table->bucket_num;
	const node_t* curr;
	char* curr_key;
	int err;
	const void*  val;
	for (curr = list_get_first((table->buckets)[hash]); curr; curr = node_get_next(curr)){
		err = node_save_key(curr, &curr_key);
      //an error has occurred
		if (err != 0) return NULL;
      //found, return it
		if (table->hash_cmp(key, curr_key) == 0){
			 val = node_get_value(curr);
			free(curr_key);
			return  val;
		}
		free(curr_key);
	}
   //not found
	errno = ENOENT;
	return NULL;
}

int table_remove(hash_table_t* table, const void* key){
	if (!table || !key){
		errno = EINVAL;
		return -1;
	}
   size_t hash = (*table->hash_fun)(key) % table->bucket_num;
	return list_remove((table->buckets)[hash], key);
}

void table_free(hash_table_t* table){
	if (table){
		for (size_t i = 0; i < table->bucket_num; i++){
			list_free((table->buckets)[i]);
		}
		free(table->buckets);
		free(table);
	}
}

