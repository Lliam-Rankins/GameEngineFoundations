#pragma once

#include "GameObject.h"
#include "CustomAllocator.h"
#include "entities.h"
#include "collisions.h"
#include <new>  // For placement new
// #include <SDL3/SDL.h>




/**
 * GameObjectPool: Manages a pool of GameObjects using a MemoryPool
 */
class GameObjectPool {
public:
    /**
     * Constructor: Initialize the Game Object pool
     */
    GameObjectPool(int slotSize, int numSlots, GameObject *player, Vector startPos);
    
    /**
     * Destructor: Cleans up pool resources
     */
    ~GameObjectPool();

    /**
     * Spawn a new game object
     * @return Pointer to new bullet, or nullptr if pool is full
     */
    GameObject* spawn(Vector position, Vector velocity);

    /**
     * Update all active GameObjects
     */
    void update(float dt);

    // TODO: Might be outdated, and/or non modifiable
    /**
     * Render all active bullets
     */
    // void render(SDL_Renderer* renderer);

    /**
     * Get pool statistics
     */
    int getActiveCount() const { return activeCount; }
    int getCapacity() const { return capacity; }
    float getUsagePercent() const { return alloc.getUsagePercent(); }

private:
    CustomAllocator alloc;
    int* activeIDs;       // Dynamically allocated array of active IDs
    int activeCount;      // Number of active bullets
    int capacity;         // Total pool capacity
    Vector startPos;
    GameObject *player;

    std::map<std::string, Component>* componentsArray;
};