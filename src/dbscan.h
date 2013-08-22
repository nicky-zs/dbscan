#ifndef _DBSCAN_H_
#define _DBSCAN_H_

#include "geo.h"

#include <stdlib.h>


/*
 * Point which is used for clustering.
 */
typedef struct s_cpoint
{
	point_t point;

	unsigned long cluster_id;
}
cpoint_t, *cpoint_p;


/*
 * Initialize the cpoint.
 */
void cpoint_init(cpoint_p cpoint, double x, double y);


/*
 * Cluster the points, using DBSCAN.
 *
 * All the cpoints' cluster_id MUST be 0.
 *
 * eps: radius of the neighbourhood
 * min_pts: minimum of points within the neighbourhood
 *
 * Returns: the numbers of clusters if succeed;
 *          -1 if failed.
 */
int dbscan_cluster(cpoint_p *cpoints, size_t n, double eps, size_t min_pts);


#endif /* _DBSCAN_H_ */

