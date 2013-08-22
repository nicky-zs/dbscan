#include "geo.h"

#include <stdlib.h>

#include <math.h>

#include "array.h"


#define P_INF INFINITY

#define N_INF (-1 * INFINITY)


void point_init(point_p point, double x, double y)
{
	point->x = x;
	point->y = y;
}


double point_dist(point_p a, point_p b)
{
	return pow(a->x - b->x, 2) + pow(a->y - b->y, 2);
}


int point_equals(point_p a, point_p b)
{
	return a == b || (a->x == b->x && a->y == b->y);
}


void point_clone_to(point_p from, point_p to)
{
	if (from != to) {
		to->x = from->x;
		to->y = from->y;
	}
}


/*
 * A line, including two points: head and tail.
 */
typedef struct s_line
{
	point_p head;
	point_p tail;
}
line_t, *line_p;


static inline double line_length(line_p line)
{
	return point_dist(line->head, line->tail);
}


/*
 * Calculates the cross multiply of vectors (p0, p1) and (p0, p2).
 */
static inline double cross(point_p p0, point_p p1, point_p p2)
{
	return (p1->x - p0->x) * (p2->y - p0->y) - (p1->y - p0->y) * (p2->x - p0->x);
}


/*
 * Tell the clock direction of l1 from l2, which have the same head.
 *
 * Returns: 0 if l1 and l2 are the save direction;
 *          1 if l1 is clockwise to l2;
 *         -1 if l1 is un-clockwise to l2.
 */
static inline int clock_direct(line_p l1, line_p l2)
{
	register double m = cross(l1->head, l1->tail, l2->tail);

	return (m > 0.0) - (m < 0.0);
}


static int cmp_line(const void *line1, const void *line2)
{
	return -clock_direct((line_p) line1, (line_p) line2);
}


/*
 * Whether p0 -- p1 -- p2 makes a left turn.
 */
static inline int is_left_turn(point_p p0, point_p p1, point_p p2)
{
	return cross(p0, p1, p2) > 0;
}


/* NOTE: the caller MUST free the result. */
static point_p *rerange(point_p *points, size_t size, size_t *ret_size)
{
	line_t *lines = NULL;
	array_p rlines = NULL;
	point_p *result = NULL;
	point_p head = NULL;
	double min_y = INFINITY;
	size_t min_index = -1;
	unsigned int i = 0;

#define FREEALL()\
	{\
		free(lines); lines = NULL;\
		array_destroy(rlines); rlines = NULL;\
	}

	lines = (line_t *) malloc(sizeof(line_t) * (size - 1));
	rlines = array_create(128);
	if (!lines || !rlines) {
		FREEALL();
		return NULL;
	}

	for (i = 0; i < size; ++i) {
		if (points[i]->y < min_y) {
			min_y = points[i]->y;
			min_index = i;
		} else if (points[i]->y == min_y) {
			if (points[i]->x < points[min_index]->x) {
				min_index = i;
			}
		}
	}

	head = points[min_index];
	for (i = 0; i < min_index; ++i) {
		lines[i] = (line_t) { .head = head, .tail = points[i] };
	}
	for (i = min_index + 1; i < size; ++i) {
		lines[i - 1] = (line_t) { .head = head, .tail = points[i] };
	}
	qsort(lines, size - 1, sizeof(line_t), cmp_line);

	array_append(rlines, &lines[0]);

	for (i = 1; i < size - 1; ++i) {
		line_p line = &lines[i], last = NULL;

		array_at(rlines, array_size(rlines) - 1, (void **) &last);

		if (clock_direct(line, last)) {
			array_append(rlines, line);
		} else {
			if (line_length(last) < line_length(line)) {
				array_pop(rlines, NULL);
				array_append(rlines, line);
			}
		}
	}

	result = (point_p *) malloc(sizeof(point_p) * (array_size(rlines) + 1));
	if (result) {
		line_p line = NULL;

		result[0] = head;
		for (i = 0; i < array_size(rlines); ++i) {
			array_at(rlines, i, (void **) &line);
			result[i + 1] = line->tail;
		}
		*ret_size = array_size(rlines) + 1;
	}

	FREEALL();
	return result;

#undef FREEALL

}


/*
 * Find convex hulls.
 *
 * refs: Introduction to Algorithms
 *       Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, Clifford Stein
 */
point_p *convex_hulls(point_p *points, size_t size, size_t *ret_size)
{
	array_p stack = NULL;
	point_p *result = NULL;
	unsigned int i = 0;
	size_t n = 0;

#define FREEALL()\
	{\
		free(points); points = NULL;\
		array_destroy(stack); stack = NULL;\
	}

	if (size <= 3) {
		result = (point_p *) malloc(sizeof(point_p *) * size);
		if (result) {
			n = size;
			for (i = 0; i < n; ++i) {
				result[i] = points[i];
			}
		}
		*ret_size = n;
		return result;
	}

	points = rerange(points, size, &n);
	if (!points) {
		FREEALL();
		return NULL;
	}

	stack = array_create(128);
	if (!stack) {
		FREEALL();
		return NULL;
	}

	if (n >= 3) {
		array_append(stack, points[0]);
		array_append(stack, points[1]);
		array_append(stack, points[2]);

		for (i = 3; i < n; ++i) {
			point_p top2 = NULL, top1 = NULL; // non-allocated pointers
			array_at(stack, array_size(stack) - 2, (void **) &top2);
			array_at(stack, array_size(stack) - 1, (void **) &top1);

			while (!is_left_turn(top2, top1, points[i])) {
				array_pop(stack, NULL);
				array_at(stack, array_size(stack) - 2, (void **) &top2);
				array_at(stack, array_size(stack) - 1, (void **) &top1);
			}
			array_append(stack, points[i]);
		}
		n = array_size(stack);

	} else {
		for (i = 0; i < n; ++i) {
			array_append(stack, points[i]);
		}
	}

	result = (point_p *) malloc(sizeof(point_p) * n);
	if (!result) {
		FREEALL();
		return NULL;
	}

	for (i = 0; i < n; ++i) {
		array_at(stack, i, (void **) &result[i]);
	}
	*ret_size = n;

	FREEALL();
	return result;

#undef FREEALL

}


void interval_init(interval_p itv, double lower, double upper)
{
	itv->lower = lower;
	itv->upper = upper;
}


int interval_contains(interval_p itv, double p)
{
	return p >= itv->lower && p <= itv->upper;
}


void interval_enlarge_to(interval_p itv, double p)
{
	if (itv->lower > p) {
		itv->lower = p;
	} else if (itv->upper < p) {
		itv->upper = p;
	}
}


void interval_clone_to(interval_p from, interval_p to)
{
	to->lower = from->lower;
	to->upper = from->upper;
}


void rect_init_space(rect_p rect)
{
	interval_init(&rect->x_itv, N_INF, P_INF);
	interval_init(&rect->y_itv, N_INF, P_INF);
}


void rect_init_point(rect_p rect, point_p point)
{
	interval_init(&rect->x_itv, point->x, point->x);
	interval_init(&rect->y_itv, point->y, point->y);
}


int rect_contains(rect_p rect, point_p point)
{
	return interval_contains(&rect->x_itv, point->x)
		&& interval_contains(&rect->y_itv, point->y);
}


void rect_enlarge_to(rect_p rect, point_p point)
{
	interval_enlarge_to(&rect->x_itv, point->x);
	interval_enlarge_to(&rect->y_itv, point->y);
}


double rect_min_dist_to(rect_p rect, point_p point)
{
	double sum = 0.0;
	double p;
	interval_p itv = NULL;

#define ADDSUM(itv, p)\
	if (!interval_contains((itv), (p))) {\
		if ((p) < (itv)->lower) {\
			sum += pow((p) - (itv)->lower, 2);\
		} else {\
			sum += pow((itv)->upper - (p), 2);\
		}\
	}

	p = point->x;
	itv = &rect->x_itv;
	ADDSUM(itv, p);

	p = point->y;
	itv = &rect->y_itv;
	ADDSUM(itv, p);

#undef ADDSUM

	return sum;
}


void rect_clone_to(rect_p from, rect_p to)
{
	interval_clone_to(&from->x_itv, &to->x_itv);
	interval_clone_to(&from->y_itv, &to->y_itv);
}


int rect_set_upper(rect_p from, rect_p to, point_p point, int xd)
{
	double p = point->dim[xd];

	if (from->dim[xd].upper < p) {
		return -1;
	}

	rect_clone_to(from, to);

	if (to->dim[xd].lower < p) {
		to->dim[xd].lower = p;
	}

	return 0;
}


int rect_set_lower(rect_p from, rect_p to, point_p point, int xd)
{
	double p = point->dim[xd];

	if (from->dim[xd].lower > p) {
		return -1;
	}

	rect_clone_to(from, to);

	if (to->dim[xd].upper > p) {
		to->dim[xd].upper = p;
	}

	return 0;
}


