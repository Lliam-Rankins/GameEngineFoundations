/*
    This is the new main file for the server, rebuilt to use the
    GameObject and Component model as the authoritative game state. Some of the content in this file was edited with AI tools.
    This citation is to abide by the syllabus requirement that "appropriate citations"
    must be given when referring to external sources. More information is available upon request.
*/
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <cmath>

#include "../headers/network.h"
#include "../headers/timeline.h"
#include "../headers/GameObject.h"
#include "../headers/gameUtils.h"

int main(int argc, char *argv[])
{
    std::cout << "Starting server..." << std::endl;

    std::vector<GameObject *> serverMasterObjectList;
    std::mutex serverObjectListMutex;

    GameObject *platform = new GameObject();
    platform->setComponent("is_platform", true);
    platform->setComponent("object_id", -1);
    platform->setComponent("position", Vector(1920 / 2.0f, 1080 / 2.0f));
    platform->setComponent("dimensions", Vector(62.0f, 30.0f));
    serverMasterObjectList.push_back(platform);

    GameObject *platform2 = new GameObject();
    platform2->setComponent("is_platform", true);
    platform2->setComponent("object_id", -2);
    platform2->setComponent("position", Vector(1920 / 2.0f + 300.0f, 1080 / 2.0f + 150.0f));
    platform2->setComponent("dimensions", Vector(32.0f, 30.0f));
    serverMasterObjectList.push_back(platform2);

    GameObject *policeCar = new GameObject();
    policeCar->setComponent("is_npc", true);
    policeCar->setComponent("npc_id", 0);
    policeCar->setComponent("position", Vector(860.0f, 590.0f));
    policeCar->setComponent("velocity", Vector(150.0f, 0.0f));
    policeCar->setComponent("dimensions", Vector(163.0f, 60.0f));
    serverMasterObjectList.push_back(policeCar);

    GameObject *hotelSign = new GameObject();
    hotelSign->setComponent("is_hotel", true);
    hotelSign->setComponent("npc_id", 1);
    hotelSign->setComponent("position", Vector(1920 / 2.0f - 400.0f, 1080 / 2.0f - 100.0f));
    hotelSign->setComponent("dimensions", Vector(68.0f, 35.0f));
    hotelSign->setComponent("velocity", Vector(0.0f, 100.0f));
    serverMasterObjectList.push_back(hotelSign);

    GameObject *spawnPoint1 = new GameObject();
    spawnPoint1->setComponent("is_spawnpoint", true);
    spawnPoint1->setComponent("position", Vector(1920 / 2.0f, 1080 / 2.0f - 100.0f));
    serverMasterObjectList.push_back(spawnPoint1);

    GameObject *spawnPoint2 = new GameObject();
    spawnPoint2->setComponent("is_spawnpoint", true);
    spawnPoint2->setComponent("position", Vector(200.0f, 200.0f));
    serverMasterObjectList.push_back(spawnPoint2);

    NetworkManager serverManager;
    const int REPLY_PORT = 6000;
    const int PUBLISH_PORT = 5556;
    const int HANDSHAKE_PORT = 5557;

    // Start the server, passing pointers to our game state.
    // This connects the NetworkManager to our master list.
    if (!serverManager.startServer(REPLY_PORT, PUBLISH_PORT, HANDSHAKE_PORT, 5000,
                                   serverMasterObjectList, serverObjectListMutex))
    {
        std::cerr << "Failed to start the server." << std::endl;
        return 1;
    }

    // 4. ==================== SIMULATION LOOP ====================
    Timeline serverTimeline;
    std::cout << "Server started. Running simulation..." << std::endl;

    // Use steady sleep_until to maintain a steady tick rate and reduce jitter.
    const std::chrono::milliseconds TICK_MS(16); // ~60Hz
    auto next_tick = std::chrono::steady_clock::now() + TICK_MS;
    while (true)
    {
        serverTimeline.update();
        float dt = serverTimeline.getDeltaTime();
        const float MAX_DELTA_TIME = 1.0f / 20.0f; // Clamp delta time
        if (dt > MAX_DELTA_TIME)
            dt = MAX_DELTA_TIME;
        if (dt <= 0)
        {
            std::this_thread::sleep_until(next_tick);
            next_tick += TICK_MS;
            continue;
        }

        // --- Physics / AI System ---
        // Lock only for the duration of state updates so network threads
        // (messageLooper) can access the object list while the server sleeps.
        {
            std::lock_guard<std::mutex> lock(serverObjectListMutex);

            // This system loops through all objects and updates them based on their components.
            for (auto &obj : serverMasterObjectList)
            {
                if (obj->hasComponent("is_npc") && obj->getComponent<int>("npc_id") == 0)
                {
                    Vector pos = obj->getComponent<Vector>("position");
                    Vector vel = obj->getComponent<Vector>("velocity");
                    const float leftBound = 860.0f - 100.0f;
                    const float rightBound = 860.0f + 100.0f;
                    float nextX = pos.x + vel.x * dt;
                    if (nextX > rightBound)
                    {
                        float overflow = nextX - rightBound;
                        nextX = rightBound - overflow;
                        vel.x = -std::fabs(vel.x);
                    }
                    else if (nextX < leftBound)
                    {
                        float overflow = leftBound - nextX;
                        nextX = leftBound + overflow;
                        vel.x = std::fabs(vel.x);
                    }
                    pos.x = nextX;
                    obj->setComponent("position", pos);
                    obj->setComponent("velocity", vel);
                }
                if (obj->hasComponent("is_npc") && obj->getComponent<int>("npc_id") == 1)
                {
                    Vector pos = obj->getComponent<Vector>("position");
                    Vector vel = obj->getComponent<Vector>("velocity");

                    // Define absolute world coordinates for the sign's movement
                    const float upBound = (1080.0f / 2.0f - 100.0f) - 80.0f;   // Top of patrol
                    const float downBound = (1080.0f / 2.0f - 100.0f) + 80.0f; // Bottom of patrol
                    float nextY = pos.y + vel.y * dt;

                    if (nextY > downBound && vel.y > 0)
                    {
                        nextY = downBound - (nextY - downBound);
                        vel.y *= -1.0f;
                    }
                    else if (nextY < upBound && vel.y < 0)
                    {
                        nextY = upBound + (upBound - nextY);
                        vel.y *= -1.0f;
                    }
                    pos.y = nextY;
                    obj->setComponent("position", pos);
                    obj->setComponent("velocity", vel);
                }
            }
        }

        // Limit server tick rate using steady sleep to keep ticks regular
        std::this_thread::sleep_until(next_tick);
        next_tick += TICK_MS;
    }

    return 0; // This simple server runs forever
}
