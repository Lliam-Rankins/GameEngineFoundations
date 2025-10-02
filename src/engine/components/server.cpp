#include <iostream>
#include <thread>   // Required for std::this_thread::sleep_for
#include <chrono>   // Required for std::chrono::milliseconds

#include "../headers/network.h" // Include your NetworkManager
#include "../headers/mathEngine.h"
#include "../headers/timeline.h"

// Moving Platforms starting Position
Vector movingPlatPos_1 = {500, 600};
float movingPlatSpeed = 50;


int main(int argc, char* argv[]) {

    std::cout << "Starting server..." << std::endl;

    Timeline timeline;
    NetworkManager serverManager;

    // Use the same ports you configured in the client
    const int PUBLISH_PORT = 5556;
    const int HANDSHAKE_PORT = 5557;

    const int REPLY_PORT = 5600;

    if (!serverManager.startServer(REPLY_PORT, PUBLISH_PORT, HANDSHAKE_PORT)) {
        std::cerr << "Failed to start the server." << std::endl;
        return 1;
    }

    std::cout << "Server started successfully. Waiting for clients..." << std::endl;

    int movingPlat_1_ID = 0;
    
    Vector movingPlat_currPos = movingPlatPos_1;
    std::cout << movingPlat_currPos.x << std::endl;
    float platmovement = -movingPlatSpeed;
    NPCState movingPlat_1 = {movingPlat_1_ID, movingPlat_currPos.x, movingPlat_currPos.y};

    ///////////////////////////////
    //
    //  Main Server Loop
    //
    ///////////////////////////////
    while (true) {
        // Time Line Update
		timeline.update();
		float d_time = timeline.getDeltaTime();
        

        // Update moving platforms position
		if (movingPlat_1.x > movingPlatPos_1.x + 100) platmovement = -movingPlatSpeed;
		if (movingPlat_1.x < movingPlatPos_1.x - 100) platmovement = movingPlatSpeed;
        movingPlat_1.x += platmovement * d_time;


        serverManager.updateGameStateNPC(movingPlat_1);
        
        // Prevent the server from using 100% CPU.
        // A 16ms sleep gives us a "tick rate" of about 60 updates per second.
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // In this simple server, the loop never ends.
    // You would press Ctrl+C to stop it.
    // The NetworkManager's destructor will handle cleanup automatically.
    return 0;
}
