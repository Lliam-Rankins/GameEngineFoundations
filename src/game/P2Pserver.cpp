//    Some of the content in this file was generated with Gemini 2.5 Pro.
//    This citation is to abide by the syllabus requirement that "appropriate citations"
//    must be given when referring to external sources.
#include "../engine/headers/P2PServerManager.h"
#include "../engine/headers/timeline.h"
#include "../engine/headers/protocol.h" // Your shared protocol file
#include <iostream>
#include <thread>
#include <chrono>

// A simple placeholder for any server-side entity your game might have
struct ServerEntity
{
    int id;
    float x, y;
    float vx, vy;
};

int main()
{
    P2PServerManager serverManager;
    serverManager.start(5555, 5556);
    std::cout << "Authoritative P2P Server started..." << std::endl;

    // --- Game-Specific Logic Starts Here ---
    Timeline server_timeline;
    ServerEntity myGameNpc = {0, 500.0f, 500.0f, 100.0f, 0.0f};
    // ---
    // A simple timer to rebroadcast the peer list periodically
    auto last_peer_broadcast_time = std::chrono::steady_clock::now();
    while (true)
    {
        server_timeline.update();
        float dt = server_timeline.getDeltaTime();

        // Let the engine manager handle new connections
        if (serverManager.check_for_new_peers())
        {
            std::cout << "New peer connected. Broadcasting peer list." << std::endl;
            serverManager.broadcast_peer_list();
        }

        // Periodically re-broadcast the peer list every 2 seconds to ensure everyone is in sync
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_peer_broadcast_time).count() >= 2) {
            serverManager.broadcast_peer_list();
            last_peer_broadcast_time = now;
        }

        // --- Game-Specific Simulation Logic ---
        myGameNpc.x += myGameNpc.vx * dt;
        if (myGameNpc.x > 900.0f || myGameNpc.x < 500.0f)
        {
            myGameNpc.vx *= -1;
        }

        // Create a data packet for our game's NPC state
        NPCState npc_state_packet = {myGameNpc.id, myGameNpc.x, myGameNpc.y};

        // Use the engine manager to broadcast our game data
        serverManager.broadcast_data("NPCS", &npc_state_packet, sizeof(NPCState));

        // Maintain a consistent tick rate
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    serverManager.clean_up();
    return 0;
}
