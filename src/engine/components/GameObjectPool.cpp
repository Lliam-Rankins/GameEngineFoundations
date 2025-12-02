#include "../headers/GameObjectPool.h"

GameObjectPool::GameObjectPool(int slotSize, int numSlots, GameObject *player, Vector startPos)
    : alloc(slotSize, numSlots), activeCount(0), capacity(numSlots) {
    // Dynamically allocate the array for tracking active IDs
    activeIDs = new int[numSlots];
    componentsArray = new std::map<std::string, Component>[numSlots];
    this->startPos = startPos;
    this->player = player;
}

GameObjectPool::~GameObjectPool() {
    // Manually destruct any remaining active bullets
    for (int i = 0; i < activeCount; ++i) {
        int id = activeIDs[i];
        GameObject* gameObject = reinterpret_cast<GameObject*>(alloc.getPtr(id));
        gameObject->~GameObject();
    }
    // Free the tracking array
    delete[] activeIDs;
    delete[] componentsArray;
}

GameObject* GameObjectPool::spawn(Vector position, Vector velocity) {
    // Check if the pool is already at capacity
    if (activeCount >= capacity) {
        SDL_Log("GameObjectPool FULL! Cannot spawn more GameObjects.");
        return nullptr;
    }

    // Allocate a memory slot
    int id = alloc.alloc();
    if (id == -1) {
        // This can happen if alloc logic differs from activeCount, a safeguard.
        SDL_Log("Allocator returned -1, pool is full.");
        return nullptr;
    }

    // Get a pointer to the allocated memory
    void* mem = alloc.getPtr(id);
    
    // Use placement new to construct a Bullet object at the memory location
    GameObject* gameObject = new (mem) GameObject(&componentsArray[id]);

    gameObject->setComponent("position", position);
    gameObject->setComponent("velocity", velocity);
    
    // Add the new bullet's ID to the active list
    activeIDs[activeCount++] = id;

    return gameObject;
}

// TODO: Change 
void GameObjectPool::update(float dt) {
    // Iterate through active bullets and update them
    for (int i = 0; i < activeCount; ) {
        int id = activeIDs[i];
        GameObject* bullet = reinterpret_cast<GameObject*>(alloc.getPtr(id));
        
        Vector currPos = bullet->getComponent<Vector>("position");
        Vector currVel = bullet->getComponent<Vector>("velocity");

        Vector newPos = {currPos.x + (currVel.x * dt), currPos.y + (currVel.y * dt)};

        bullet->setComponent("position", newPos);

        // Colliding with player
        if (overlappingColliders1(*bullet, *player)) {
            player->setComponent("position", player->getComponent<Vector>("starting_pos"));
        }


        // If bullet is no longer active, clean it up
        if (newPos.x < 0) {
            // 1. Explicitly call the destructor
            bullet->~GameObject();
            // 2. Free the memory slot
            alloc.freeSlot(id);
            // 3. Remove from active list using swap-and-pop
            activeIDs[i] = activeIDs[activeCount - 1];
            activeCount--;
            // Do not increment 'i' since we need to check the swapped element
        } else {
            i++; // Increment only if no removal occurred
        }

    }
}

// void GameObjectPool::render(SDL_Renderer* renderer) {
//     // Render all active bullets
//     for (int i = 0; i < activeCount; i++) {
//         int id = activeIDs[i];
//         Bullet* bullet = reinterpret_cast<Bullet*>(alloc.getPtr(id));
//         bullet->render(renderer);
//     }
// }