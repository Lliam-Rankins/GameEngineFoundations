#include "../headers/ship_bullet_pool.h"

Ship_Bullet_Pool::Ship_Bullet_Pool(int slotSize, int numSlots, GameObject *player, std::vector<GameObject *> *objects, std::mutex *objectMutex)
    : alloc(slotSize, numSlots), activeCount(0), capacity(numSlots) {
    // Dynamically allocate the array for tracking active IDs
    activeIDs = new int[numSlots];
    componentsArray = new std::map<std::string, Component>[numSlots];
    this->startPos = startPos;
    this->player = player;
    this->objects = objects;
    this->objectMutex = objectMutex;
}

Ship_Bullet_Pool::~Ship_Bullet_Pool() {
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

GameObject* Ship_Bullet_Pool::spawn() {
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

    float orientation = player->getComponent<float>("orientation");
    Vector vel{bulletSpeed * cos(orientation - M_PI/2), bulletSpeed * sin(orientation - M_PI/2)};

    Vector playerSpeed = player->getComponent<Vector>("velocity");

    gameObject->setComponent("position", player->getComponent<Vector>("position"));
    gameObject->setComponent("velocity", Vector{vel.x,vel.y});

    std::cout << "Player: " << playerSpeed.x << " |  Bullet: " << vel.x << " |  Total: " <<  gameObject->getComponent<Vector>("velocity").x << std::endl;

    // Add the new bullet's ID to the active list
    activeIDs[activeCount++] = id;

    return gameObject;
}

// TODO: Change 
void Ship_Bullet_Pool::update(float dt) {
    // Iterate through active bullets and update them
    for (int i = 0; i < activeCount; ) {
        int id = activeIDs[i];
        GameObject* bullet = reinterpret_cast<GameObject*>(alloc.getPtr(id));
        
        Vector currPos = bullet->getComponent<Vector>("position");
        Vector currVel = bullet->getComponent<Vector>("velocity");

        Vector newPos = {currPos.x + (currVel.x * dt), currPos.y + (currVel.y * dt)};

        bullet->setComponent("position", newPos);
        bullet->setComponent("isBullet", true);
        
       

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