
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/collisions.h"
#include "../engine/headers/input.h"
#include "../engine/headers/network.h"
#include "../engine/headers/protocol.h"
#include "../engine/headers/entities.h"
#include "../engine/headers/input.h"
#include "../engine/headers/timeline.h"
#include "../engine/headers/GameObject.h"
#include "../engine/headers/gameUtils.h"
#include "../engine/headers/EventManager.h"
#include "../engine/headers/recording.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <utility>


// Initialize the window and renderer using SDL method
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// Window Variables
Vector windowSize = {720, 540};


///////////////////////////
//	Defaults
///////////////////////////

// Player
Vector player_Position = {100, 100};
Vector player_Dimensions;
SDL_Texture* player_Texture;
Vector player_Velocity = {0, 0};

// Platforms
Vector platform1_Position = {100, 400};
Vector platform2_Position = {400, 400};
Vector platform3_Position = {700, 400};

SDL_Texture* platform_Texture;
Vector platform_Dimensions;;

// Spawn Zones
Vector spawnZone1_Position = {100, 100};
Vector spawnZone2_Position = {700, 100};

// DeathZones
Vector deathZone1_Position = {100, 900};
Vector deathZone1_Dimensions = {700, 100};

// Game Vars
float playerSpeed = 300.0;
float playerJumpSpeed = 300.0;

int gravity = 200;

bool isPaused = false;


// Multiplayer Data
std::thread myNetworkThread;
std::mutex playersMutex;
std::mutex npcMutex;
NetworkManager myNetwork;
std::map<int, std::unique_ptr<GameObject>> remotePlayers;
std::map<int, std::unique_ptr<GameObject>> remoteNPCs;
Vector remoteMovPlat_Pos;

SDL_Texture* playerTex;




//////////////////////////////////////////////////
//
// Helper Functions
//
//////////////////////////////////////////////////

// Input Enum
enum Direction {
    UP,
    DOWN,
	LEFT,
	RIGHT,
	FAR_LEFT
};

// Texture valid check
void texCheck(SDL_Texture* tex) {
	if (!tex) {
		SDL_Log("Could not load image: %s", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		exit(1);
	}
}

bool collidable(GameObject *a) {
	if (a->hasComponent("position") && a->hasComponent("dimensions")) return true;
	return false;
}

void renderObj(GameObject *object, Vector offset) {
	// Logic for adding dimensions and textures to remove players
	if (object->hasComponent("is_player") && (!object->hasComponent("dimensions") || !object->hasComponent("texture"))) {
		object->setComponent("dimensions", player_Dimensions);
		object->setComponent("texture", player_Texture);
	}

	// Logic for adding dimensions and textures to npcs
	if (object->hasComponent("is_npc") && (!object->hasComponent("dimensions") || !object->hasComponent("texture"))) {
		object->setComponent("dimensions", platform_Dimensions);
		object->setComponent("texture", platform_Texture);
	}



	// Only Render Objects with Positions and Dimensions, and a color or texture
	// (!object->hasComponent("color") && !object->hasComponent("texture"))
	if (!object->hasComponent("position") || !object->hasComponent("dimensions")) {
		return;
	}

	Vector pos = object->getComponent<Vector>("position");
	Vector dim = object->getComponent<Vector>("dimensions");

	// Create Rectangle to render Obj
	SDL_FRect rectangle = { pos.x - offset.x + 700, pos.y - offset.y  + 500, dim.x, dim.y};

	// Object has Texture
	if (object->hasComponent("texture")) {
		SDL_Texture* tex = object->getComponent<SDL_Texture *>("texture");
		SDL_RenderTexture(renderer, tex, NULL, &rectangle);
	}
	// Object has Color
	else if (object->hasComponent("color")) {
		SDL_Color color = object->getComponent<SDL_Color>("color");
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		SDL_RenderRect(renderer, &rectangle);
	}
	// Object has no color or texture
	else {
		SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
		SDL_RenderFillRect(renderer, &rectangle);
	}
}


void printList(std::vector<GameObject *> *objectList) {
	for (const auto& obj : *objectList) {
		if (obj->hasComponent("client_id")) {
			std::cout << "Client Id:" << obj->getComponent<int>("client_id") << std::endl;
		}
	}
}



//////////////////////////////////////////////////
//
// Multithreading
//
/////////////////\/////////////////////////////////

// Rendering Thread
void rendering(bool *playingRecording, std::vector<GameObject *> *objectList, std::mutex *objectMutex, GameObject *player, std::vector<GameObject *> *localObjectList) {
	// Loop Forever
	while (true) {
		if (!*playingRecording) {
			// Setup the Screen
			setupScreen(renderer);

			{
				// Lock, rendering remote objects
				std::lock_guard<std::mutex> lock(*objectMutex);

				// Get Player Position for offsetting others
				Vector offset = player->getComponent<Vector>("position");


				// Render Local Objects
				for (const auto &object : *localObjectList) {
					renderObj(object, offset);
				}

				// Render Each Object
				for (const auto &object : *objectList) {
					if (object->hasComponent("client_id") && object->getComponent<int>("client_id") == player->getComponent<int>("client_id")) continue;
					renderObj(object, offset);
				}
			}
		}

		// Wait
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}


	// Clear screen
	// SDL_RenderClear(renderer);
	
	// refreshScreen(renderer);
}

// Network Thread
void networking(NetworkManager *myNetwork, std::vector<GameObject *> *objectList, std::mutex *objectMutex, GameObject *localPlayer, NetworkEvent* localEvents, int *numEvents, std::mutex* eventMutex) {
	// Loop Forever
	while (true) {
		PlayerState myPlayerState;
		{
			Vector pos = localPlayer->getComponent<Vector>("position");
			myPlayerState.clientId = localPlayer->getComponent<int>("client_id");
			myPlayerState.x = pos.x;
			myPlayerState.y = pos.y; 
		}

		{
            std::lock_guard<std::mutex> lock(*eventMutex);
            myPlayerState.num_events = (*numEvents);
            for(int i = 0; i < myPlayerState.num_events; i++) {
                myPlayerState.events[i] = localEvents[i];
            }
            
            (*numEvents) = 0;
        }

		myNetwork->sendPlayerState(myPlayerState);


		// Get correct Object List
		myNetwork->update();

		


		// Wait
		std::this_thread::sleep_for(std::chrono::milliseconds(33));
	}
}

//////////////////////////////////////////////////
//
// Main Function
//
//////////////////////////////////////////////////
int main(int argc, char* argv[])
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
	std::vector<std::vector<GameObject *> *> masterObjectList;
    std::vector<GameObject *> objectList;
	std::vector<GameObject *> localObjects;
    std::mutex objectMutex;

	masterObjectList.push_back(&objectList);
	masterObjectList.push_back(&localObjects);

	// Player
	GameObject player;
	player.setComponent("position", player_Position);
	player.setComponent("is_local_player", true);
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
	//`Event Setup
	////////////////////
	std::vector<std::shared_ptr<Event>> eventList;
    std::mutex eventMutex;

	NetworkEvent networkEvents[32];
    std::mutex networkEventMutex;
	int numNetworkEvents = 0;

	std::cout << "Setup" << std::endl;

	EventManager eventManager;
	bool *playingRecording = new bool(false);

	// Collision Event
	eventManager.RegisterListener(CollisionEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
		const auto &collision = static_cast<const CollisionEvent &>(e);

		// Player collides with platform
		if (collision.objectB_ID == -1 || collision.objectB_ID == -2 || collision.objectB_ID == -3 || collision.objectB_ID == 1 || collision.objectB_ID == 2) {
			// Set downward velocity to 0
			player.setComponent("velocity", Vector{player.getComponent<Vector>("velocity").x, -WorldPhysics::getGravity() * timeline.getDeltaTime()});
		}

		// Player collides with Death Zone
		if (collision.objectB_ID == -6) {
			auto deathEvent = std::make_shared<DeathEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), collision.objectA_ID);
			eventManager.QueueEvent(deathEvent);
		}
	});

	// Death Event
	eventManager.RegisterListener(DeathEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
		const auto &death = static_cast<const DeathEvent &> (e);

		// Create Spawn point for the player
		if (death.entityID % 2 == 0) {
			auto spawnEvent = std::make_shared<SpawnEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), death.entityID, spawnZone1_Position.x, spawnZone1_Position.y);
        	eventManager.QueueEvent(spawnEvent);
		}
		else {
			auto spawnEvent = std::make_shared<SpawnEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), death.entityID, spawnZone2_Position.x, spawnZone2_Position.y);
        	eventManager.QueueEvent(spawnEvent);
		}
	});

	// Spawn Event
	eventManager.RegisterListener(SpawnEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
		const auto &spawn = static_cast<const SpawnEvent &>(e);

		player.setComponent("position", Vector(spawn.x, spawn.y));
	});

	// Input Event
	eventManager.RegisterListener(InputEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
		const auto &input = static_cast<const InputEvent &>(e);

		if (input.playerID == player.getComponent<int>("client_id")) {
			Vector playerVel = player.getComponent<Vector>("velocity");

			switch (input.action) {
				case UP:
					player.setComponent("velocity", Vector{playerVel.x, -playerJumpSpeed * timeline.getDeltaTime()});
					break; 

				case DOWN:
					player.setComponent("velocity", Vector{playerVel.x, playerSpeed * timeline.getDeltaTime()});
					break;

				case LEFT:
					player.setComponent("velocity", Vector{-playerSpeed * timeline.getDeltaTime(), playerVel.y});
					break;

				case RIGHT:
					player.setComponent("velocity", Vector{playerSpeed * timeline.getDeltaTime(), playerVel.y});
					break;
					
				case FAR_LEFT:
					player.setComponent("velocity", Vector{-playerSpeed * 30 * timeline.getDeltaTime(), playerVel.y});
					break;
			}
		}
	});

	std::cout << "Events" << std::endl;




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

	myNetwork.startClient("localhost", REPLY_PORT, SUBSCRIBE_PORT, objectList, objectMutex, eventList, eventMutex);

	std::cout << "Network Up" << std::endl;



	///////////////////////
	//	Recording Manager
	///////////////////////
	RecordingManager recordingManager(renderer, &eventManager, myID, &masterObjectList, &objectMutex, renderObj);

	// Register Recording events
	eventManager.RegisterListener(StartRecordingEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
		recordingManager.startRecording();
	});

	eventManager.RegisterListener(StopRecordingEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
		recordingManager.stopRecording();
	});

	eventManager.RegisterListener(StartPlaybackEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
		*playingRecording = true;
		recordingManager.startPlayback();
	});

	eventManager.RegisterListener(StopPlaybackEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
		*playingRecording = false;
	});

	//////////////////////////////////////////////////
	//
	// Multi Threading
	//
	//////////////////////////////////////////////////

	// Start Networking Thread
	std::thread netThread(&networking, &myNetwork, &objectList, &objectMutex, &player, networkEvents, &numNetworkEvents, &networkEventMutex);

	// Start Rendering Thread
	std::thread renderThread(&rendering, playingRecording, &objectList, &objectMutex, &player, &localObjects);

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

	// Scaling Type bool
	bool constantSizeScale = true;

	bool justPressed1 = false;
	bool justPressed3 = false;

	// The main game loop
	while (running) {

		// std::cout << eventList.size() << std::endl;

		// Time Line Update
		timeline.update();
		float d_time = timeline.getDeltaTime();

		{
			std::lock_guard<std::mutex> lock(eventMutex);

			for (auto &evt : eventList) {
				std::cout << "Queued network event" << std::endl;
				eventManager.QueueEvent(evt);
			}

			eventList.clear(); // clear after dispatch
		}


		// Poll for events
		while (SDL_PollEvent(&event)) {

			// If event is Window Resize
			if (event.type == SDL_EVENT_WINDOW_RESIZED) {
				// Constant Scaling
				if (constantSizeScale) {
					// Resize as if the screen was still the same
					SDL_SetRenderLogicalPresentation(renderer, 1920, 1080, SDL_LOGICAL_PRESENTATION_STRETCH);
				}
				// Proportional Scaling
				else {
					//Get Window Size
					int w, h;
					SDL_GetWindowSize(window, &w, &h);
					// Resize as if the screen was still the same
					SDL_SetRenderLogicalPresentation(renderer, w, h, SDL_LOGICAL_PRESENTATION_STRETCH);
				}	
			}

			// Read input from input manager
			// If the event is close the window
			if (event.type == SDL_EVENT_QUIT)
				running = false;

			// Otherwise look for a key press
			else if (event.type == SDL_EVENT_KEY_DOWN) {
				// Testing to allow the keypress of "ESC" to exit the window.
				if (isKeyPressed(SDL_SCANCODE_ESCAPE)) {								// Quit
					running = false;
				}

				// Change Scaling Mode
				if (isKeyPressed(SDL_SCANCODE_GRAVE)) {									// Change Scaling Mode
					constantSizeScale = !constantSizeScale;
				}

				////////////////////////////////
				//	Asyc
				////////////////////////////////
				// Slow down game
				if (isKeyPressed(SDL_SCANCODE_COMMA)){
					timeline.setTimeScale(0.5);
				}
				// Regular Speed
				if (isKeyPressed(SDL_SCANCODE_PERIOD)) {
					timeline.setTimeScale(1.0);
				}
				// Speed Up
				if (isKeyPressed(SDL_SCANCODE_SLASH)) {
					timeline.setTimeScale(2.0);
				}
				// Pause
				if (isKeyPressed(SDL_SCANCODE_P)) {
					//Unpause
					if (isPaused) {
						timeline.setTimeScale(1.0);
						isPaused = false;
					}
					//Pause
					else {
						timeline.setTimeScale(0.0);
						isPaused = true;
					}
					
				}
			}
		}

		//////////////////////////////////////////////////
		//
		// Gameplay Updates
		//
		//////////////////////////////////////////////////





		//////////////////////////////////////////////////
		//
		// Player Movement
		//
		//////////////////////////////////////////////////
		// Player Movement
		Vector vel = player.getComponent<Vector>("velocity");
		if (isKeyPressed(SDL_SCANCODE_W) || isKeyPressed(SDL_SCANCODE_SPACE)) {	// Jump
			{
				auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), UP, myID);
				eventManager.QueueEvent(inputEvent);

				std::lock_guard<std::mutex> lock(networkEventMutex);
				networkEvents[numNetworkEvents] = NetworkEvent {2, UP, myID, -1, -1, -1, inputEvent->timestamp};
			}
		}
		if (isKeyPressed(SDL_SCANCODE_A)) {										// Left
			{
				auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), LEFT, myID);
				eventManager.QueueEvent(inputEvent);

				std::lock_guard<std::mutex> lock(networkEventMutex);
				networkEvents[numNetworkEvents] = NetworkEvent {2, LEFT, myID, -1, -1, -1, inputEvent->timestamp};
			}
		}
		if (isKeyPressed(SDL_SCANCODE_S)) {										// Down
			{
				auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), DOWN, myID);
				eventManager.QueueEvent(inputEvent);

				std::lock_guard<std::mutex> lock(networkEventMutex);
				networkEvents[numNetworkEvents] = NetworkEvent {2, DOWN, myID, -1, -1, -1, inputEvent->timestamp};
			}
		}
		if (isKeyPressed(SDL_SCANCODE_D)) {										// Right
			{
				auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), RIGHT, myID);
				eventManager.QueueEvent(inputEvent);

				std::lock_guard<std::mutex> lock(networkEventMutex);
				networkEvents[numNetworkEvents] = NetworkEvent {2, RIGHT, myID, -1, -1, -1, inputEvent->timestamp};
			}
		}

		// std::cout << isKeyPressed(SDL_SCANCODE_1) << std::endl;

		// Recording Input
		if (isKeyPressed(SDL_SCANCODE_1)) {
			
			if (!justPressed1) {
				justPressed1 = true;
				//fire event for pressing
				std::cout << "Pressed 1!" << std::endl;

				auto startRecordEvent = std::make_shared<StartRecordingEvent>(std::chrono::steady_clock::now().time_since_epoch().count());
				eventManager.QueueEvent(startRecordEvent);
			}
			// fire events for holding			
		}
		else justPressed1 = false;

		if (isKeyPressed(SDL_SCANCODE_2)) {
			auto stopRecordEvent = std::make_shared<StopRecordingEvent>(std::chrono::steady_clock::now().time_since_epoch().count());
			eventManager.QueueEvent(stopRecordEvent);
		}
		if (isKeyPressed(SDL_SCANCODE_3)) {
			if (!justPressed3) {
				justPressed3 = true;

				std::cout << "Pressed 3!" << std::endl;

				auto startPlaybackEvent = std::make_shared<StartPlaybackEvent>(std::chrono::steady_clock::now().time_since_epoch().count());
				eventManager.QueueEvent(startPlaybackEvent);
			}
		}
		else justPressed3 = false;

		// Add Gravity
		Vector playerSpeed = player.getComponent<Vector>("velocity");
		player.setComponent("velocity", Vector {playerSpeed.x, playerSpeed.y + WorldPhysics::getGravity() * d_time});

		//////////////////////////////////////////////////
		//
		// Collisions
		//
		//////////////////////////////////////////////////
		
		// Check Local Platforms
		if (collidable(&player) && collidable(&platform_1) && collidable(&platform_2) && collidable(&platform_3)) {				
			if (overlappingColliders1(player, platform_1) || overlappingColliders1(player, platform_2) || overlappingColliders1(player, platform_3)) {				
				auto collisionEvent = std::make_shared<CollisionEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), myID, platform_1.getComponent<int>("npc_id"));
				eventManager.QueueEvent(collisionEvent);
			}
		}

		// Check Death Zones
		if (collidable(&player) && collidable(&deathZone_1)) {
			if (overlappingColliders1(player, deathZone_1)) {
				auto collisionEvent = std::make_shared<CollisionEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), myID, deathZone_1.getComponent<int>("object_id"));
				eventManager.QueueEvent(collisionEvent);
			}
		}

		// Check Remote Platforms
		{
			// Lock, remote objects
			std::lock_guard<std::mutex> lock(objectMutex);

			// Check if player is not coliding with anything
			GameObject *movingPlatformHorizontal = findGameObjectByNpcId(1, objectList);
			GameObject *movingPlatformVertical = findGameObjectByNpcId(2, objectList);

			// Do we have the remote objects
			if (movingPlatformHorizontal && movingPlatformVertical) {
				if (collidable(movingPlatformHorizontal) && collidable(movingPlatformVertical)) {
					if (overlappingColliders1(player, *movingPlatformHorizontal) || overlappingColliders1(player, *movingPlatformVertical)) {	
						auto collisionEvent = std::make_shared<CollisionEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), myID, movingPlatformHorizontal->getComponent<int>("npc_id"));
						eventManager.QueueEvent(collisionEvent);
					}
				}
			}
			else {
				std::cout << "findGameObject Failed" << std::endl;
			}

		}
		
		
		eventManager.ProcessEvents(std::chrono::steady_clock::now().time_since_epoch().count());
		

		// Move player and then reset their velocity
		updatePosition(player, isPaused);
		player.setComponent("velocity", Vector{0, 0});




		//////////////////////////////////////////////////
		//
		// Rendering
		//
		//////////////////////////////////////////////////			
		// // Render Local Player

		refreshScreen(renderer);
		
		// std::cout << "Refresh, end of loop" << std::endl;

		// {
        //     std::lock_guard<std::mutex> lock(eventMutex);
        //     eventList.clear();
        // }
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}