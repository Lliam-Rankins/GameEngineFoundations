/*
	This is a component file that manages entities.
*/

#include "../headers/entities.h"
#include <SDL3/SDL.h> // Included for SDL_QueryTexture if needed

/*
 * Constructs a basic entity.
 */
Entity::Entity(Vector pos, Vector dim, Vector vel, bool phys)
	: position(pos), dimensions(dim), velocity(vel),
	  physicsApplied(phys), collider(nullptr)
{
}

// Destructor
Entity::~Entity()
{
}

// These functions are available to both client and server as they don't use textures.
void Entity::setCollider(Collider *newCollider)
{
	this->collider = newCollider;
}

void Entity::makeCollider()
{
	this->collider = new Collider(this->position.x, this->position.y, this->position.x + this->dimensions.x, this->position.y + this->dimensions.y);
}

void Entity::updatePosition()
{
	// Apply velocity to position
	this->position.x += this->velocity.x;
	this->position.y += this->velocity.y;

	// Sync collider with new position (center-based)
	if (this->collider != nullptr)
	{
		this->collider->topLeft.x = this->position.x - this->dimensions.x / 2.0f;
		this->collider->topLeft.y = this->position.y - this->dimensions.y / 2.0f;
		this->collider->bottomRight.x = this->position.x + this->dimensions.x / 2.0f;
		this->collider->bottomRight.y = this->position.y + this->dimensions.y / 2.0f;
	}
}
