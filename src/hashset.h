#ifndef _HASHSET_H_
#define _HASHSET_H_


#include <stdlib.h>


/*
 * A hashset stores data with certain size.
 */
typedef struct s_hashset *hashset_p;


/*
 * Create hashset instance.
 *
 * NOTE: the hashset created by this function MUST be freed by the caller with hashset_destroy.
 */
hashset_p hashset_create(size_t init_size, unsigned int (*hash)(const void *item, size_t bytes),
		int (*cmp)(const void *item1, const void *item2, size_t bytes));


/*
 * Destroy hashset instance.
 */
void hashset_destroy(hashset_p set);


/*
 * Get current size of the hashset.
 */
size_t hashset_size(hashset_p set);


/*
 * Add item into the hashset.
 *
 * Returns: 0 if succeed;
 *         -1 if the item already in the set;
 *         -2 if memory error.
 */
int hashset_add(hashset_p set, const void *item, size_t bytes);


/*
 * Remove item from the hashset.
 *
 * Returns: 0 if succeed;
 *         -1 if the item doesn't exist.
 */
int hashset_remove(hashset_p set, const void *item, size_t bytes);


/*
 * Remove all the items.
 */
void hashset_remove_all(hashset_p set);


/*
 * Check whether the item is contained in the hashset.
 */
int hashset_contains(hashset_p set, const void *item, size_t bytes);


/*
 * Pop an arbitrary item from the set to *item.
 *
 * Returns: 0 if succeed;
 *         -1 if the set is empty.
 */
int hashset_pop(hashset_p set, void *item, size_t bytes);


/*
 * Put all the items into the list.
 *
 * NOTE: the list MUST have enough space as hashset_size(set).
 */
void hashset_to_list(hashset_p set, void *list, size_t bytes);


#endif /* _HASHSET_H_ */

