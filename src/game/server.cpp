#include <iostream>
#include <thread>   // Required for std::this_thread::sleep_for
#include <chrono>   // Required for std::chrono::milliseconds

#include "../engine/headers/network.h" // Include your NetworkManager
#include "../engine/headers/entities.h"
#include "../engine/headers/mathEngine.h"
#include "../engine/headers/timeline.h"
#include "../engine/headers/collisions.h"

// Moving Platforms starting Position
Vector movingPlatformHorizontal_StartPosition = {500, 600};
Vector movingPlatformVertical_StartPosition = {1200, 300};

enum Direction {
    WIND,
    UP,
    DOWN,
    LEFT,
    RIGHT
};

float movingPlatformSpeed = 50;


int main(int argc, char* argv[]) {
    std::cout << "Starting server..." << std::endl;

    Timeline timeline;
    NetworkManager serverManager;

    // Use the same ports you configured in the client
    const int PUBLISH_PORT = 5556;
    const int HANDSHAKE_PORT = 5557;
    const int REPLY_PORT = 5600;

    // Instantiate ObjectList
    std::vector<GameObject *> objectList;
    std::mutex objectMutex;

    // Create Moving Platforms
    GameObject movingPlatformHorizontal;
    movingPlatformHorizontal.setComponent("position", movingPlatformHorizontal_StartPosition);
    movingPlatformHorizontal.setComponent("dimensions", Vector{96, 32});
    movingPlatformHorizontal.setComponent("is_npc", true);
    movingPlatformHorizontal.setComponent("npc_id", 1);

    // Create Moving Platform
    GameObject movingPlatformVertical;
    movingPlatformVertical.setComponent("position", movingPlatformVertical_StartPosition);
    movingPlatformVertical.setComponent("dimensions", Vector{96, 32});
    movingPlatformVertical.setComponent("is_npc", true);
    movingPlatformVertical.setComponent("npc_id", 2);

    std::cout << "Made Objects" << std::endl;

    objectList.push_back(&movingPlatformHorizontal);
    objectList.push_back(&movingPlatformVertical);
    
    std::cout << "Pushed" << std::endl;

    if (!serverManager.startServer(REPLY_PORT, PUBLISH_PORT, HANDSHAKE_PORT, 1, objectList, objectMutex)) {
        std::cerr << "Failed to start the server." << std::endl;
        return 1;
    }

    std::cout << "Server started successfully. Waiting for clients..." << std::endl;

    ///////////////////////////////
    //
    //  Main Server Loop
    //
    ///////////////////////////////
    float platformMovementVertical = movingPlatformSpeed;
    float platformMovementHorizontal = movingPlatformSpeed;
    while (true) {
        // Time Line Update
		timeline.update();
		float d_time = timeline.getDeltaTime();
        
        // Update moving platforms position
        {
            // Lock and update the two moving platforms
            std::lock_guard<std::mutex> lock(objectMutex);           

            // Check all objects for a collision with the moving platforms

            for (auto obj: objectList) {
                if (obj->hasComponent("client_id")) {
                    // Give obj dim
                    obj->setComponent("dimensions", Vector{64, 64});

                    // Check if colliding
                    if (overlappingColliders1(*obj, movingPlatformHorizontal)) {
                        // Apply "Wind" on moving platform
                        GameState gamestate = serverManager.getGameState();
                        gamestate.eventList.push_back(NetworkEvent {2, WIND, obj->getComponent<int>("client_id"), -1, -1, -1, 0});
                        serverManager.setGameState(gamestate);
                    }
                }   
            }

            

            Vector movingPlatformPosition = movingPlatformHorizontal.getComponent<Vector>("position");
            if (movingPlatformPosition.x > movingPlatformHorizontal_StartPosition.x + 100) platformMovementHorizontal = -movingPlatformSpeed;
            if (movingPlatformPosition.x < movingPlatformHorizontal_StartPosition.x - 100) platformMovementHorizontal = movingPlatformSpeed;
            movingPlatformHorizontal.setComponent("velocity", Vector{platformMovementHorizontal * d_time, 0});

            movingPlatformPosition = movingPlatformVertical.getComponent<Vector>("position");
            if (movingPlatformPosition.y > movingPlatformVertical_StartPosition.y + 100) platformMovementVertical = -movingPlatformSpeed;
            if (movingPlatformPosition.y < movingPlatformVertical_StartPosition.y - 100) platformMovementVertical = movingPlatformSpeed;
            movingPlatformVertical.setComponent("velocity", Vector{0, platformMovementVertical * d_time});

            updatePosition(movingPlatformHorizontal, false);
            updatePosition(movingPlatformVertical, false);

            
        }
        
        // Prevent the server from using 100% CPU.
        // A 16ms sleep gives us a "tick rate" of about 60 updates per second.
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // In this simple server, the loop never ends.
    // You would press Ctrl+C to stop it.
    // The NetworkManager's destructor will handle cleanup automatically.
    return 0;
}
