/*
	This is a component file that manages entities.
*/

#include "../headers/entities.h"
#include "../headers/GameObject.h"
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

// Default constructor
Entity::Entity()
	: position{ 0, 0 }, dimensions{ 0, 0 }, velocity{0, 0}, physicsApplied(false), texture(nullptr), collider(nullptr) {
}

// Full constructor with texture
Entity::Entity(const Vector pos, const Vector dim, SDL_Texture* tex, bool physics, const Vector vel)
	: position(pos), dimensions(dim), velocity(vel), physicsApplied(physics), texture(tex), collider(nullptr) {
}

// Full constructor with texture
Entity::Entity(const Vector pos, const Vector dim, SDL_Texture* tex, bool physics)
	: position(pos), dimensions(dim), velocity(Vector(0,0)), physicsApplied(physics), texture(tex), collider(nullptr) {
}

// Constructor for non-textured entities (server-friendly)
Entity::Entity(Vector pos, Vector dim, Vector vel, bool phys)
	: position(pos), dimensions(dim), velocity(vel), physicsApplied(phys), texture(nullptr), collider(nullptr)
{
}

Entity::Entity(const Vector position, SDL_Texture* texture, bool physics)
	: position(position), dimensions(Vector()), velocity(Vector()), physicsApplied(physics), texture(texture), collider(nullptr) {
		float w = 0;
		float h = 0;
		SDL_GetTextureSize(texture, &w, &h);
		dimensions = Vector{(float)w, (float)h};
	}

// Destructor - The Entity does NOT own the texture.
Entity::~Entity()
{
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

void Entity::updatePosition(bool isPaused)
{
	if(isPaused) {
		return;
	}
	std::unique_lock<std::mutex> cv_lock(entityMutex);
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


////////////////////////////////////////////
//
//	Getters
//
////////////////////////////////////////////

Vector Entity::getPosition() {
	std::unique_lock<std::mutex> cv_lock(entityMutex);
	return this->position;
}

Vector Entity::getDimensions() {
	std::unique_lock<std::mutex> cv_lock(entityMutex);
	return this->dimensions;
}

Vector Entity::getVelocity() {
	std::unique_lock<std::mutex> cv_lock(entityMutex);
	return this->velocity;
}

bool Entity::getPhysicsApplied() {
	std::unique_lock<std::mutex> cv_lock(entityMutex);
	return this->physicsApplied;
}

////////////////////////////////////////////
//
//	Setters
//
////////////////////////////////////////////

void Entity::setPosition(Vector newPosition) {
	std::unique_lock<std::mutex> cv_lock(entityMutex);
	this->position = newPosition;
}

void Entity::setDimensions(Vector newDimensions) {
	std::unique_lock<std::mutex> cv_lock(entityMutex);
	this->dimensions = newDimensions;
}

void Entity::setVelocity(Vector newVelocity) {
	std::unique_lock<std::mutex> cv_lock(entityMutex);
	this->velocity = newVelocity;
}

void Entity::setPhysicsApplied(bool newPhysicsApplied) {
	std::unique_lock<std::mutex> cv_lock(entityMutex);
	this->physicsApplied = newPhysicsApplied;
}

void Entity::setCollider(Collider *newCollider) {
	std::unique_lock<std::mutex> cv_lock(entityMutex);
	this->collider = newCollider;
}