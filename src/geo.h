/* This library supports only 2D geo. */

#ifndef _GEO_H_
#define _GEO_H_

#include <stdlib.h>


/*
 * 2D point.
 *
 * It can be accessed by both .x, .y and dim[0], dim[1].
 */
typedef union s_point
{
	double dim[2];
	struct {
		double x;
		double y;
	};
}
point_t, *point_p;


/*
 * Initialize the point with x axis and y axis.
 */
void point_init(point_p point, double x, double y);


/*
 * Calculate the euclidean distance between point a and point b.
 */
double point_dist(point_p a, point_p b);


/*
 * Check whether the two points are the same.
 */
int point_equals(point_p a, point_p b);


/*
 * Clone the point.
 */
void point_clone_to(point_p from, point_p to);


/*
 * Find the convex hulls of the points.
 *
 * NOTE: the point_t *returned by this function MUST be freed by
 * the caller!
 */
point_p *convex_hulls(point_p *points, size_t size, size_t *ret_size);



/*
 * Interval indicating the range for some dim.
 */
typedef struct s_interval
{
	double lower;
	double upper;
}
interval_t, *interval_p;


/*
 * Initialize the interval with lower bound and upper bound.
 */
void interval_init(interval_p itv, double lower, double upper);


/*
 * Check whether the interval contains the value p.
 */
int interval_contains(interval_p itv, double p);


/*
 * Enlarge the interval just to contain the value p.
 */
void interval_enlarge_to(interval_p itv, double p);


/*
 * Clone the interval.
 */
void interval_clone_to(interval_p from, interval_p to);



/*
 * 2D rectangle.
 *
 * Each dim is represented by an interval.
 */
typedef union s_rect
{
	interval_t dim[2];
	struct {
		interval_t x_itv;
		interval_t y_itv;
	};
}
rect_t, *rect_p;


/*
 * Initialize the rect to the whole space, i.e. -INF to INF.
 */
void rect_init_space(rect_p rect);


/*
 * Initialize the rect containing only the point.
 */
void rect_init_point(rect_p rect, point_p point);


/*
 * Check whether the rect contains the point.
 */
int rect_contains(rect_p rect, point_p point);


/*
 * Enlarge the rect just to contain the point.
 */
void rect_enlarge_to(rect_p rect, point_p point);


/*
 * Calculate the minimum distance from the rect to the point.
 */
double rect_min_dist_to(rect_p rect, point_p point);


/*
 * Clone the rect.
 */
void rect_clone_to(rect_p from, rect_p to);


/*
 * Get the upper part of the rect, dividing by the point, xd axis.
 *
 * If xd == 0, then dividing by x, else dividing by y.
 *
 * Returns: 0 if succeed;
 *         -1 if failed, i.e. no upper beyond the point.
 */
int rect_set_upper(rect_p from, rect_p to, point_p point, int xd);


/*
 * Get the lower part of the rect, dividing by the point, xd axis.
 *
 * If xd == 0, then dividing by x, else dividing by y.
 *
 * Returns: 0 if succeed;
 *         -1 if failed, i.e. no lower beyond the point.
 */
int rect_set_lower(rect_p from, rect_p to, point_p point, int xd);


#endif /* _GEO_H_ */

