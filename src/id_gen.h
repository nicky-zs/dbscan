#ifndef _ID_GEN_H_
#define _ID_GEN_H_


/*
 * Auto-increasing id generator.
 */
typedef struct s_id_generator *id_generator_p;


/*
 * Create an id_generator.
 *
 * NOTE: MUST be destroyed.
 */
id_generator_p id_generator_create();


/*
 * Release an id_generator.
 */
void id_generator_destroy(id_generator_p gen);


/*
 * Get the next id.
 */
unsigned long id_generator_next_id(id_generator_p gen);


#endif /* _ID_GEN_H_ */

