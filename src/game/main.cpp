
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

// Starting Positions
Vector playerPos = {100, 100};
Vector platformPos_1 = {100, 400};
Vector movingPlatPos_1 = {500, 600};

// Defaults
Vector defaultVel = {0, 0};


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

// Render entity
void renderEntity(GameObject& e) {
	if (e.hasComponent("Position") && e.hasComponent("Dimensions") && e.hasComponent("tex")) {
		Vector pos = e.getComponent<Vector>("Position");
		Vector dim = e.getComponent<Vector>("Dimensions");
		SDL_Texture* tex = e.getComponent<SDL_Texture *>("tex");

		SDL_FRect rectangle = { pos.x - dim.x, pos.y - dim.y, dim.x, dim.y};
		SDL_RenderTexture(renderer, tex, NULL, &rectangle);
	}
}


//////////////////////////////////////////////////
//
// Multithreading
//
//////////////////////////////////////////////////

// Rendering Thread
void rendering(GameObject *player, GameObject *movingPlat, GameObject *plat, int *myID) {
	// Loop Forever
	while (true) {
		// Setup the Screen
		setupScreen(renderer);

		// For all players and NPCs
		renderEntity(*player);
		{
			// Lock
			std::unique_lock<std::mutex> cv_lock(playersMutex);
			for (const auto& pair : remotePlayers) {
				if (pair.first != *myID) renderEntity(*pair.second);
			}

			renderEntity(*movingPlat);
		}
		renderEntity(*plat);


		// Wait
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

// Network Thread
void networking(NetworkManager *myNetwork, int *myID, GameObject *localPlayer, GameObject *movingPlat) {
	// Loop Forever
	while (true) {
		PlayerState myPlayerState;

		Vector pos = localPlayer->getComponent<Vector>("Position");
		myPlayerState.clientId = *myID;
		myPlayerState.x = pos.x;
		myPlayerState.y = pos.y; 

		myNetwork->sendPlayerState(myPlayerState);
		myNetwork->update();
		
		auto newGameState = myNetwork->getLatestGameState();



		std::cout << "Got Game State" << std::endl;

		// Check to make sure that there is a game state
		if (!newGameState.has_value()) continue;

		GameState gameState = newGameState.value();

		std::cout << "Non Null Game State" << std::endl;


		//Update remotePlayers
		{
			// Lock
			std::unique_lock<std::mutex> cv_lock(playersMutex);

			// For every client
			// TODO: will need to remove player when i stop receiving their information from server
			for (int i = 0; i < gameState.num_clients; i++) {
				// Grab player i
				PlayerState playerState = gameState.players[i];

				// Not me
				if (playerState.clientId == *myID) continue;

				Vector remotePlayerPosition = {playerState.x, playerState.y};

				// Otherwise, Update or create new player
				if (remotePlayers.find(playerState.clientId) == remotePlayers.end()) {	
					// Make new Player Game Object 
					remotePlayers[i] = std::make_unique<GameObject>();
					remotePlayers[i]->setComponent("Position", remotePlayerPosition);
				}
				else {
					if (!isPaused) remotePlayers[i]->setComponent("Position", remotePlayerPosition);
				}
			}
		}

		std::cout << "Read In Players" << std::endl;

		//Update NPCs
		{
			// Lock
			std::unique_lock<std::mutex> cv_lock(npcMutex);

			NPCState movingPlatformState = gameState.npcs[0];
			std::cout << movingPlatformState.x << std::endl;

			// Making Sure we arent paused
			Vector movingPlatformPosition = {movingPlatformState.x, movingPlatformState.y};

			std::cout << "Updating Platform Pos" << std::endl;
			if (!isPaused) movingPlat->setComponent("Position", movingPlatformPosition);
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
	playerTex = IMG_LoadTexture(renderer, "../media/darkworld_character_morwen_idle.png");
	texCheck(playerTex);

	SDL_Texture* brickTex = IMG_LoadTexture(renderer, "../media/darkworld_platform_brick_idle.png");
	texCheck(brickTex);

	// Create Dimensions
	Vector playerDim = {playerTex->w, playerTex->h};
	Vector brickDim = {brickTex->w, brickTex->h};

	// Entity creation
	// players[0] = std::make_unique<Entity>(playerPos, playerTex, true);
	GameObject player = GameObject();
	player.setComponent("Position", playerPos);
	player.setComponent("Velocity", defaultVel);
	player.setComponent("Dimensions", playerDim);
	player.setComponent("tex", playerTex);
	player.setComponent("physics", true);

	GameObject platform_1 = GameObject();
	platform_1.setComponent("Position", platformPos_1);
	platform_1.setComponent("Dimensions", brickDim);
	platform_1.setComponent("tex", brickTex);
	platform_1.setComponent("physics", false);

	GameObject movingPlat_1 = GameObject();
	movingPlat_1.setComponent("Position", movingPlatPos_1);
	movingPlat_1.setComponent("Velocity", defaultVel);
	movingPlat_1.setComponent("Dimensions", brickDim);
	movingPlat_1.setComponent("tex", brickTex);
	movingPlat_1.setComponent("physics", false);

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

	int myID = myNetwork.connectAndHandshake("localhost", HANDSHAKE_PORT);
	if (myID == -1) {
		std::cerr << "Client Failed to be created" << std::endl;
		exit(1);
	}

	std::cout << "Client Id: " << myID << std::endl;
	myNetwork.startClient("localhost", REQUEST_BASE_PORT + myID, SUBSCRIBE_PORT);
	

	//////////////////////////////////////////////////
	//
	// Multi Threading
	//
	//////////////////////////////////////////////////

	// Start Networking Thread
	std::thread netThread(&networking, &myNetwork, &myID, &player, &movingPlat_1);

	// Start Rendering Thread
	std::thread renderThread(&rendering, &player, &movingPlat_1, &platform_1, &myID);


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
		updatePosition(movingPlat_1, isPaused);

		//////////////////////////////////////////////////
		//
		// Player Movement
		//
		//////////////////////////////////////////////////
		// Player Movement
		Vector vel = player.getComponent<Vector>("Velocity");
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

		
		
		// Check if player is not coliding with anything
		if (!overlappingColliders1(player, platform_1) && !overlappingColliders1(player, movingPlat_1)) {
			vel.y += WorldPhysics::getGravity() * d_time;
		}

		player.setComponent("Velocity", vel);

		// Move player and then reset their velocity
		updatePosition(player, isPaused);
		player.setComponent("Velocity", defaultVel);



		//////////////////////////////////////////////////
		//
		// Rendering
		//
		//////////////////////////////////////////////////

		

			

		// Clear screen
		//SDL_RenderClear(renderer);
		
		refreshScreen(renderer);

	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
