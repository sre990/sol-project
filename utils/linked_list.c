/**
 * @brief implementation for the doubly linked list data structure.
 *
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "error_handlers.h"
#include "linked_list.h"


struct _node{
   char* key; 
   unsigned long val_size; 
   void* val;
   struct _node* prev;
   struct _node* next;
   // pointer to function for deallocating resources
   void (*free_data) (void*);
};

node_t* node_create(const char* key, size_t key_size, const void* val,
            size_t val_size, void (*free_data) (void*)){
   if (!key || key_size == 0){
      errno = EINVAL;
      return NULL;
   }
   int err;
   node_t* new = NULL;
   char* new_key = NULL;
   void* new_val = NULL;
   new = malloc(sizeof(node_t));
   GOTO_NULL(new, err, cleanup);
   if (key_size != 0){
      new_key = malloc(key_size + 1);
      GOTO_NULL(new_key, err, cleanup);
      memset(new_key, 0, key_size + 1);
      memcpy(new_key, key, key_size);
   }
   new->key = new_key;
   if (val_size != 0){
      new_val = malloc(val_size);
      GOTO_NULL(new_val, err, cleanup);
      memcpy(new_val, val, val_size);
   }
   new->val = new_val;
   new->val_size = val_size;
   new->next = NULL;
   new->prev = NULL;
   new->free_data =  free_data ? free_data : free;

   return new;

   cleanup:
   err = errno;
   free(new_key);
   free(new_val);
   free(new);
   errno = err;
   return NULL;
}

int node_set_next(node_t* curr, const node_t* next){
   if (!curr){
      errno = EINVAL;
      return -1;
   }
   curr->next = (node_t*) next;
   return 0;
}

int node_set_prev(node_t* curr, const node_t* prev){
   if (!curr){
      errno = EINVAL;
      return -1;
   }
   curr->prev = (node_t*) prev;
   return 0;
}

const node_t* node_get_next(const node_t* node){
   if (!node){
      errno = EINVAL;
      return NULL;
   }
   return node->next;
}

const node_t* node_get_prev(const node_t* node){
   if (!node){
      errno = EINVAL;
      return NULL;
   }
   return node->prev;
}

const void* node_get_value(const node_t* node){
   if (!node){
      errno = EINVAL;
      return NULL;
   }
   return node->val;
}

int node_save_key(const node_t* node, char** key_ptr){
   if (!node || !(node->key) || !key_ptr){
      errno = EINVAL;
      return -1;
   }
   size_t len = strlen(node->key);
   char* new;
   new = malloc(sizeof(char) * (len+1));
   if (!new){
      errno = ENOMEM;
      return -1;
   }
   strncpy(new, node->key, len+1);
   *key_ptr = new;
   return 0;
}

size_t node_save_val(const node_t* node, void** val_ptr){
   if (!node || !val_ptr){
      errno = EINVAL;
      return 0;
   }
   size_t len = node->val_size;
   if (len == 0){
      *val_ptr = NULL;
      return len;
   }
   void* new;
   new = malloc(len+1);
   if (!new){
      errno = ENOMEM;
      return 0;
   }
   memset(new, 0, len+1);
   memcpy(new, node->val, len);
   *val_ptr = new;

   return len;
}

void node_free(node_t* node){
   if (node){
      if (node->prev) node->prev->next = node->next;
      if (node->next) node->next->prev = node->prev;
      free(node->key);
      node->free_data(node->val);
      free(node);
   }
}


struct _linked_list{
	node_t* first;
	node_t* last;
	unsigned long tasks;
   //pointer to function for deallocating resources
	void (*free_data) (void*);
};

linked_list_t* list_create(void (*free_data) (void*)){
	linked_list_t* new = malloc(sizeof(linked_list_t));
	if (!new) return NULL;

	new->first = NULL;
	new->last = NULL;
	new->tasks = 0;
	new->free_data = free_data ? free_data : free;

	return new;
}

const node_t* list_get_first(const linked_list_t* list){
	if (!list) return NULL;
	return list->first;
}

const node_t* list_get_last(const linked_list_t* list){
	if (!list) return NULL;
	return list->last;
}

unsigned long list_get_size(const linked_list_t* list){
	if (!list) return 0;
	return list->tasks;
}

int list_push_to_front(linked_list_t* list, const char* key,
		size_t key_size, const void* val, size_t val_size){
	if (!list || !key || key_size == 0){
		errno = EINVAL;
		return -1;
	}
	node_t* new;
	if ((new = node_create(key, key_size, val, val_size, list->free_data)) == NULL)
		return -1;

	if (list->first == NULL){
      //first element of the list
		list->first = new;
		list->last = new;
 	}else{
		node_set_next(new, list->first);
		node_set_prev(list->first, new);
		list->first = new;
	}
	list->tasks++;

	return 0;
}

int list_push_to_back(linked_list_t* list, const char* key,
		size_t key_size, const void* val, size_t val_size){
	if (!list){
		errno = EINVAL;
		return -1;
	}

	node_t* new;
	if ((new = node_create(key, key_size, val, val_size, list->free_data)) == NULL)
		return -1;

	if (list->first == NULL) {
      //first element of the list
		list->first = new;
		list->last = new;
 	}else{
		node_set_next(list->last, new);
		node_set_prev(new, list->last);
		list->last = new;
	}
	list->tasks++;

	return 0;
}

size_t list_pop_from_front(linked_list_t* list, char** key_ptr, void** val_ptr){
	if (!list || !(list->tasks)){
		errno = EINVAL;
		return 0;
	}
	list->tasks--;
	errno = 0;
	if (key_ptr && (node_save_key(list->first, key_ptr) != 0)){
		if (errno == ENOMEM) return 0;
		else *key_ptr = NULL;
	}
	size_t res = 0;
	errno = 0;
	if (val_ptr && ((res = node_save_val(list->first, val_ptr)) == 0)){
		if (errno == ENOMEM) return 0;
		else *val_ptr = NULL;
	}
	if (list->tasks == 0){
		node_free(list->first);
		list->first = NULL;
		list->last = NULL;
 	}else{
		node_t* new = (node_t*) node_get_next(list->first);
		node_set_prev(new, NULL);
		node_free(list->first);
		list->first = new;
	}
	return res;
}

size_t list_pop_from_back(linked_list_t* list, char** key_ptr, void** val_ptr){
	if (!list || !(list->tasks)) {
		errno = EINVAL;
		return 0;
	}
	list->tasks--;
	errno = 0;
	if (key_ptr && (node_save_key(list->last, key_ptr) != 0)){
		if (errno == ENOMEM) return 0;
		else *key_ptr = NULL;
	}
	errno = 0;
	size_t res = 0;
	if (val_ptr && ((res = node_save_val(list->last, val_ptr)) == 0)){
		if (errno == ENOMEM) return 0;
		else *val_ptr = NULL;
	}
	if (list->tasks == 0){
		node_free(list->first);
		list->first = NULL;
		list->last = NULL;
 	}else{
		node_t* new = (node_t*) node_get_prev(list->last);
		node_set_next(new, NULL);
		node_free(list->last);
		list->last = new;
	}
	return res;
}

int list_remove(linked_list_t* list, const char* key){
	if (!list || !key){
		errno = EINVAL;
		return -1;
	}
	node_t* curr = list->first;
	char* new;
	int err;
	while (curr){
		err = node_save_key(curr, &new);
		if (err == -1) return -1;
		if (strcmp(key, new) != 0){
			free(new);
			curr = (node_t*) node_get_next(curr);
		}else{
			if (!node_get_next(curr)) list->last = (node_t*) node_get_prev(curr);
			if (!node_get_prev(curr)) list->first = (node_t*) node_get_next(curr);
			node_free(curr);
			free(new);
			list->tasks--;
			return 0;
		}
	}
	return 1;
}

int list_is_in(const linked_list_t* list, const char* key){
	if (!list || !key || !list->first) return 0;
	const node_t* curr;
	char* new;
	int err;
	for (curr = list->first; curr != NULL; curr = node_get_next(curr)){
		err = node_save_key(curr, &new);
		if (err != 0) return -1;
		if (strcmp(new, key) != 0) free(new);
		else{
			free(new);
			return 1;
		}
	}
	return 0;
}

linked_list_t* list_save_keys(const linked_list_t* list){
	if (!list || list->tasks == 0){
		errno = EINVAL;
		return NULL;
	}
	linked_list_t* new = list_create(NULL);
	if (!new) return NULL;
	char* key;
	const node_t* curr = list->first;
	int errno_cpy;
	while (curr){
		if (node_save_key(curr, &key) != 0){
			errno_cpy = errno;
			list_free(new);
			errno = errno_cpy;
			return NULL;
		}
		if (list_push_to_back(new, key, strlen(key) + 1, NULL, 0) != 0){
			errno_cpy = errno;
			list_free(new);
			free(new);
			errno = errno_cpy;
			return NULL;
		}
		free(key); key = NULL;
		curr = node_get_next(curr);
	}
	return new;
}

void list_print(const linked_list_t* list){
	if (!list) return;
	fprintf(stdout, "Number of elements after server shutdown: %lu\n", list->tasks);
	const node_t* curr = list->first;
	char* key = NULL;
	while (curr){
		if (node_save_key(curr, &key) != 0) fprintf(stdout, "NULL -> ");
		else fprintf(stdout, "\t%s\n", key);
		curr = node_get_next(curr);
		free(key);
	}
}

void list_free(linked_list_t* list){
	if (!list) return;
	node_t* new;
	node_t* curr = list->first;
	while (curr){
		new = curr;
		curr = (node_t*) node_get_next(curr);
		node_free(new);
	}
	free(list);
}