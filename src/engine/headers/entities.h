#ifndef ENTITIES_H
#define ENTITIES_H

#include <pthread.h>
#include "struct.h"
#include "collisions.h"

// Mutex Lock for Entity
pthread_mutex_t entityLock;

class Entity
{
public:
    // A simple constructor for our data-only entity
    Entity(Vector pos = {0,0}, Vector dim = {0,0}, Vector vel = {0,0}, bool phys = false);
    ~Entity();

    // Prevent accidental copying (pointers in collider)
    Entity(const Entity &) = delete;
    Entity &operator=(const Entity &) = delete;

    // Allow move semantics
    Entity(Entity &&) noexcept = default;
    Entity &operator=(Entity &&) noexcept = default;

    // --- DATA MEMBERS ---
    Vector position;
    Vector dimensions;
    Vector velocity;
    bool physicsApplied = false;
    Collider *collider = nullptr;

    // Getters
    Vector getPosition();
    Vector getDimensions();
    Vector getVelocity();
    bool getPhysicsApplied();
    Collider getCollider();

    // Setters
    void setPosition(Vector newPosition);
    void setDimensions(Vector newDimensions);
    void setVelocity(Vector newVelocity);
    void setPhysicsApplied(bool newPhysicsApplied);
    void setCollider(Collider *newCollider);

    void makeCollider();
    void updatePosition();
};

#endif // ENTITIES_H