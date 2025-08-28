/*
	This is a component file that manages entities.
*/

#include "entity.h";
#include <memory>;

Entity* entity(float x1, float y1, float x2, float y2, enum ent_type type) {
	Entity* e = new Entity;
	e->x1 = x1;
	e->y1 = y1;
	e->x2 = x2;
	e->y2 = y2;

	e->xc = (x1 + x2) / 2;
	e->yc = (y1 + y2) / 2;

	e->type = type;
	return e;
}
