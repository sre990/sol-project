/**
 * @brief header file for the hash table used in the cache for storing files.
 *
*/

#ifndef _HASH_TABLE_H_
#define _HASH_TABLE_H_

#include <stdlib.h>

#include <linked_list.h>

typedef struct _hash_table hash_table_t;

/**
 * @brief creates a new hash table.
 * @returns an hash table on success, NULL on failure.
 * @param buckets number of buckets
 * @param hash_fun pointer to hashing function
 * @param hash_cmp pointer to comparing function
 * @param free_data pointer to function for deallocating a node's memory.
 * @exception errno is set to ENOMEM if malloc fails.
*/
hash_table_t* table_create(size_t buckets, size_t (*hash_fun) (const void*),
                           int (*hash_cmp) (const void*, const void*), void (*free_data) (void*));

/**
 * @brief inserts a key-value pair into the table
 * @returns 1 on success, 0 on duplicate, -1 on failure.
 * @param key must be != NULL.
 * @param key_size must be != 0.
 * @exception errno is set to EINVAL for invalid params.
*/
int table_insert(hash_table_t* table, const void* key,
                      size_t key_size, const void* data, size_t data_size);

/**
 * @brief checks if a key is inside the hashtable.
 * @returns 1 if found, 0 if not found, -1 on failure.
 * @param table must be != NULL.
 * @param key must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int table_is_in(const hash_table_t* table, const void* key);

/**
 * @brief gets value paired to key inside the hashtable
 * @returns a pointer to the data on success, NULL on failure.
 * @param table must be != NULL.
 * @param key must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to ENOENT if the key
 * is not inside the hashtable.
*/
const void* table_get_value(const hash_table_t* table, const void* key);

/**
 * @brief deletes the node having a certain key.
 * @returns 1 on success, 0 on not found, -1 on failure.
 * @param table must be != NULL.
 * @param key must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int table_remove(hash_table_t* table, const void* key);

/**
 * @brief frees resources allocated for the hash table.
 * @param table
*/
void table_free(hash_table_t* table);

#endif
