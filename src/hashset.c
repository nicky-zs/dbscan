#include "hashset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DEFAULT_CAPACITY 1024


/*
 * Hashset storing unit.
 */
typedef struct s_slot
{
	struct s_slot *next;
	
	/*
	 * This data can be anything.
	 * But it's the caller's responsibility to make sure that all the data
	 * object are of the same size, which can avoid a lot of problems.
	 */
	char data[];
}
*slot_p;


/*
 * Head of the slot linked list.
 */
typedef struct s_slotlist
{
	slot_p list;

	/*
	 * The non-NULL slotlist.
	 */
	struct s_slotlist *pre; // non-allocated pointer
	struct s_slotlist *next; // non-allocated pointer
}
slotlist_t, *slotlist_p;


typedef struct s_hashset
{
	slotlist_t *slotlists;
	size_t slotlists_n;

	/* how many elements in this set */
	size_t size;

	/* pointer to the non null slot list */
	slotlist_p non_null; // non-allocated pointer

	unsigned int (*hash)(const void *item, size_t bytes);
	int (*cmp)(const void *item1, const void *item2, size_t bytes);
}
hashset_t;


static unsigned int default_hash(const void *item, size_t bytes)
{
	const unsigned char *buffer = (const unsigned char *) item;
	unsigned int s1 = 1;
	unsigned int s2 = 0;
	unsigned int i;

	for (i = 0; i < bytes; ++i) {
		s1 = (s1 + buffer[i]) % 65521;
		s2 = (s2 + s1) % 65521;
	}
	return (s2 << 16) | s1;
}


#define HASH(set, item, bytes) ((set)->hash ? (set)->hash : default_hash)((item), (bytes))

#define CMP(set, item1, item2, bytes) ((set)->cmp ? (set)->cmp : memcmp)((item1), (item2), (bytes))


hashset_p hashset_create(size_t init_size, unsigned int (*hash)(const void *item, size_t bytes),
		int (*cmp)(const void *item1, const void *item2, size_t bytes))
{
	hashset_p set = NULL;
	slotlist_t *slotlists = NULL;

	if (init_size <= DEFAULT_CAPACITY) {
		init_size = DEFAULT_CAPACITY;
	}

	set = (hashset_p) malloc(sizeof(hashset_t));
	slotlists = (slotlist_t *) calloc(init_size, sizeof(slotlist_t));
	if (!set || !slotlists) {
		free(set);
		free(slotlists);
		return NULL;
	}

	set->slotlists = slotlists;
	set->slotlists_n = init_size;
	set->non_null = NULL;
	set->size = 0;
	set->hash = hash;
	set->cmp = cmp;

	return set;
}


void hashset_destroy(hashset_p set)
{
	if (set) {
		void hashset_remove_all(hashset_p set);

		hashset_remove_all(set);

		free(set->slotlists);
		set->slotlists = NULL;

		free(set);
	}
}


size_t hashset_size(hashset_p set)
{
	return set->size;
}


int hashset_add(hashset_p set, const void *item, size_t bytes)
{
	slot_p slot = NULL, new_slot = NULL;

	slotlist_p slotlist = &set->slotlists[HASH(set, item, bytes) % set->slotlists_n];
	slot_p *pslot = &slotlist->list;

	int is_new = !(slot = *pslot);

	while (slot) {
		if (!CMP(set, &slot->data, item, bytes)) {
			return -1;
		}
		pslot = &slot->next;
		slot = *pslot;
	}

	*pslot = new_slot = (slot_p) malloc(bytes + sizeof(slot_p));
	if (!new_slot) {
		return -2;
	}
	new_slot->next = NULL;

	memcpy(&new_slot->data, item, bytes);

	if (!set->non_null) {
		set->non_null = slotlist;
	} else if (is_new) {
		set->non_null->pre = slotlist;
		slotlist->next = set->non_null;
		set->non_null = slotlist;
	}

	++set->size;

	return 0;
}


int hashset_remove(hashset_p set, const void *item, size_t bytes)
{
	slot_p pre_slot = NULL, slot = NULL;

	slotlist_p slotlist = &set->slotlists[HASH(set, item, bytes) % set->slotlists_n];
	slot_p *pslot = &slotlist->list;

	pre_slot = *pslot;
	if (!pre_slot) {
		return -1;
	}

	if (!CMP(set, &pre_slot->data, item, bytes)) {
		*pslot = pre_slot->next;
		free(pre_slot);

		if (!slotlist->list) {
			if (slotlist->pre) {
				slotlist->pre->next = slotlist->next;
			} else {
				set->non_null = slotlist->next;
			}
			if (slotlist->next) {
				slotlist->next->pre = slotlist->pre;
			}
		}

		--set->size;
		return 0;
	}

	while ((slot = pre_slot->next)) {
		if (!CMP(set, &slot->data, item, bytes)) {
			pre_slot->next = slot->next;
			free(slot);

			if (!slotlist->list) {
				if (slotlist->pre) {
					slotlist->pre->next = slotlist->next;
				} else {
					set->non_null = slotlist->next;
				}
				if (slotlist->next) {
					slotlist->next->pre = slotlist->pre;
				}
			}

			--set->size;
			return 0;
		}
		pre_slot = slot;
	}

	return -1;
}


void hashset_remove_all(hashset_p set)
{
	slotlist_p slotlist = set->non_null;

	while (slotlist) {
		slotlist_p next = slotlist->next;

		slot_p slot = slotlist->list;
		while (slot) {
			slot_p next = slot->next;
			free(slot);
			slot = next;
		}

		slotlist->list = NULL;
		slotlist->pre = NULL;
		slotlist->next = NULL;
		
		slotlist = next;
	}

	set->non_null = NULL;
	set->size = 0;
}


int hashset_contains(hashset_p set, const void *item, size_t bytes)
{
	slot_p slot = set->slotlists[HASH(set, item, bytes) % set->slotlists_n].list;

	while (slot) {
		if (!CMP(set, &slot->data, item, bytes)) {
			return 1;
		}
		slot = slot->next;
	}
	return 0;
}


int hashset_pop(hashset_p set, void *item, size_t bytes)
{
	slot_p slot;
	slotlist_p slotlist = set->non_null;

	if (!slotlist) {
		/* empty hashset */
		return -1;
	}

	slot = slotlist->list;
	if (item) {
		memcpy(item, &slot->data, bytes);
	}
	slotlist->list = slot->next;
	free(slot);

	if (!slotlist->list) {
		if (slotlist->pre) {
			slotlist->pre->next = slotlist->next;
		} else {
			set->non_null = slotlist->next;
		}
		if (slotlist->next) {
			slotlist->next->pre = slotlist->pre;
		}
		slotlist->pre = NULL;
		slotlist->next = NULL;
	}

	--set->size;
	return 0;
}


void hashset_to_list(hashset_p set, void *list, size_t bytes)
{
	unsigned int i = 0;
	slotlist_p slotlist = set->non_null;

	while (slotlist) {
		slot_p slot = slotlist->list;

		while (slot) {
			memcpy(list + i, &slot->data, bytes);
			i += bytes;
			slot = slot->next;
		}
		slotlist = slotlist->next;
	}
}

