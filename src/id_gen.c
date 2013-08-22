#include "id_gen.h"

#include <stdlib.h>


typedef struct s_id_generator
{
	/* next id */
	unsigned long id;
}
id_generator_t;


id_generator_p id_generator_create()
{
	id_generator_p gen = (id_generator_p) malloc(sizeof(id_generator_t));
	if (gen) {
		gen->id = (unsigned long) 1;
	}
	return gen;
}


void id_generator_destroy(id_generator_p gen)
{
	if (gen) {
		free(gen);
	}
}


unsigned long id_generator_next_id(id_generator_p gen)
{
	return gen->id++;
}


