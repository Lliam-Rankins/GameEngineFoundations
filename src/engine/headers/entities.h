/*
	This is a header file that contains the function declarations for all functions in entities.cpp.
*/

#ifndef ENTITIES_H
#define ENTITIES_H

// This preprocessor directive makes sure SDL_Texture is declared before we use it,
// but only for the client. The server will not see this.
#ifdef IS_CLIENT
#include <SDL3/SDL_render.h>
struct SDL_Texture; // Forward declaration
#endif

#include "struct.h"
#include "collisions.h"

class Entity
{
public:
	// This constructor is available to both client and server.
	Entity();

// These constructors are ONLY available to the client, because they handle textures.
#ifdef IS_CLIENT
	Entity(const Vector position, const Vector dimension, SDL_Texture *texture, bool physics);
	Entity(const Vector position, SDL_Texture *texture, bool physics);
	Entity(const Vector position, SDL_Texture *texture, Vector velocity, bool physics);
	Entity(const Vector position, const Vector dimension, SDL_Texture *texture, bool physics, const Vector vel);
#endif

	~Entity();

	void setCollider(Collider *newCollider);
	void makeCollider();
	void updatePosition(); // Renamed from your header for consistency

	// These special member functions are excellent for resource management.
	Entity(const Entity &) = delete;
	Entity &operator=(const Entity &) = delete;
	Entity(Entity &&) noexcept = default;
	Entity &operator=(Entity &&) noexcept = default;

	// --- DATA MEMBERS ---
	// These members are available to both client and server.
	Vector position;
	Vector dimensions;
	Vector velocity;
	bool physicsApplied = false;
	Collider *collider = nullptr;

// This member variable will ONLY exist in the client build.
#ifdef IS_CLIENT
	SDL_Texture *texture = nullptr;
#endif
};

#endif // ENTITIES_H

