/* This kdtree only supports 2D points. */

#ifndef _KDTREE_H
#define _KDTREE_H

#include <stdlib.h>

#include "geo.h"


/*
 * KDTree.
 */
typedef struct s_kdtree *kdtree_p;


/*
 * Create a new kdtree.
 *
 * NOTE: the tree created by this function MUST be destroyed by the caller,
 * using the kdtree_destroy function.
 */
kdtree_p kdtree_create();


/*
 * Create a kdtree statically from a point array.
 *
 * It's better not to insert point into or delete point from the tree later.
 */
kdtree_p kdtree_create_static(point_p *points, size_t n);


/*
 * Destroy the tree, release all memories it uses.
 */
void kdtree_destroy(kdtree_p tree);


/*
 * Insert the point to the tree.
 *
 * Returns: 0 if succeed;
 *         -1 if the point already exists in the tree.
 */
int kdtree_insert(kdtree_p tree, point_p point);


/*
 * Delete the point from the tree.
 *
 * Returns: 0 if succeed;
 *         -1 if the point doesn't exist in the tree.
 */
int kdtree_delete(kdtree_p tree, point_p point);


/*
 * Find the nearest neighbour of the point in the tree.
 *
 * Return NULL if nothing is found.
 * NOTE: the point_p return by this function MUST be freed by the caller!
 */
point_p kdtree_nearest_neighbour(kdtree_p tree, point_p point);


/*
 * Find all the neighbours of the point the distance from which to the point is less or equal to thre.
 *
 * NOTE: the point_p *return by this function MUST be freed by the caller!
 */
point_p *kdtree_k_nearest_neighbour(kdtree_p tree, point_p point, double thre, size_t *ret_size);


#endif /* _KDTREE_H */

