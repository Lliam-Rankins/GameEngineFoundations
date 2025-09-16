/*
	This is a component file that manages entities.
*/

#include "../headers/entities.h"
#include <SDL3/SDL.h> // Included for SDL_QueryTexture if needed

/*
* Constructs a basic entity. The implementation depends on whether this is a
* client or server build.
*/
#ifdef IS_CLIENT
// CLIENT version of the default constructor
Entity::Entity()
	: position{0, 0}, dimensions{0, 0}, velocity{0, 0}, physicsApplied(false), collider(nullptr), texture(nullptr)
{
}
#else
// SERVER version of the default constructor (no texture member)
Entity::Entity()
	: position{0, 0}, dimensions{0, 0}, velocity{0, 0}, physicsApplied(false), collider(nullptr)
{
}
#endif

// The following constructors are ONLY compiled for the client.
#ifdef IS_CLIENT
Entity::Entity(const Vector pos, const Vector dim, SDL_Texture *tex, bool physics)
	: position(pos), dimensions(dim), physicsApplied(physics), collider(nullptr), texture(tex)
{
	velocity = Vector{0, 0};
}

Entity::Entity(const Vector pos, SDL_Texture *tex, bool physics)
	: position(pos), physicsApplied(physics), collider(nullptr), texture(tex)
{
	velocity = Vector{0, 0};
	// Safely query texture dimensions using SDL3's SDL_GetTextureSize
	if (tex) {
        float w, h;
        SDL_GetTextureSize(tex, &w, &h);
		dimensions = Vector{w, h};
	} else {
		dimensions = Vector{0, 0};
	}
}

Entity::Entity(const Vector pos, SDL_Texture *tex, Vector vel, bool physics)
	: position(pos), velocity(vel), physicsApplied(physics), collider(nullptr), texture(tex)
{
	// Safely query texture dimensions using SDL3's SDL_GetTextureSize
	if (tex) {
        float w, h;
        SDL_GetTextureSize(tex, &w, &h);
		dimensions = Vector{w, h};
	} else {
		dimensions = Vector{0, 0};
	}
}

Entity::Entity(const Vector pos, const Vector dim, SDL_Texture *tex, bool physics, const Vector vel)
	: position(pos), dimensions(dim), velocity(vel), physicsApplied(physics), collider(nullptr), texture(tex)
{
}
#endif // IS_CLIENT

// Destructor
Entity::~Entity()
{
#ifdef IS_CLIENT
	// This code will only be compiled for the client.
	if (texture)
	{
		// Note: The entity that loads the texture should be responsible for destroying it.
		// If multiple entities share one texture, this will cause a crash.
		// For this project, assuming each entity has a unique texture is okay.
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}
#endif
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


