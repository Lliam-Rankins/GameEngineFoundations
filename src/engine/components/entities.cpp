/*
	This is a component file that manages entities.
*/

#include "product.h"
#include <memory>

Entity* entity(int x1, int y1, int x2, int y2, enum ent_type type) {
	Entity* e = new Entity;

	e->x1 = x1;
	e->y1 = y1;
	e->x2 = x2;
	e->y2 = y2;

	e->type = type;

	return e;
}
