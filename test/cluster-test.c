#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "geo.h"
#include "array.h"
#include "dbscan.h"


#define EPS 0.001

#define MINPTS 3


void usage(char *s_progname) {
	printf("Usage: %s <file path>\n", s_progname);
}


point_t *read_points(char *s_filepath, size_t *size)
{
	FILE *fp = NULL;
	point_t *points = NULL;
	size_t s = 0, n = 1024;
	unsigned int i;
	int r;
	double x, y;

#define FREEALL()\
	{\
		fclose(fp); fp = NULL;\
		free(points); points = NULL;\
	}

	fp = fopen(s_filepath, "r");
	if (!fp) {
		return NULL;
	}

	points = (point_t *) malloc(sizeof(point_t) * n);
	if (!points) {
		FREEALL();
		return NULL;
	}

	while ((r = fscanf(fp, "%lg, %lg\n", &x, &y)) != EOF) {
		if (r != 2) {
			FREEALL();
			return NULL;
		}

		if (s == n) {
			point_t *new_points;
			n *= 2;
			new_points = (point_t *) malloc(sizeof(point_t) * n);
			if (!new_points) {
				FREEALL();
				return NULL;
			}
			for (i = 0; i < s; ++i) {
				new_points[i] = points[i];
			}
			free(points);
			points = new_points;
		}

		points[s].x = x;
		points[s].y = y;
		++s;
	}

	*size = s;
	fclose(fp);
	return points;

#undef FREEALL

}


int main(int argc, char *argv[])
{
	point_t *points = NULL;
	cpoint_t *cpoints = NULL;
	cpoint_p *cpoint_ps = NULL;
	unsigned int i, j;
	int r;
	size_t n;

#define FREEALL()\
	{\
		free(points); points = NULL;\
		free(cpoints); cpoints = NULL;\
		free(cpoint_ps); cpoint_ps = NULL;\
	}

	if (argc != 2) {
		usage(argv[0]);
		return -1;
	}

	points = read_points(argv[1], &n);
	if (!points) {
		FREEALL();
		perror(NULL);
		return -1;
	}

	cpoints = (cpoint_t *) malloc(sizeof(cpoint_t) * n);
	if (!cpoints) {
		FREEALL();
		perror(NULL);
		return -1;
	}
	for (i = 0; i < n; ++i) {
		cpoint_init(&cpoints[i], points[i].x, points[i].y);
	}

	cpoint_ps = (cpoint_p *) malloc(sizeof(cpoint_p) * n);
	if (!cpoint_ps) {
		FREEALL();
		perror(NULL);
		return -1;
	}
	for (i = 0; i < n; ++i) {
		cpoint_ps[i] = &cpoints[i];
	}

	r = dbscan_cluster(cpoint_ps, n, EPS, MINPTS);
	if (r > 0) {
		int *counts = (int *) malloc(sizeof(int) * r);
		double *deviations = (double *) malloc(sizeof(double) * r);
		point_t *centers = (point_t *) malloc(sizeof(point_t) * r);
		array_p *groups = (array_p *) malloc(sizeof(array_p) * r);

		for (i = 0; i < (unsigned int) r; ++i) {
			counts[i] = 0;
			deviations[i] = 0.0;
			centers[i].x = 0.0;
			centers[i].y = 0.0;
			groups[i] = array_create(0);
		}

		for (i = 0; i < n; ++i) {
			int id = cpoints[i].cluster_id - 1;
			++counts[id];
			centers[id].x += cpoints[i].point.x;
			centers[id].y += cpoints[i].point.y;
			array_append(groups[id], &cpoints[i]);
		}

		for (i = 0; i < (unsigned int) r; ++i) {
			centers[i].x /= counts[i];
			centers[i].y /= counts[i];
		}

		for (i = 0; i < n; ++i) {
			int id = cpoints[i].cluster_id - 1;
			// deviations[id] += point_dist((point_p) &cpoints[i], &centers[id]);
		}

		for (i = 0; i < (unsigned int) r; ++i) {
			deviations[i] /= counts[i];
		}

		// for (i = 0; i < (unsigned int) r; ++i) {
		// 	printf("group %2d:\n", i);
		// 	for (j = 0; j < array_size(groups[i]); ++j) {
		// 		point_p p = NULL;
		// 		array_at(groups[i], j, (void **) &p);
		// 		printf("(%.8g,%.8g)\n", p->x, p->y);
		// 	}
		// 	printf("\n");
		// }

		for (i = 0; i < (unsigned int) r; ++i) {
			printf("group %2d:\ttotal:%3d\tdeviation %.9g\n", i, counts[i], deviations[i]);
		}

		free(counts);
		free(centers);
		free(deviations);
		for (i = 0; i < (unsigned int) r; ++i) {
			array_destroy(groups[i]);
			groups[i] = NULL;
		}
		free(groups);

	} else {
		printf("error\n");
	}

	FREEALL();
	return 0;

#undef FREEALL

}

