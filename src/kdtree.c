#include "kdtree.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "array.h"
#include "hashset.h"


/*
 * Node in kdtree.
 */
typedef struct s_kdnode
{
	/*
	 * Stores the pointer of the point.
	 *
	 * So, once the point is added into the tree,
	 * it's better NOT to change it any more.
	 */
	point_p point;

	struct s_kdnode *parent;
	struct s_kdnode *child[2];
}
kdnode_t, *kdnode_p;


#define L(pnode) ((pnode)->child[0])

#define R(pnode) ((pnode)->child[1])


/*
 * Data structure for kdtree.
 *
 * refs: Multidimensional Binary Search Trees Used for Associative Searching
 *       Jon Louis Bentley
 *
 *       An Algorithm for Finding Best Matches in Logarithmic Expected Time
 *       Jerome H. Friedman, Jon Louis Bentley, Raphael Ari Finkel
 */
typedef struct s_kdtree
{
	kdnode_p root;
	rect_p rect;
	size_t size;
}
kdtree_t;


#define SWAP(a, b, type)\
{\
	type temp = (a);\
	(a) = (b);\
	(b) = temp;\
}


void kdnode_init(kdnode_p node, point_p point)
{
	node->point = point;
	node->parent = NULL;
	L(node) = NULL;
	R(node) = NULL;
}


kdtree_p kdtree_create()
{
	kdtree_p tree = (kdtree_p) malloc(sizeof(kdtree_t));
	if (tree) {
		tree->root = NULL;
		tree->rect = NULL;
		tree->size = 0;
	}
	return tree;
}


static void uniq_and_shuffle_points(point_p *from, point_p *to, size_t *pn)
{
	unsigned int i, j;
	size_t n = *pn;
	time_t t;
	hashset_p set = NULL;

	if (!(set = hashset_create(n, NULL, NULL))) {
		*pn = 0;
		return;
	}
	for (i = j = 0; i < n; ++i) {
		switch (hashset_add(set, from[i], sizeof(point_t))) {
			case -2: // memory alloc error
				hashset_destroy(set);
				*pn = 0;
				return;
			case 0: // succeed
				to[j++] = from[i];
			default: // exists (-1)
				break;
		}
	}
	*pn = n = j;

	if ((t = time(NULL)) != -1) {
		srandom(t);
	}
	for (i = n; i > 0; ) {
		long rand = random() % i--;
		SWAP(to[rand], to[i], point_p);
	}

	hashset_destroy(set);
}


static int partition_points(point_p *points, int xd, int p, int r)
{
	int i, j;
	point_p x = points[r];

	for (i = p - 1, j = p; j < r; ++j) {
		if (points[j]->dim[xd] <= x->dim[xd]) {
			++i;
			SWAP(points[i], points[j], point_p);
		}
	}
	++i;
	SWAP(points[i], points[r], point_p);
	return i;
}


static int select_point(point_p *points, int xd, int p, int r, int i)
{
	int q, k;

	if (p == r) {
		return p;
	}

	q = partition_points(points, xd, p, r);
	k = q - p + 1;
	if (i == k) {
		return q;
	} else if (i < k) {
		return select_point(points, xd, p, q - 1, i);
	} else {
		return select_point(points, xd, q + 1, r, i - k);
	}
}


static int median_point(point_p *points, int xd, int p, int r)
{
	return select_point(points, xd, p, r, (r - p) / 2 + 1);
}


static kdnode_p build_kdtree(point_p *points, int xd, int p, int r, int *error)
{
	int m;
	kdnode_p node = NULL, left = NULL, right = NULL;

	if (p > r || *error) {
		return NULL;
	}

	m = median_point(points, xd, p, r);
	node = (kdnode_p) malloc(sizeof(kdnode_t));
	if (!node) {
		*error = 1;
		return NULL;
	}
	kdnode_init(node, points[m]);
	xd = !xd;

	left = build_kdtree(points, xd, p, m - 1, error);
	if (left) {
		L(node) = left;
		left->parent = node;
	}

	right = build_kdtree(points, xd, m + 1, r, error);
	if (right) {
		R(node) = right;
		right->parent = node;
	}

	return node;
}


kdtree_p kdtree_create_static(point_p *_points, size_t n)
{
	unsigned int i;
	int error = 0;
	kdtree_p tree = (kdtree_p) malloc(sizeof(kdtree_t));
	rect_p rect = (rect_p) malloc(sizeof(rect_t));
	point_p *points = (point_p *) malloc(sizeof(point_p) * n);

	if (!tree || !rect || !points) {
		free(tree);
		free(rect);
		free(points);
		return NULL;
	}

	uniq_and_shuffle_points(_points, points, &n);
	if (!n) {
		free(tree);
		free(rect);
		free(points);
		return NULL;
	}

	rect_init_point(rect, points[0]);
	for (i = 0; i < n; ++i) {
		rect_enlarge_to(rect, points[i]);
	}
	tree->rect = rect;
	tree->size = n;

	tree->root = build_kdtree(points, 1, 0, n - 1, &error);
	if (error) {
		kdtree_destroy(tree);
		free(points);
		return NULL;
	}

	free(points);
	return tree;
}


static void rec_destroy_node(kdnode_p node)
{
	if (node) {
		rec_destroy_node(L(node));
		L(node) = NULL;

		rec_destroy_node(R(node));
		R(node) = NULL;

		free(node);
	}
}


void kdtree_destroy(kdtree_p tree)
{
	if (tree) {
		rec_destroy_node(tree->root);
		tree->root = NULL;

		free(tree->rect);
		tree->rect = NULL;

		free(tree);
	}
}


int kdtree_insert(kdtree_p tree, point_p point)
{
	/* TODO: unimplemented */
	assert(tree);
	assert(point);
	return -1;
}


int kdtree_delete(kdtree_p tree, point_p point)
{
	/* TODO: unimplemented */
	assert(tree);
	assert(point);
	return -1;
}


point_p kdtree_nearest_neighbour(kdtree_p tree, point_p point)
{
	/* TODO: unimplemented */
	assert(tree);
	assert(point);
	return NULL;
}


typedef struct s_point_dist
{
	point_p point;
	double dist;
}
point_dist_t, *point_dist_p;


static void knn(kdnode_p node, point_p point, rect_p rect, double dist, int xd, array_p best)
{
	double d;
	rect_t left_rect = {}, right_rect = {};

	if (!node || rect_min_dist_to(rect, point) > dist) {
		return;
	}

	d = point_dist(node->point, point);
	if (d <= dist) {
		point_dist_p pd = (point_dist_p) malloc(sizeof(point_dist_t));
		pd->point = node->point;
		pd->dist = d;
		array_append(best, pd);
	}

	rect_set_lower(rect, &left_rect, point, xd);
	rect_set_upper(rect, &right_rect, point, xd);

	knn(L(node), point, &left_rect, dist, !xd, best);
	knn(R(node), point, &right_rect, dist, !xd, best);
}


static int cmp(const void *a, const void *b)
{
	double dista = (*(point_dist_p *) a)->dist;
	double distb = (*(point_dist_p *) b)->dist;

	if (dista < distb) {
		return -1;
	} else if (dista > distb) {
		return 1;
	} else {
		return 0;
	}
}


point_p *kdtree_k_nearest_neighbour(kdtree_p tree, point_p point, double thre, size_t *ret_size)
{
	int i, n;
	array_p best = NULL;
	point_dist_p *best_list = NULL;
	point_p *result = NULL;

#define FREEALL()\
	{\
		free(best_list); best_list = NULL;\
		array_destroy(best); best = NULL;\
	}

	*ret_size = 0;
	
	best = array_create(512);
	if (!best) {
		return NULL;
	}

	knn(tree->root, point, tree->rect, thre, 0, best);
	n = array_size(best);

	best_list = (point_dist_p *) malloc(sizeof(point_dist_p) * n);
	if (!best_list) {
		FREEALL();
		return NULL;
	}

	array_to_list(best, (void **) best_list);
	array_destroy(best);
	best = NULL;

	qsort(best_list, n, sizeof(point_dist_p), cmp);

	result = (point_p *) malloc(sizeof(point_p) * n);
	if (!result) {
		FREEALL();
		return NULL;
	}

	for (i = 0; i < n; ++i) {
		result[i] = ((point_dist_p) best_list[i])->point;
		free(best_list[i]);
		best_list[i] = NULL;
	}

	*ret_size = n;

	FREEALL();
	return result;

#undef FREEALL

}


#undef SWAP


