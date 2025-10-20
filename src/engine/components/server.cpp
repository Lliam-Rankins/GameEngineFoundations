/*
    This is the new main file for the server, rebuilt to use the
    GameObject and Component model as the authoritative game state. Some of the content in this file was generated with Gemini 2.5 Pro.
    This citation is to abide by the syllabus requirement that "appropriate citations"
    must be given when referring to external sources. More information is available upon request.
*/
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>

#include "../headers/network.h"
#include "../headers/timeline.h"
#include "../headers/GameObject.h"
#include "../headers/gameUtils.h" // Our utility header

    int main(int argc, char *argv[])
{
    std::cout << "Starting server..." << std::endl;

    // 1. ==================== GAME STATE SETUP ====================
    // The server's main function "owns" the master list of all game objects.
    std::vector<GameObject *> serverMasterObjectList;
    std::mutex serverObjectListMutex;

    // 2. ============== INITIAL WORLD CREATION ==============
    // The server defines the static and dynamic elements of the world.

    // Create a static platform (fulfills Part 2 requirement)
    GameObject *platform = new GameObject();
    platform->setComponent("is_platform", true);
    // Note: We use a different ID scheme for non-player objects. Let's use negative numbers.
    platform->setComponent("object_id", -1);
    platform->setComponent("position", Vector(1920 / 2.0f, 1080 / 2.0f));
    platform->setComponent("dimensions", Vector(62.0f, 30.0f));
    serverMasterObjectList.push_back(platform);

    // Create the moving police car NPC (fulfills Part 2 requirement)
    GameObject *policeCar = new GameObject();
    policeCar->setComponent("is_npc", true);
    policeCar->setComponent("npc_id", 0); // Use the same ID as in your old server
    policeCar->setComponent("position", Vector(860.0f, 590.0f));
    policeCar->setComponent("velocity", Vector(150.0f, 0.0f));
    policeCar->setComponent("dimensions", Vector(163.0f, 60.0f));
    serverMasterObjectList.push_back(policeCar);

    // Create Spawn Points (fulfills Part 2 requirement)
    // These are "hidden" objects - they have a position but no dimensions or texture.
    GameObject *spawnPoint1 = new GameObject();
    spawnPoint1->setComponent("is_spawnpoint", true);
    spawnPoint1->setComponent("position", Vector(1920 / 2.0f, 1080 / 2.0f - 100.0f));
    serverMasterObjectList.push_back(spawnPoint1);

    GameObject *spawnPoint2 = new GameObject();
    spawnPoint2->setComponent("is_spawnpoint", true);
    spawnPoint2->setComponent("position", Vector(200.0f, 200.0f));
    serverMasterObjectList.push_back(spawnPoint2);

    // 3. ==================== NETWORKING SETUP ====================
    NetworkManager serverManager;
    const int REPLY_PORT = 6000;
    const int PUBLISH_PORT = 5556;
    const int HANDSHAKE_PORT = 5557;

    // Start the server, passing pointers to our game state.
    // This connects the NetworkManager to our master list.
    if (!serverManager.startServer(REPLY_PORT, PUBLISH_PORT, HANDSHAKE_PORT, 50,
                                   serverMasterObjectList, serverObjectListMutex))
    {
        std::cerr << "Failed to start the server." << std::endl;
        return 1;
    }

    // 4. ==================== SIMULATION LOOP ====================
    Timeline serverTimeline;
    std::cout << "Server started. Running simulation..." << std::endl;

    while (true)
    {
        serverTimeline.update();
        float dt = serverTimeline.getDeltaTime();
        const float MAX_DELTA_TIME = 1.0f / 20.0f; // Clamp delta time
        if (dt > MAX_DELTA_TIME)
            dt = MAX_DELTA_TIME;
        if (dt <= 0)
            continue;

        // Lock the mutex to ensure network threads don't access the list
        // while we are running the simulation.
        std::lock_guard<std::mutex> lock(serverObjectListMutex);

        // --- Physics / AI System ---
        // This system loops through all objects and updates them based on their components.
        for (auto &obj : serverMasterObjectList)
        {
            // This is the server-side simulation of the police car.
            if (obj->hasComponent("is_npc") && obj->getComponent<int>("npc_id") == 0)
            {
                Vector pos = obj->getComponent<Vector>("position");
                Vector vel = obj->getComponent<Vector>("velocity");

                // Simple patrol logic from your old server.cpp
                int leftBound = 860 - 100;
                int rightBound = 860 + 100;
                if (pos.x > rightBound || pos.x < leftBound)
                {
                    vel.x *= -1.0f;
                }
                pos.x += vel.x * dt;

                obj->setComponent("position", pos);
                obj->setComponent("velocity", vel);
            }

            // In a full game, you'd also apply gravity to players here,
            // check for collisions, run AI, etc.
        }

        // The network threads (messageLooper, readClient) will automatically
        // handle sending this updated state to the clients.

        // Limit server tick rate
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 ticks per second
    }

    return 0; // This simple server runs forever
}
