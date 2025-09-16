#ifndef ENTITIES_H
#define ENTITIES_H

#include "struct.h"
#include "collisions.h"

class Entity
{
public:
    // A simple constructor for our data-only entity
    Entity(Vector pos = {0,0}, Vector dim = {0,0}, Vector vel = {0,0}, bool phys = false);
    ~Entity();

    void setCollider(Collider *newCollider);
    void makeCollider();
    void updatePosition();

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
};

#endif // ENTITIES_H