#include <iostream>
#include <thread> // Required for std::this_thread::sleep_for
#include <chrono> // Required for std::chrono::milliseconds
#include <vector>
#include <mutex>
#include "../headers/network.h"
#include "../headers/timeline.h"
#include "../headers/GameObject.h"

int main(int argc, char *argv[])
{
    std::cout << "Starting server..." << std::endl;
    std::vector<GameObject *> serverMasterObjectList;
    std::mutex serverObjectListMutex;
    NetworkManager serverManager;

    // Use the same ports you configured in the client
    const int REPLY_PORT = 6000;
    const int PUBLISH_PORT = 5556;
    const int HANDSHAKE_PORT = 5557;

    if (!serverManager.startServer(REPLY_PORT, PUBLISH_PORT, HANDSHAKE_PORT, serverMasterObjectList, serverObjectListMutex))
    {
        std::cerr << "Failed to start the server." << std::endl;
        return 1;
    }

    Timeline serverTimeline;
    // Let's create a server-side representation of the police car
    NPCState policeCarState = {0, 860.0f, 590.0f}; // objectId 0, initial position
    float policeCarVelocityX = 150.0f;

    // The main server loop now runs the simulation.
    while (true)
    {
        serverTimeline.update();
        float dt = serverTimeline.getDeltaTime();

        // 1. Simulate the NPC on the server
        int leftBound = 860 - 100;
        int rightBound = 860 + 100;
        if (policeCarState.x > rightBound || policeCarState.x < leftBound)
        {
            policeCarVelocityX *= -1.0f;
        }
        policeCarState.x += policeCarVelocityX * dt;

        // 2. Safely update the shared GameState for all threads to see
        serverManager.updateNpcState(policeCarState);

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 ticks per second
    }

    // In this simple server, the loop never ends.
    // You would press Ctrl+C to stop it.
    // The NetworkManager's destructor will handle cleanup automatically.
    return 0;
}
