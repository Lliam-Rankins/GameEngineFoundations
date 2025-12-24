#include "../headers/GameObjectPool.h"

GameObjectPool::GameObjectPool(int slotSize, int numSlots)
    : alloc(slotSize, numSlots), activeCount(0), capacity(numSlots) {
    // Dynamically allocate the array for tracking active IDs
    activeIDs = new int[numSlots];
    componentsArray = new std::map<std::string, Component>[numSlots];
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

GameObject* GameObjectPool::spawn() {
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
    
    // Add the new bullet's ID to the active list
    activeIDs[activeCount++] = id;

    return gameObject;
}

void GameObjectPool::despawn(GameObject* obj) {
    // 1. Find the ID associated with this pointer
    // Since we don't have a direct Pointer->ID map, we search the active list.
    int foundIndex = -1;
    int foundID = -1;

    for (int i = 0; i < activeCount; ++i) {
        int id = activeIDs[i];
        // Compare the pointer address
        if (alloc.getPtr(id) == (void*)obj) {
            foundID = id;
            foundIndex = i;
            break;
        }
    }

    if (foundIndex == -1) {
        SDL_Log("Error: Trying to despawn object not managed by this pool!");
        return;
    }

    // 2. Destruct the object manually
    obj->~GameObject();

    // 3. Free the slot in the allocator using the CORRECT method name
    alloc.freeSlot(foundID); 

    // 4. Remove from activeIDs list (Swap and Pop method for O(1) removal)
    // Move the last active ID into the slot of the one we just removed
    activeIDs[foundIndex] = activeIDs[activeCount - 1];
    activeCount--;
}

// // TODO: Change 
// void BulletPool::update(float dt) {
//     // Iterate through active bullets and update them
//     for (int i = 0; i < activeCount; ) {
//         int id = activeIDs[i];
//         Bullet* bullet = reinterpret_cast<Bullet*>(alloc.getPtr(id));
        
//         bullet->update(dt);

//         // If bullet is no longer active, clean it up
//         if (!bullet->active) {
//             // 1. Explicitly call the destructor
//             bullet->~Bullet();
//             // 2. Free the memory slot
//             alloc.freeSlot(id);
//             // 3. Remove from active list using swap-and-pop
//             activeIDs[i] = activeIDs[activeCount - 1];
//             activeCount--;
//             // Do not increment 'i' since we need to check the swapped element
//         } else {
//             i++; // Increment only if no removal occurred
//         }
//     }
// }

// void GameObjectPool::render(SDL_Renderer* renderer) {
//     // Render all active bullets
//     for (int i = 0; i < activeCount; i++) {
//         int id = activeIDs[i];
//         Bullet* bullet = reinterpret_cast<Bullet*>(alloc.getPtr(id));
//         bullet->render(renderer);
//     }
// }