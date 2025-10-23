
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

SDL_Texture* movingPlatform_Texture;
Vector movingPlatform_Dimensions;;

// Spawn Zones
Vector spawnZone1_Positon = {100, 100};

// DeathZones
Vector deathZone1_Position = {100, 400};
Vector deathZone1_Dimensions = {300, 300};



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

void renderObj(GameObject *object) {
	// Logic for adding dimensions and textures to remove players
	if (object->hasComponent("is_player") && (!object->hasComponent("dimensions") || !object->hasComponent("texture"))) {
		object->setComponent("dimensions", player_Dimensions);
		object->setComponent("texture", player_Texture);
	}

	// Logic for adding dimensions and textures to npcs
	if (object->hasComponent("is_npc") && (!object->hasComponent("dimensions") || !object->hasComponent("texture"))) {
		object->setComponent("dimensions", movingPlatform_Dimensions);
		object->setComponent("texture", movingPlatform_Texture);
	}



	// Only Render Objects with Positions and Dimensions, and a color or texture
	// (!object->hasComponent("color") && !object->hasComponent("texture"))
	if (!object->hasComponent("position") || !object->hasComponent("dimensions")) {
			std::cout << "Object lacks something" << std::endl;
			return;
		}

	Vector pos = object->getComponent<Vector>("position");
	Vector dim = object->getComponent<Vector>("dimensions");

	// Create Rectangle to render Obj
	SDL_FRect rectangle = { pos.x - dim.x, pos.y - dim.y, dim.x, dim.y};

	// Object has Texture
	if (object->hasComponent("texture")) {
		SDL_Texture* tex = object->getComponent<SDL_Texture *>("texture");
		SDL_RenderTexture(renderer, tex, NULL, &rectangle);
	}
	// Object has Color
	else if (object->hasComponent("color")) {
		SDL_Color color = object->getComponent<SDL_Color>("color");
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
	}
}

//////////////////////////////////////////////////
//
// Multithreading
//
//////////////////////////////////////////////////

// Rendering Thread
void rendering(std::vector<GameObject *> *objectList, std::mutex *objectMutex, GameObject *player, std::vector<GameObject *> *localObjectList) {
	// Loop Forever
	while (true) {
		// // Setup the Screen
		setupScreen(renderer);

		// Render Local Objects
		for (const auto &object : *localObjectList) {
			renderObj(object);
		}

		// For all players and NPCs
		{
			// Lock, rendering remote objects
			std::lock_guard<std::mutex> lock(*objectMutex);

			// Render Each Object
			for (const auto &object : *objectList) {
				if (object->hasComponent("client_id") && object->getComponent<int>("client_id") == player->getComponent<int>("client_id")) continue;
				renderObj(object);
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
void networking(NetworkManager *myNetwork, std::vector<GameObject *> *objectList, std::mutex *objectMutex, GameObject *localPlayer) {
	// Loop Forever
	while (true) {
		// Get correct Object List
		std::cout << "Pre Update" << std::endl;
		myNetwork->update();
		std::cout << "Updated" << std::endl;

		{
			PlayerState myPlayerState;
			Vector pos = localPlayer->getComponent<Vector>("position");
			myPlayerState.clientId = localPlayer->getComponent<int>("client_id");
			myPlayerState.x = pos.x;
			myPlayerState.y = pos.y; 
			myNetwork->sendPlayerState(myPlayerState);

			std::cout << "Send player state" << std::endl;
		}

		// Wait
		std::this_thread::sleep_for(std::chrono::milliseconds(33));
	}
}

void handleMovement() {
	
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

	movingPlatform_Texture = IMG_LoadTexture(renderer, "../media/darkworld_platform_brick_idle.png");
	texCheck(movingPlatform_Texture);

	// Create Dimensions
	player_Dimensions = {player_Texture->w, player_Texture->h};
	movingPlatform_Dimensions = {movingPlatform_Texture->w, movingPlatform_Texture->h};


	// Instantiate ObjectList
    std::vector<GameObject *> objectList;
	std::vector<GameObject *> localObjects;
    std::mutex objectMutex;

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
	platform_1.setComponent("dimensions", movingPlatform_Dimensions);
	platform_1.setComponent("texture", movingPlatform_Texture);

	GameObject platform_2 = GameObject();
	platform_2.setComponent("position", platform2_Position);
	platform_2.setComponent("npc_id", -2);
	platform_2.setComponent("dimensions", movingPlatform_Dimensions);
	platform_2.setComponent("texture", movingPlatform_Texture);

	GameObject platform_3 = GameObject();
	platform_3.setComponent("position", platform3_Position);
	platform_3.setComponent("npc_id", -3);
	platform_3.setComponent("dimensions", movingPlatform_Dimensions);
	platform_3.setComponent("texture", movingPlatform_Texture);

	// Spawn Zones
	GameObject spawnZone_1 = GameObject();
	platform_3.setComponent("position", platform3_Position);
	platform_3.setComponent("object_id", -4);
	// TODO: Change Dimensions
	platform_3.setComponent("dimensions", movingPlatform_Dimensions);

	localObjects.push_back(&player);
	localObjects.push_back(&platform_1);
	localObjects.push_back(&platform_2);
	localObjects.push_back(&platform_3);

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

	// Scaling Type bool
	bool constantSizeScale = true;

	// The main game loop
	while (running) {

		// Time Line Update
		timeline.update();
		float d_time = timeline.getDeltaTime();


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
			vel.y += -playerJumpSpeed * d_time;
		}
		if (isKeyPressed(SDL_SCANCODE_A)) {										// Left
			vel.x += -playerSpeed * d_time;
		}
		if (isKeyPressed(SDL_SCANCODE_S)) {										// Down
			vel.y += playerJumpSpeed * d_time;
		}
		if (isKeyPressed(SDL_SCANCODE_D)) {										// Right
			vel.x += playerSpeed * d_time;	
		}

		//////////////////////////////////////////////////
		//
		// Collisions
		//
		//////////////////////////////////////////////////
		{
			// // Check if player is not coliding with anything
			GameObject *tempMovingPlatform1 = findGameObjectByNpcId(1, objectList);
			
			if (tempMovingPlatform1) {
				std::cout << "Collisions: " << tempMovingPlatform1->hasComponent("dimensions") << std::endl;				
				if (!overlappingColliders1(player, platform_1) && !overlappingColliders1(player, *tempMovingPlatform1)) {
					vel.y += WorldPhysics::getGravity() * d_time;
				}
			}	
		}
		
		

		

		// Move player and then reset their velocity
		player.setComponent("velocity", vel);
		updatePosition(player, isPaused);
		player.setComponent("velocity", player_Velocity);



		//////////////////////////////////////////////////
		//
		// Rendering
		//
		//////////////////////////////////////////////////			
		// // Render Local Player

		refreshScreen(renderer);
		
		// std::cout << "Refresh, end of loop" << std::endl;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
