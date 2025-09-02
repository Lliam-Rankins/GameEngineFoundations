/*
	This is a component file that manages entities.
*/

#include "entity.h";
#include <memory>;

// Empty Constructor
Entity::Entity() {
	Entity* e = new Entity();
	return e;
}

// Full Constructor
Entity::Entity(OrderedPair* pos, OrderedPair* dim, SDL_Texture* texture, bool physics, OrderedPair* velocity) {
	Entity* e = new Entity();
	
	// Setting Position and Dimensions
	e->position = pos;
	e->dimensions = dim;

	// Setting Texture
	e->texture = texture;

	//  Setting if physics is applied to this object
	e->physicsApplied = physics;

	e->velocity = velocity;
	
	return e;
}

// Deconstructor
Entity::~Entity() {
	delete e->position;
	delete e->dimensions;
	delete e->texture;
	delete e->velocity;

	return;
}
