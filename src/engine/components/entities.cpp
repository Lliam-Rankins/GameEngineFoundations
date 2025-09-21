/*
	This is a component file that manages entities.
*/

#include "../headers/entities.h"
#include <SDL3/SDL.h> // Included for SDL_QueryTexture if needed

/*
 * --------------------AI USE CITATION---------------------
 * AI was used to refactor the original code written for the constructors
 * which cleaned bugs and followed best practices for constructors.
 * - hplenham
*/

////////////////////////////////////////////
//
//	Entity Constructors & Destructor
//
////////////////////////////////////////////

/*
* Constructs an entity with default parameters.
*/
Entity::Entity()
	: position{ 0, 0 }, dimensions{ 0, 0 }, velocity{0, 0}, physicsApplied(false), texture(nullptr) {
}

/*
* Constructs an entity with full parameters except velocity.
* @param pos the entity's position
* @param dim the entity's dimensions
* @param tex the SDL texture
* @param physics whether or not physics is enabled for this entity
*/
Entity::Entity(const Vector position, const Vector dimension, SDL_Texture* texture, bool physics)
	: position(position), dimensions(dimension), physicsApplied(physics), texture(texture) {
        velocity = Vector{0, 0};
}

/*
* Constructs an entity with full parameters except velocity & dimensions.
* @param pos the entity's position
* @param tex the SDL texture
* @param physics whether or not physics is enabled for this entity
*/
Entity::Entity(const Vector pos, SDL_Texture* tex, bool physics)
	: position(pos),  physicsApplied(physics), texture(tex) {
        velocity = Vector{0, 0};
        dimensions = Vector{(float)tex->w, (float)tex->h};
}

/*
* Constructs an entity with full parameters except Dimensions.
* @param pos the entity's position
* @param dim the entity's dimensions
* @param tex the SDL texture
* @param vel the entity's velocity
* @param physics whether or not physics is enabled for this entity
*/
Entity::Entity(const Vector pos, SDL_Texture* tex, Vector vel, bool physics)
	: position(pos),  physicsApplied(physics), velocity(vel), texture(tex) {
        dimensions = Vector{(float)tex->w, (float)tex->h};
}

/*
* Constructs an entity with full parameters.
* @param pos the entity's position
* @param dim the entity's dimensions
* @param tex the SDL texture
* @param physics whether or not physics is enabled for this entity
* @param vel the entity's velocity
*/
Entity::Entity(const Vector pos, const Vector dim, SDL_Texture* tex, bool physics, const Vector vel)
	: position(pos), dimensions(dim), velocity(vel), physicsApplied(physics), texture(tex) {
}

// Destructor
Entity::~Entity()
{
	if (texture) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
}


////////////////////////////////////////////
//
//	Entity Funcs
//
////////////////////////////////////////////

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

// Update Collider
void Entity::updateCollider() {
    if(this->collider != nullptr) {
        this->collider->topLeft.x = this->position.x;
    	this->collider->topLeft.y = this->position.y;
    	this->collider->bottomRight.x = this->position.x + this->dimensions.x;
    	this->collider->bottomRight.y = this->position.y + this->dimensions.y;
    }
}


////////////////////////////////////////////
//
//	Getters
//
////////////////////////////////////////////

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


////////////////////////////////////////////
//
//	Setters
//
////////////////////////////////////////////

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