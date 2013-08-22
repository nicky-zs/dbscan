#include "array.h"

#include <stdlib.h>


#define MIN_INIT_SIZE 32


typedef struct s_array
{
	/* all the pointers */
	void **items;
	
	/* allocated size */
	size_t n;

	/* current size */
	size_t size;
}
array_t;


array_p array_create(size_t init_size)
{
	array_p array = (array_p) malloc(sizeof(array_t));
	if (array) {
		void **items = NULL;
		
		if (init_size < MIN_INIT_SIZE) {
			init_size = MIN_INIT_SIZE;
		}

		items = (void **) calloc(init_size, sizeof(void *));
		if (!items) {
			free(array);
			return NULL;
		}

		array->items = items;
		array->n = init_size;
		array->size = 0;
	}
	return array;
}


void array_destroy(array_p array)
{
	if (array) {
		free(array->items);
		array->items = NULL;

		free(array);
	}
}


static int ensure_capacity(array_p array)
{
	unsigned int i = 0;
	size_t size = array->size;
	size_t n = array->n, new_n = 0;
	void **items = NULL, **new_items = NULL;

	if (size != n) {
		/* need not enlarge */
		return 0;
	}

	if (n >> (sizeof(size_t) * 8 - 1)) {
		/* too large */
		return -1;
	}

	new_n = n << 1;
	new_items = (void **) calloc(new_n, sizeof(void *));
	if (!new_items) {
		return -1;
	}

	items = array->items;
	for (i = 0; i < size; ++i) {
		new_items[i] = items[i];
	}

	array->items = new_items;
	array->n = new_n;
	free(items);
	return 0;
}


int array_append(array_p array, void *item)
{
	if (ensure_capacity(array)) {
		return -1;
	}

	array->items[array->size++] = item;

	return 0;
}


int array_pop(array_p array, void **item)
{
	size_t size = array->size;
	void **items = array->items;

	if (!size--) {
		return -1;
	}

	if (item) {
		*item = items[size];
	}

	items[size] = NULL;
	array->size = size;

	return 0;
}


int array_at(array_p array, unsigned int index, void **item)
{
	if (index >= array->size) {
		return -1;
	}

	*item = array->items[index];

	return 0;
}


size_t array_size(array_p array)
{
	return array->size;
}


void array_to_list(array_p array, void **list)
{
	unsigned int i;
	size_t size = array->size;
	void **items = array->items;

	for (i = 0; i < size; ++i) {
		list[i] = items[i];
	}
}


