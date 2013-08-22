#include "dbscan.h"

#include <stdlib.h>
#include <string.h>

#include "geo.h"
#include "kdtree.h"
#include "array.h"
#include "hashset.h"
#include "id_gen.h"


void cpoint_init(cpoint_p cpoint, double x, double y)
{
	cpoint->point.x = x;
	cpoint->point.y = y;
	cpoint->cluster_id = 0;
}


/*
 * This structure represents a set of cpoint_t with exactly the same coordinates.
 *
 * It can be used as a cpoint_t.
 */
typedef struct s_cpointset
{
	/* as a cpoint_t */
	cpoint_t cpoint;

	/* pointers to all the cpoints it represents */
	array_p cpoints;
}
cpointset_t, *cpointset_p;


/*
 * Create a pointset representing cpoint.
 */
static cpointset_p cpointset_create(cpoint_p cpoint)
{
	cpointset_p cpointset = (cpointset_p) malloc(sizeof(cpointset_t));
	if (cpointset) {
		cpointset->cpoints = array_create(0);
		if (!cpointset->cpoints) {
			free(cpointset);
			return NULL;
		}
		cpointset->cpoint = *cpoint;
	}
	return cpointset;
}


static void cpointset_destroy(cpointset_p cpointset)
{
	if (cpointset) {
		array_destroy(cpointset->cpoints);
		cpointset->cpoints = NULL;
		free(cpointset);
	}
}


/*
 * Just for uniq test.
 */
static int cmp(const void *p1, const void *p2)
{
	if ((*(point_p *) p1)->x < (*(point_p *) p2)->x) {
		return -1;
	}

	if ((*(point_p *) p1)->x > (*(point_p *) p2)->x) {
		return 1;
	}

	if ((*(point_p *) p1)->y < (*(point_p *) p2)->y) {
		return -1;
	}

	if ((*(point_p *) p1)->y > (*(point_p *) p2)->y) {
		return 1;
	}

	return 0;
}


/*
 * Convert cpoint_p * to pointset_p *.
 *
 * This is to avoid duplicated points in the input, which would cause an incorrect result.
 *
 * NOTE: the returned array, as well as all the elements in the returned array,
 *       MUST be freed and destroyed by the caller!
 */
static cpointset_p *convert_points(cpoint_p *cpoints, size_t size, size_t *ret_size)
{
	unsigned int i, j;

	cpoint_p *temp = (cpoint_p *) malloc(sizeof(cpoint_p) * size);
	cpointset_p *result = (cpointset_p *) calloc(size, sizeof(cpointset_t));

	cpoint_p last = NULL; // non-allocated pointer

	if (!temp || !result) {
		free(result);
		free(temp);
		*ret_size = 0;
		return NULL;
	}

	memcpy(temp, cpoints, sizeof(cpoint_p) * size);

	/* first, sort */
	qsort(temp, size, sizeof(cpoint_p), cmp);

	/* then, uniq */
	j = -1;
	for (i = 0; i < size; ++i) {
		if (!last || last->point.x != temp[i]->point.x || last->point.y != temp[i]->point.y) {
			/* not equal */
			result[++j] = cpointset_create(temp[i]);
			if (!result[j]) {
				for (i = 0; i < j; ++i) {
					cpointset_destroy(result[i]);
					result[i] = NULL;
				}
				free(result);
				free(temp);
				*ret_size = 0;
				return NULL;
			}

			last = temp[i];

		}

		if (array_append(result[j]->cpoints, temp[i])) {
			for (i = 0; i <= j; ++i) {
				cpointset_destroy(result[i]);
				result[i] = NULL;
			}
			free(result);
			free(temp);
			*ret_size = 0;
			return NULL;
		}
	}

	free(temp);

	*ret_size = j + 1;
	return result;
}


/*
 * DBSCAN Algorithm implementation.
 *
 * ref: A Density-Based Algorithm for Discovering Clusters in Large Spatial Databases with Noise
 *      Martin Ester, Hans-Peter Kriegel, Jorg Sander, Xiaowei Xu
 *
 *      A Fast Approach to Clustering Datasets using DBSCAN and Pruning Algorithms
 *      S. Vijayalaksmi, M Punithavalli
 *
 *
 * arguments: cpoints	all the points
 *            size		size of the points
 *            eps		eps in the algorithm, not sqrted
 *            min_pts	min pts in the algorithm
 *
 * returns: the clusters num, or -1 if failed
 */
int dbscan_cluster(cpoint_p *cpoints, size_t size, double eps, size_t min_pts)
{
	unsigned int i, j, k;
	unsigned long next_id = 0;
	size_t uni_size;

	eps *= eps;

	cpointset_p *cpointsets = NULL;
	id_generator_p gen = NULL;
	kdtree_p tree = NULL;
	hashset_p visited = NULL; // maintaining pointers of cpoint_p
	hashset_p nnset = NULL;
	array_p noise = NULL;
	cpointset_p *nn = NULL; // for knn result

	/*
	 * Considering that the input points may have duplicated points,
	 * this will convert all single points to pointsets.
	 */
	cpointsets = convert_points(cpoints, size, &uni_size);
	if (!cpointsets) {
		return -1;
	}
	size = uni_size;

	gen = id_generator_create();

	/* create the kd-tree with pointsets, which is used as points */
	tree = kdtree_create_static((point_p *) cpointsets, size);

	visited = hashset_create(size, NULL, NULL);
	nnset = hashset_create(size, NULL, NULL);

	/* maintain border points */
	noise = array_create(128);

#define FREEALL()\
	{\
		for (i = 0; i < uni_size; ++i) {\
			cpointset_destroy(cpointsets[i]);\
			cpointsets[i] = NULL;\
		}\
		free(cpointsets); cpointsets = NULL;\
		id_generator_destroy(gen); gen = NULL;\
		kdtree_destroy(tree); tree = NULL;\
		hashset_destroy(visited); visited = NULL;\
		hashset_destroy(nnset); nnset = NULL;\
		array_destroy(noise); noise = NULL;\
		free(nn); nn = NULL;\
	}

	if (!tree || !gen || !visited || !nnset || !noise) {
		FREEALL();
		return -1;
	}

	/* traverse all points, */
	for (i = 0; i < size; ++i) {
		size_t n = 0, total = 0;
		cpointset_p point = cpointsets[i]; // notice, point is a pointset

		/* , if the point has not been visited yet */
		if (hashset_contains(visited, &point, sizeof(cpointset_p))) {
			continue;
		}
		hashset_add(visited, &point, sizeof(cpointset_p));

		/* find knn points in the kd-tree */
		nn = (cpointset_p *) kdtree_k_nearest_neighbour(tree, (point_p) point, eps, &n);

		for (j = 0; j < n; ++j) {
			total += array_size(nn[j]->cpoints);
		}

		if (total < min_pts) {
			/* border point, add to noise */
			array_append(noise, point);

		} else {
			/* core point, form a new cluster */
			size_t n_hulls = 0;
			cpointset_p *hulls = NULL, *new_hulls = NULL, *nnlist = NULL;
			hashset_p hullset = NULL;

#define FREEINNER()\
			{\
				free(hulls); hulls = NULL;\
				free(new_hulls); new_hulls = NULL;\
				free(nnlist); nnlist = NULL;\
				hashset_destroy(hullset); hullset = NULL;\
			}

			/* form a new cluster and set cluster_id of all points */
			next_id = id_generator_next_id(gen);
			point->cpoint.cluster_id = next_id;
			for (j = 0; j < array_size(point->cpoints); ++j) {
				cpoint_p cp = NULL;
				array_at(point->cpoints, j, (void **) &cp);
				cp->cluster_id = next_id;
			}

			/* add all points found in knn into the hashset for finding convex hulls */
			hashset_remove_all(nnset);
			for (j = 1; j < n; ++j) {
				hashset_add(nnset, &nn[j], sizeof(cpointset_p));
			}

			nnlist = (cpointset_p *) malloc(sizeof(cpointset_p) * (n - 1));
			if (!nnlist) {
				FREEINNER();
				FREEALL();
				return -1;
			}
			hashset_to_list(nnset, nnlist, sizeof(cpointset_p));
			hulls = (cpointset_p *) convex_hulls((point_p *) nnlist, n - 1, &n_hulls);
			free(nnlist);
			nnlist = NULL;

			hullset = hashset_create(0, NULL, NULL);
			if (!hulls || !hullset) {
				FREEINNER();
				FREEALL();
				return -1;
			}
			for (j = 0; j < n_hulls; ++j) {
				hashset_add(hullset, &hulls[j], sizeof(point_p));
			}

			/* expand the current cluster */
			while (hashset_size(nnset)) {
				size_t n = 0, total = 0;
				cpointset_p p = NULL; // non-allocated pointer

				/* traverse current cluster, */
				if (hashset_pop(nnset, &p, sizeof(cpointset_p))) {
					FREEINNER();
					FREEALL();
					return -1;
				}

				/* , if the point is not visited, */
				if (!hashset_contains(visited, &p, sizeof(cpointset_p))) {
					hashset_add(visited, &p, sizeof(cpointset_p));

					/* , and if the point is in the convex hulls */
					if (hashset_contains(hullset, &p, sizeof(cpointset_p))) {

						/* as before, find knn points */
						cpointset_p *nn_ = (cpointset_p *) kdtree_k_nearest_neighbour(tree, (point_p) p, eps, &n);
						for (j = 0; j < n; ++j) {
							total += array_size(nn_[j]->cpoints);
						}

						if (total >= min_pts) {
							/* core point, continue expanding */
							for (k = 0; k < n; ++k) {
								hashset_add(nnset, &nn_[k], sizeof(cpointset_p));
							}
						}
						free(nn_);
						nn_ = NULL;

						nnlist = (cpointset_p *) malloc(sizeof(cpointset_p) * hashset_size(nnset));
						if (!nnlist) {
							FREEINNER();
							FREEALL();
							return -1;
						}

						/* find the new convex hulls */
						hashset_to_list(nnset, nnlist, sizeof(cpointset_p));
						new_hulls = (cpointset_p *) convex_hulls((point_p *) nnlist, hashset_size(nnset), &n_hulls);
						free(nnlist);
						nnlist = NULL;

						free(hulls);
						hulls = new_hulls;
						new_hulls = NULL;

						hashset_remove_all(hullset);
						for (j = 0; j < n_hulls; ++j) {
							hashset_add(hullset, &hulls[j], sizeof(point_p));
						}
					}
				}

				/* put current point into the current cluster while expanding current cluster */
				if (p->cpoint.cluster_id == 0) {
					p->cpoint.cluster_id = next_id;
					for (j = 0; j < array_size(p->cpoints); ++j) {
						cpoint_p cp = NULL;
						array_at(p->cpoints, j, (void **) &cp);
						cp->cluster_id = next_id;
					}
				}
				/* else, p->cpoint.cluster_id should be equal to next_id */
			}

			FREEINNER();
		}

		free(nn);
		nn = NULL;
	}

	/* collect the noise (outliers), put them into new clusters */
	if (array_size(noise)) {
		array_p noise2 = array_create(array_size(noise));

		for (i = 0; i < array_size(noise); ++i) {
			cpoint_p point = NULL; // non-allocated pointer

			array_at(noise, i, (void **) &point);
			if (!point->cluster_id) {
				array_append(noise2, point);
			}
		}

		if (array_size(noise2)) {
			kdtree_p noise_tree = NULL;
			point_p *list = (point_p *) malloc(sizeof(point_p) * array_size(noise2));

			if (!list) {
				array_destroy(noise2);
				FREEALL();
				return -1;
			}

			array_to_list(noise2, (void **) list);
			noise_tree = kdtree_create_static(list, array_size(noise2));
			free(list);

			if (!noise_tree) {
				array_destroy(noise2);
				FREEALL();
				return -1;
			}

			for (i = 0; i < array_size(noise2); ++i) {
				cpointset_p cp = NULL; // non-allocated pointer
				array_at(noise2, i, (void **) &cp);

				if (!cp->cpoint.cluster_id) {
					size_t n = 0;
					cpointset_p *nn = (cpointset_p *) kdtree_k_nearest_neighbour(noise_tree, (point_p) cp, eps, &n);

					next_id = id_generator_next_id(gen);
					for (j = 0; j < n; ++j) {
						nn[j]->cpoint.cluster_id = next_id;
						for (k = 0; k < array_size(nn[j]->cpoints); ++k) {
							cpointset_p cp_ = NULL; // non-allocated pointer
							array_at(nn[j]->cpoints, k, (void **) &cp_);
							cp_->cpoint.cluster_id = next_id;
						}
					}
					free(nn);
				}
			}
			kdtree_destroy(noise_tree);
		}
		array_destroy(noise2);
	}

	FREEALL();
	return next_id;

#undef FREEALL

}

