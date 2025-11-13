/*
    This is the new main file for the client, rebuilt from scratch
    to use the GameObject and Component model. Some of the content in this file was edited with AI tools.
    This citation is to abide by the syllabus requirement that "appropriate citations"
    must be given when referring to external sources. More information is available upon request.
*/
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <cmath>
#include <memory>
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/input.h"
#include "../engine/headers/network.h"
#include "../engine/headers/protocol.h"
#include "../engine/headers/timeline.h"
#include "../engine/headers/GameObject.h"
#include "../engine/headers/gameUtils.h"
#include "../engine/headers/collisions.h"
#include "../engine/headers/input.h"
#include "../engine/headers/network.h"
#include "../engine/headers/protocol.h"
#include "../engine/headers/entities.h"
#include "../engine/headers/input.h"
#include "../engine/headers/timeline.h"
#include "../engine/headers/GameObject.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <utility>


const float SCREEN_W = 1920.0f;
const float SCREEN_H = 1080.0f;

const float WORLD_WIDTH = 10000.0f;
const float WORLD_HEIGHT = 8000.0f;

const int ACTION_MOVE_LEFT = 1;
const int ACTION_MOVE_RIGHT = 2;
const int ACTION_JUMP = 3;

/**
 * Sends the player's information
 */
void sendPlayerThread(bool *running, NetworkManager *network, std::vector<GameObject *> *list, std::mutex *mut, NetworkEvent *localEvents, int *numEvents, std::mutex *eventMut, int id)
{
    // While the game is running...
    while (*running)
    {
        // Get this player...
        GameObject *myPlayer = nullptr;
        {
            // Lock it all down
            std::lock_guard<std::mutex> lock(*mut);
            myPlayer = findGameObjectByClientId(id, *list);
        }
        // If I couldn't get the player,  restart
        if (!myPlayer)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        // Create a PlayerState based on current player data and send it out
        PlayerState playerState;
        playerState.clientId = id;
        playerState.x = myPlayer->getComponent<Vector>("position").x;
        playerState.y = myPlayer->getComponent<Vector>("position").y;

        {
            std::lock_guard<std::mutex> lock2(*eventMut);
            playerState.num_events = (*numEvents);
            for (int i = 0; i < playerState.num_events; i++)
            {
                playerState.events[i] = localEvents[i];
            }
            // std::cout << "Num Local Events: " << (*numEvents) << std::endl;
            (*numEvents) = 0;
        }
        network->sendPlayerState(playerState);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

/**
 * Thread that receives the latest network updates asynchronously from the rest of main's functionality.
 */
void receiveUpdateThread(bool *running, NetworkManager *network, std::vector<GameObject *> *list, std::mutex *mut)
{
    while (*running)
    {
        {
            network->update();
            // Simplify engine later to also update position
            for (auto &obj : *list)
            {
                if (obj->hasComponent("net_position"))
                {
                    obj->setComponent("position", obj->getComponent<Vector>("net_position"));
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

int main(int argc, char *argv[])
{

	//////////////////////////////////////////////////
	//
	// Setup
	//
	//////////////////////////////////////////////////

	// Initialize the SDL library
	initializeSDL();
	createWindowAndRenderer(&window, &renderer);
	SDL_SetWindowSize(window, windowSize.x, windowSize.y);


	//Load Textures
	player_Texture = IMG_LoadTexture(renderer, "../media/darkworld_character_morwen_idle.png");
	texCheck(player_Texture);

	platform_Texture = IMG_LoadTexture(renderer, "../media/darkworld_platform_brick_idle.png");
	texCheck(platform_Texture);

	// Create Dimensions
	player_Dimensions = {player_Texture->w, player_Texture->h};
	platform_Dimensions = {platform_Texture->w, platform_Texture->h};


	// Instantiate ObjectList
    std::vector<GameObject *> objectList;
	std::vector<GameObject *> localObjects;
    std::mutex objectMutex;

	// Camera
	GameObject camera;
	camera.setComponent("position", player_Position);
	camera.setComponent("is_camera", true);

	// Player
	GameObject player;
	player.setComponent("position", player_Position);
	player.setComponent("is_player", true);
	player.setComponent("velocity", player_Velocity);
	player.setComponent("dimensions", player_Dimensions);
	player.setComponent("texture", player_Texture);
	player.setComponent("physics", true);

	// Platforms
	GameObject platform_1 = GameObject();
	platform_1.setComponent("position", platform1_Position);
	platform_1.setComponent("npc_id", -1);
	platform_1.setComponent("dimensions", platform_Dimensions);
	platform_1.setComponent("texture", platform_Texture);

	GameObject platform_2 = GameObject();
	platform_2.setComponent("position", platform2_Position);
	platform_2.setComponent("npc_id", -2);
	platform_2.setComponent("dimensions", platform_Dimensions);
	platform_2.setComponent("texture", platform_Texture);

	GameObject platform_3 = GameObject();
	platform_3.setComponent("position", platform3_Position);
	platform_3.setComponent("npc_id", -3);
	platform_3.setComponent("dimensions", platform_Dimensions);
	platform_3.setComponent("texture", platform_Texture);

	// Spawn Zones
	GameObject spawnZone_1 = GameObject();
	spawnZone_1.setComponent("position", spawnZone1_Position);
	spawnZone_1.setComponent("object_id", -4);

	GameObject spawnZone_2 = GameObject();
	spawnZone_2.setComponent("position", spawnZone2_Position);
	spawnZone_2.setComponent("object_id", -5);

	// Death Zone
	GameObject deathZone_1 = GameObject();
	deathZone_1.setComponent("position", deathZone1_Position);
	deathZone_1.setComponent("object_id", -6);
	deathZone_1.setComponent("dimensions", deathZone1_Dimensions);

	localObjects.push_back(&player);
	localObjects.push_back(&platform_1);
	localObjects.push_back(&platform_2);
	localObjects.push_back(&platform_3);
	localObjects.push_back(&spawnZone_1);
	localObjects.push_back(&spawnZone_2);
	localObjects.push_back(&deathZone_1);

	// Setting Gravity
	WorldPhysics::setGravity(gravity);

	// Time Line Setup
	Timeline timeline;


	////////////////////
	// Network Setup
	////////////////////
	NetworkManager myNetwork;

	// Start the client
	const int SUBSCRIBE_PORT = 5556;
	const int HANDSHAKE_PORT = 5557;
	const int REQUEST_BASE_PORT = 5600;

	int REPLY_PORT;

	int myID = myNetwork.connectAndHandshake("localhost", HANDSHAKE_PORT, REPLY_PORT);
	if (myID == -1) {
		std::cerr << "Client Failed to be created" << std::endl;
		exit(1);
	}
	player.setComponent("client_id", myID);

	myNetwork.startClient("localhost", REQUEST_BASE_PORT + myID, SUBSCRIBE_PORT, objectList, objectMutex);

	//////////////////////////////////////////////////
	//
	// Multi Threading
	//
	//////////////////////////////////////////////////

	// Start Networking Thread
	std::thread netThread(&networking, &myNetwork, &objectList, &objectMutex, &player);

	// Start Rendering Thread
	std::thread renderThread(&rendering, &objectList, &objectMutex, &player, &localObjects);

	std::cout << "Started multi threading" << myID << std::endl;



	//////////////////////////////////////////////////
	//
	// Main Gameplay Loop
	//
	//////////////////////////////////////////////////

	// Main game loop condition variable
	bool running = true;

	// SDL_Event to capture event of window being closed
	SDL_Event event;

    bool constantSizeScale = true;

    // Start up threads
    std::thread sendThread(sendPlayerThread, &running, &networkManager, &clientObjectList, &clientObjectListMutex, localEventList, &localEventCt, &localEventMut, clientId);
    std::thread receiveThread(receiveUpdateThread, &running, &networkManager, &clientObjectList, &clientObjectListMutex);

    bool firstLoop = true;
    int loopSpeed = (int)16 * (1 / mainTimeline.getTimeScale());

    while (running)
    {
        mainTimeline.update();
        loopSpeed = (int)16 * (1 / mainTimeline.getTimeScale());
        std::this_thread::sleep_for(std::chrono::milliseconds(loopSpeed));

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                running = false;

            if (isKeyPressed(SDL_SCANCODE_ESCAPE))
                running = false;

            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                // Constant Scaling
                if (constantSizeScale)
                {
                    // Resize as if the screen was still the same
                    SDL_SetRenderLogicalPresentation(renderer, 1920, 1080, SDL_LOGICAL_PRESENTATION_STRETCH);
                }
                // Proportional Scaling
                else
                {
                    // Get Window Size
                    int w, h;
                    SDL_GetWindowSize(window, &w, &h);
                    // Resize as if the screen was still the same
                    SDL_SetRenderLogicalPresentation(renderer, w, h, SDL_LOGICAL_PRESENTATION_STRETCH);
                }
            }

            if (isKeyPressed(SDL_SCANCODE_GRAVE))
            {
                constantSizeScale = !constantSizeScale;
            }
        }

        std::lock_guard<std::mutex> lock(clientObjectListMutex);

        // --- Link to Local Player ---
        if (!localPlayer)
        {
            localPlayer = findGameObjectByClientId(clientId, clientObjectList);
        }

        if (localPlayer)
        {
            if (firstLoop)
            {
                auto deathEvent = std::make_shared<DeathEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), clientId);
                eventManager.QueueEvent(deathEvent);
                std::lock_guard<std::mutex> lock2(localEventMut);
                localEventList[localEventCt] = NetworkEvent{3, -1, clientId, -1, -1, -1, deathEvent->timestamp};
                localEventCt++;
            }
            localPlayer->setComponent("velocity", Vector{localPlayer->getComponent<Vector>("velocity").x, 100});
            if (isKeyPressed(SDL_SCANCODE_A))
            {
                auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), 1, clientId);
                eventManager.QueueEvent(inputEvent);
                std::lock_guard<std::mutex> lock2(localEventMut);
                localEventList[localEventCt] = NetworkEvent{2, ACTION_MOVE_LEFT, clientId, -1, -1, -1, inputEvent->timestamp};
                localEventCt++;
            }
            if (isKeyPressed(SDL_SCANCODE_D))
            {
                auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), 2, clientId);
                eventManager.QueueEvent(inputEvent);
                std::lock_guard<std::mutex> lock2(localEventMut);
                localEventList[localEventCt] = NetworkEvent{2, ACTION_MOVE_RIGHT, clientId, -1, -1, -1, inputEvent->timestamp};
                localEventCt++;
            }
            if (isKeyPressed(SDL_SCANCODE_SPACE))
            {
                auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), 3, clientId);
                eventManager.QueueEvent(inputEvent);
                std::lock_guard<std::mutex> lock2(localEventMut);
                localEventList[localEventCt] = NetworkEvent{2, ACTION_JUMP, clientId, -1, -1, -1, inputEvent->timestamp};
                localEventCt++;
            }

            Vector cameraPos = cameraObject->getComponent<Vector>("position");
            Vector playerPos = localPlayer->getComponent<Vector>("position");

            float xDifference = playerPos.x - cameraPos.x - (SCREEN_W / 2.0f);
            cameraPos.x += xDifference * 0.05f;

            float yDifference = playerPos.y - cameraPos.y - (SCREEN_H / 2.0f);
            cameraPos.y += yDifference * 0.05f;

            if (cameraPos.x < 0.0f)
                cameraPos.x = 0.0f;
            if (cameraPos.y < 0.0f)
                cameraPos.y = 0.0f;
            if (cameraPos.x > WORLD_WIDTH - SCREEN_W)
                cameraPos.x = WORLD_WIDTH - SCREEN_W;
            if (cameraPos.y > WORLD_HEIGHT - SCREEN_H)
                cameraPos.y = WORLD_HEIGHT - SCREEN_H;

            cameraObject->setComponent("position", cameraPos);
        }

        if (localPlayer->hasComponent("position") && localPlayer->hasComponent("dimensions") && platform->hasComponent("position") && platform->hasComponent("dimensions") && overlappingColliders1(*localPlayer, *platform))
        {
            auto collisionEvent = std::make_shared<CollisionEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), clientId, platform->getComponent<int>("object_id"));
            eventManager.QueueEvent(collisionEvent);
            std::lock_guard<std::mutex> lock2(localEventMut);
            localEventList[localEventCt] = NetworkEvent{1, -1, clientId, -3, -1, -1, collisionEvent->timestamp};
            localEventCt++;
        }
        // If collision with death zone, respawn player
        if (localPlayer->hasComponent("position") && localPlayer->hasComponent("dimensions") && overlappingColliders1(*localPlayer, *deathZone))
        {
            auto collisionEvent = std::make_shared<CollisionEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), clientId, -2);
            eventManager.QueueEvent(collisionEvent);
            std::lock_guard<std::mutex> lock2(localEventMut);
            localEventList[localEventCt] = NetworkEvent{1, -1, clientId, -2, -1, -1, collisionEvent->timestamp};
            localEventCt++;
        }
        eventManager.ProcessEvents(std::chrono::steady_clock::now().time_since_epoch().count());

        localPlayer->setComponent("position", Vector{localPlayer->getComponent<Vector>("position").x, localPlayer->getComponent<Vector>("position").y + localPlayer->getComponent<Vector>("velocity").y * mainTimeline.getTimeScale() * mainTimeline.getDeltaTime()});

        setupScreen(renderer);
        {
            Vector cameraPos = cameraObject->getComponent<Vector>("position");
            for (auto &obj : clientObjectList)
            {
                if (!obj->hasComponent("texture"))
                {
                    if (obj->hasComponent("is_player"))
                        obj->setComponent("texture", playerTexture);
                    else if (obj->hasComponent("is_platform"))
                        obj->setComponent("texture", platformTexture);
                }
                if (obj->hasComponent("texture") && obj->hasComponent("position") && obj->hasComponent("dimensions"))
                {
                    SDL_Texture *tex = obj->getComponent<SDL_Texture *>("texture");
                    Vector pos = obj->getComponent<Vector>("position");
                    Vector dim = obj->getComponent<Vector>("dimensions");

                    float screenX = pos.x - cameraPos.x - (dim.x / 2.0f);
                    float screenY = pos.y - cameraPos.y - (dim.y / 2.0f);

                    SDL_FRect destRect = {screenX, screenY, dim.x, dim.y};
                    SDL_RenderTexture(renderer, tex, NULL, &destRect);
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock3(clientEventListMutex);
            clientEventList.clear();
        }
        refreshScreen(renderer);
        firstLoop = false;
    }
    
    // Clean up
    running = false;
    sendThread.join();
    receiveThread.join();

    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(platformTexture);

    for (auto &obj : clientObjectList)
    {
        delete obj;
    }
    clientObjectList.clear();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}