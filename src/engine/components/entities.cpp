/*
	This is a component file that manages entities.
*/

#include "../headers/entities.h"
#include <SDL3/SDL.h>

/*
 * --------------------AI USE CITATION---------------------
 * AI was used to refactor the original code written for the constructors
 * which cleaned bugs and followed best practices for constructors.
 * - hplenham
*/

/*
* Constructs an entity with default parameters.
*/
Entity::Entity()
	: position{ 0, 0 }, dimensions{ 0, 0 }, velocity{ {0, 0}, 0 }, physicsApplied(false), texture(nullptr) {
}

/*
* Constructs an entity with full parameters.
* @param pos the entity's position
* @param dim the entity's dimensions
* @param tex the SDL texture
* @param physics whether or not physics is enabled for this entity
* @param vel the entity's velocity
*/
Entity::Entity(const OrderedPair pos, const OrderedPair dim, SDL_Texture* tex, bool physics, const Velocity vel)
	: position(pos), dimensions(dim), velocity(vel), physicsApplied(physics), texture(tex) {
}

// Destructor
Entity::~Entity() {
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}
