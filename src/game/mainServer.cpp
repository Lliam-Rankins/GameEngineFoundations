#include "../engine/headers/network.h" 
#include "../engine/headers/protocol.h"
#include "../engine/headers/timeline.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>

int main(int argc, char* argv[]) {
    // Instantiate a network manager
    NetworkManager manager;
    // Assign a few port nums
    int replyPortStart = 5050;
    int pubPort = 5030;
    int handshakePort = 5040;

    // Create a GameObject list and mutex
    std::vector<GameObject *> serverObjectList;
    std::mutex serverObjectMutex;

    // Create a timeline
    Timeline timeline(5);

    // Instantiate NPC GameObjects
    GameObject *platform = new GameObject();
    platform->setComponent("is_npc", true);
    platform->setComponent("npc_id", -3);
    platform->setComponent("position", Vector{0,1000});
    serverObjectList.push_back(platform);

    GameObject *platform2 = new GameObject();
    platform2->setComponent("is_npc", true);
    platform2->setComponent("npc_id", -4);
    platform2->setComponent("position", Vector{400,750});
    serverObjectList.push_back(platform2);

    GameObject *platform3 = new GameObject();
    platform3->setComponent("is_npc", true);
    platform3->setComponent("npc_id", -5);
    platform3->setComponent("position", Vector{800,900});
    serverObjectList.push_back(platform3);
  

    GameObject *mover = new GameObject();
    mover->setComponent("is_npc", true);
    mover->setComponent("npc_id", -2);
    mover->setComponent("position", Vector{52,800});
    mover->setComponent("velocity", Vector{0,15});
    serverObjectList.push_back(mover);

    GameObject *mover2 = new GameObject();
    mover2->setComponent("is_npc", true);
    mover2->setComponent("npc_id", -6);
    mover2->setComponent("position", Vector{600,750});
    mover2->setComponent("velocity", Vector{30,0});
    serverObjectList.push_back(mover2);

    GameObject *coin1 = new GameObject();
    coin1->setComponent("is_npc", true);
    coin1->setComponent("npc_id", -7);
    coin1->setComponent("position", Vector{400, 725});
    serverObjectList.push_back(coin1);

    EventManager eventManager;

    eventManager.RegisterListener(CollisionEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
        const auto &collision = static_cast<const CollisionEvent &>(e);
        
        {
            if(collision.objectB_ID == -7) {
                std::cout << "Coin Collected!!!" << std::endl;
                coin1->setComponent("position", Vector{-10000,-10000});
            }
            
        }
    });

    eventManager.RegisterListener(InputEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
        const auto &input = static_cast<const InputEvent  &>(e);

        if(input.action == 1) {
            if(mover->getComponent<Vector>("position").y >= 950 || mover->getComponent<Vector>("position").y <= 650) {
                mover->setComponent("velocity", Vector{mover->getComponent<Vector>("velocity").x, mover->getComponent<Vector>("velocity").y * -1});
            } 
            Vector currPos = mover->getComponent<Vector>("position");
            Vector currVel = mover->getComponent<Vector>("velocity");
            currPos.y += currVel.y * (timeline.getDeltaTime());
            mover->setComponent("position", currPos);
        } else if(input.action == 2) {
            if(mover2->getComponent<Vector>("position").x >= 1000 || mover2->getComponent<Vector>("position").x <= 200) {
                mover2->setComponent("velocity", Vector{mover2->getComponent<Vector>("velocity").x * -1, mover2->getComponent<Vector>("velocity").y});
            } 
            Vector currPos = mover2->getComponent<Vector>("position");
            Vector currVel = mover2->getComponent<Vector>("velocity");
            currPos.x += currVel.x * (timeline.getDeltaTime());
            mover2->setComponent("position", currPos);
        }
    });

    // Start the server (the lone number is timeout for clients, remember that! Do not make it too high or else disconnected clients will stay rendered.)
    if(!manager.startServer(replyPortStart, pubPort, handshakePort, 5, serverObjectList, serverObjectMutex)) {
        std::cout << "Error starting server!" << std::endl;
        return 1;
    }

    // Server main loop
    while(true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(8));

        {
            // Lock it all down
            std::lock_guard<std::mutex> lock(serverObjectMutex);

            GameState latest = manager.getGameState();
            
            for(auto &event : latest.eventList) {
                if(event.id2 == -7) {
                    auto collisionEvent = std::make_shared<CollisionEvent>(event.timestamp, event.id1, event.id2);
                    eventManager.QueueEvent(collisionEvent);
                }
            }

            // Go through each object, for the movers update their position based on velocity
            for (auto &obj : serverObjectList) {
                if(obj->hasComponent("is_npc") && obj->getComponent<int>("npc_id") == -2) {
                   auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), 1, -2);
                    eventManager.QueueEvent(inputEvent);
                }

                if(obj->hasComponent("is_npc") && obj->getComponent<int>("npc_id") == -6) {
                   auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), 2, -6);
                    eventManager.QueueEvent(inputEvent);
                }
            }

            eventManager.ProcessEvents(std::chrono::steady_clock::now().time_since_epoch().count());
        }
        // loop
        timeline.update();
    }

    // clean up
    manager.cleanUp();
    return 0;
}