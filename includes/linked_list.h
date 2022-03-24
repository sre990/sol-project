/**
 * @brief header file for the doubly linked list data structure.
 *
*/

#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#include <stdlib.h>

typedef struct _linked_list linked_list_t;
typedef struct _node node_t;

/**
 * @brief creates a node with a key-value pair.
 * @returns a new node on success, NULL on failure.
 * @param key must be != NULL.
 * @param key_size must be != 0.
 * @param free_data pointer to function to free allocated resources for the node.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
node_t* node_create(const char* key, size_t key_size, const void* data,
            size_t data_size, void (*free_data) (void*));

/**
 * @brief sets a node successor.
 * @returns 0 on success, -1 on failure.
 * @param node must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int node_set_next(node_t* node_curr, const node_t* node_next);

/**
 * @brief sets a node precedessor.
 * @returns 0 on success, -1 on failure.
 * @param node1 must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int node_set_prev(node_t* node_curr, const node_t* node_prev);

/**
 * @brief gets a node successor.
 * @return a node successor on success, NULL on failure.
 * @param node must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
const node_t* node_get_next(const node_t* node);

/**
 * @brief gets a node precedessor.
 * @returns a node precedessor on success, NULL on failure.
 * @param node must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
const node_t* node_get_prev(const node_t* node);

/**
 * @brief saves a node's key to key_ptr.
 * @returns 0 on success, -1 on failure.
 * @param node must be != NULL.
 * @param key_ptr must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
int node_save_key(const node_t* node, char** key_ptr);

/**
 * @brief saves a node's value to data_ptr.
 * @returns size of saved value on success, 0 on failure.
 * @param node must be != NULL.
 * @param data_ptr must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
size_t node_save_data(const node_t* node, void** data_ptr);

/**
 * @brief gets a node's value.
 * @returns node's value on success, NULL on failure.
 * @param node must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
const void* node_get_value(const node_t* node);

/**
 * @brief removes a node.
 * @node
*/
void node_remove(node_t* node);

/**
 * @brief frees resources allocated for the node.
*/
void node_free(node_t* node);
/**
 * @brief creates a new doubly linked list.
 * @returns a new list on success, NULL on failure.
 * @param free_data pointer to function to free allocated resources for the node.
 * @exception errno is set to ENOMEM for malloc failure.
*/
linked_list_t* list_create(void (*free_data) (void*));

/**
 * @brief gets the first element in a doubly linked list.
 * @param list
 * @returns first element of if non-empty, NULL on empty lists.
*/
const node_t* list_get_first(const linked_list_t* list);

/**
 * @brief gets the last element in a doubly linked list.
 * @param list
 * @returns last element of if non-empty, NULL on empty lists.
*/
const node_t* list_get_last(const linked_list_t* list);

/**
 * @brief gets the number of elements in a doubly linked list.
 * @returns the number of elements in a list.
 * @param list
*/
unsigned long list_get_size(const linked_list_t* list);

/**
 * @brief pushes a new node to the front of the list, given a key-value pair.
 * @returns 0 on success, -1 on failure.
 * @param key must be != NULL.
 * @param key_size must be != 0.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
int list_push_to_front(linked_list_t* list, const char* key,
			size_t key_size, const void* data, size_t data_size);

/**
 * @brief pushes a new node to the back of the list, given a key-value pair.
 * @returns 0 on success, -1 on failure.
 * @param key must be != NULL.
 * @param key_size must be != 0.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
int list_push_to_back(linked_list_t* list, const char* key,
			size_t key_size, const void* data, size_t data_size);

/**
 * @brief pops the first element of the list and saves its key and data to key_ptr and data_ptr resp.
 * @returns size of saved value on success, 0 on failure
 * @param list must be != NULL and != empty.
 * @param key_ptr may be null.
 * @param data_ptr may be null.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
size_t list_pop_from_front(linked_list_t* list, char** key_ptr, void** data_ptr);

/**
 * @brief pops the last element of the list and saves its key and data to key_ptr and data_ptr resp.
 * @returns size of saved value on success, 0 on failure
 * @param list must be != NULL and != empty.
 * @param key_ptr may be null.
 * @param data_ptr may be null.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
size_t list_pop_fron_back(linked_list_t* list, char** key_ptr, void** data_ptr);

/**
 * @brief removes a node with matching key from the list.
 * @returns 0 on success, 1 on not found, -1 on failure.
 * @param list must be != NULL.
 * @param key must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
int list_remove(linked_list_t* list, const char* key);

/**
 * @brief checks if the a key inside the list matches the key given as param.
 * @returns 1 on found, 0 on not found, -1 on failure.
 * @exception errno is set to EINVAL for invalid params.
*/
int list_is_in(const linked_list_t* list, const char* key);

/**
 * @brief saves all keys of a list to a new list.
 * @returns list with saved keys on success, NULL on failure.
 * @param list must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
linked_list_t* list_save_keys(const linked_list_t* list);

/**
 * @brief prints the elements of a doubly linked list and its size.
*/
void list_print(const linked_list_t* list);

/**
 * @brief frees resources allocated for the doubly linked list.
*/
void list_free(linked_list_t* list);

#endif