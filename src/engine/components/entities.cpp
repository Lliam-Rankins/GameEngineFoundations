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


// Getters
Vector Entity::getPosition() {
	pthread_mutex_lock(&entityLock);
	Vector returnPosition = this->position;
	pthread_mutex_unlock(&entityLock);

	return returnPosition;
}

Vector Entity::getDimensions() {
	pthread_mutex_lock(&entityLock);
	Vector returnDimensions = this->dimensions;
	pthread_mutex_unlock(&entityLock);

	return returnDimensions;
}

Vector Entity::getVelocity() {
	pthread_mutex_lock(&entityLock);
	Vector returnVelocity = this->velocity;
	pthread_mutex_unlock(&entityLock);

	return returnVelocity;
}

bool Entity::getPhysicsApplied() {
	pthread_mutex_lock(&entityLock);
	bool returnPhysicsApplied = this->physicsApplied;
	pthread_mutex_unlock(&entityLock);

	return returnPhysicsApplied;
}

Collider Entity::getCollider() {
	pthread_mutex_lock(&entityLock);
	Collider returnCollider = *this->collider;
	pthread_mutex_unlock(&entityLock);

	return returnCollider;
}



// Setters
void Entity::setPosition(Vector newPosition) {
	pthread_mutex_lock(&entityLock);
	this->position = newPosition;
	pthread_mutex_unlock(&entityLock);
}

void Entity::setDimensions(Vector newDimensions) {
	pthread_mutex_lock(&entityLock);
	this->dimensions = newDimensions;
	pthread_mutex_unlock(&entityLock);
}

void Entity::setVelocity(Vector newVelocity) {
	pthread_mutex_lock(&entityLock);
	this->velocity = newVelocity;
	pthread_mutex_unlock(&entityLock);
}

void Entity::setPhysicsApplied(bool newPhysicsApplied) { 
	pthread_mutex_lock(&entityLock);
	this->physicsApplied = physicsApplied;
	pthread_mutex_unlock(&entityLock);
}

void Entity::setCollider(Collider *newCollider) {
	pthread_mutex_lock(&entityLock);
	this->collider = newCollider;
	pthread_mutex_unlock(&entityLock);
}