#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <stdlib.h>


/*
 * An array only stores pointers.
 */
typedef struct s_array *array_p;


/*
 * Create a new array, with suggested initial size.
 *
 * Returns: a pointer to the new created array if succeed;
 *          NULL if failed.
 *
 * NOTE: the array_p MUST be freed by the caller using array_destroy.
 */
array_p array_create(size_t init_size);


/*
 * Release the array.
 */
void array_destroy(array_p array);


/*
 * Insert an item, which is a pointer, into the array.
 *
 * Returns: 0 if succeed;
 *         -1 if failed.
 */
int array_append(array_p array, void *item);


/*
 * Pop out the last item.
 *
 * If item is not NULL, it's assigned to the value popped out.
 *
 * Returns: 0 if succeed;
 *         -1 if the array is empty.
 */
int array_pop(array_p array, void **item);


/*
 * Get the item at the given index.
 *
 * Returns: 0 if succeed;
 *         -1 if array index out of range;
 */
int array_at(array_p array, unsigned int index, void **item);


/*
 * Get the size of the array.
 */
size_t array_size(array_p array);


/*
 * Put all the items in the array to a list.
 *
 * NOTE: the list MUST have enough space.
 */
void array_to_list(array_p array, void **list);


#endif

