#ifndef ENTITIES_H
#define ENTITIES_H

#include <mutex>
#include "struct.h"
#include "collisions.h"

// Forward declare SDL_Texture so we don't need to include the full SDL header here
struct SDL_Texture;

class Entity
{
public:
	// Mutex for thread-safe access to entity data
	std::mutex entityMutex;

	// --- Constructors & Destructor ---
	
	// Default constructor
	Entity();

	// Constructor for non-textured entities (server-friendly, from hplenham branch)
	Entity(Vector pos = {0,0}, Vector dim = {0,0}, Vector vel = {0,0}, bool phys = false);

	// Full constructor with texture (from engine branch)
	Entity(const Vector pos, const Vector dim, SDL_Texture* tex, bool physics, const Vector vel);

	// Destructor
	~Entity();

	// --- Special Member Functions ---
	// Prevents copying to avoid issues with raw pointer ownership (collider)
	// Allows moving for efficient transfers (e.g., in std::vector)
	Entity(const Entity&) = delete;
	Entity& operator=(const Entity&) = delete;
	Entity(Entity&&) noexcept = default;
	Entity& operator=(Entity&&) noexcept = default;

	// --- Public Member Functions ---

	// Getters (thread-safe)
	Vector getPosition();
	Vector getDimensions();
	Vector getVelocity();
	bool getPhysicsApplied();
	
	// Setters (thread-safe)
	void setPosition(Vector newPosition);
	void setDimensions(Vector newDimensions);
	void setVelocity(Vector newVelocity);
	void setPhysicsApplied(bool newPhysicsApplied);
	void setCollider(Collider *newCollider);

	// Other Logic
	void makeCollider();
	void updatePosition();

	// --- Public Data Members ---
	// These are public for direct access in simple, single-threaded contexts like the main game file.
	// For multi-threaded access, the getters/setters MUST be used.
	Vector position;
	Vector dimensions;
	Vector velocity;
	bool physicsApplied = false;
	SDL_Texture* texture = nullptr;
	Collider* collider = nullptr;
};

#endif // ENTITIES_H