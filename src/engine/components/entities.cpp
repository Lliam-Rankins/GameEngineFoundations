/*
	This is a component file that manages entities.
*/

#include "entity.h";
#include <memory>;

Entity* entity(OrderedPair pos, OrderedPair dim, SDL_Texture* texture) {
	Entity* e = new Entity();
	
	// Setting Position and Dimensions
	e->position = pos;
	e->dimensions = dim;

	// Setting Texture
	e->texture = texture;

	
	return e;
}
