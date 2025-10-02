
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/collisions.h"
#include "../engine/headers/input.h"
#include "../engine/headers/network.h"
#include "../engine/headers/protocol.h"
#include "../engine/headers/entities.h"
#include "../engine/headers/input.h"
#include "../engine/headers/timeline.h"
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
std::map<int, std::unique_ptr<Entity>> remotePlayers;
std::map<int, std::unique_ptr<Entity>> remoteNPCs;
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
void renderEntity(const Entity& e) {
    SDL_FRect rectangle = { e.position.x - e.dimensions.x, e.position.y - e.dimensions.y, e.dimensions.x, e.dimensions.y};
    SDL_RenderTexture(renderer, e.texture, NULL, &rectangle);
}


//////////////////////////////////////////////////
//
// Multithreading
//
//////////////////////////////////////////////////

// Rendering Thread
void rendering(Entity *player, Entity *movingPlat, Entity *plat, int *myID) {
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
void networking(NetworkManager *myNetwork, int *myID, Entity *localPlayer, Entity *movingPlat) {
	// Loop Forever
	while (true) {
		PlayerState myPlayerState;

		Vector pos = localPlayer->getPosition();
		myPlayerState.clientId = *myID;
		myPlayerState.x = pos.x;
		myPlayerState.y = pos.y; 

		myNetwork->sendPlayerState(myPlayerState);
		myNetwork->update();
		
		auto newGameState = myNetwork->getLatestGameState();

		// Check to make sure that there is a game state
		if (!newGameState.has_value()) continue;

		GameState gameState = newGameState.value();

		//Update remotePlayers
		{
			// Lock
			std::unique_lock<std::mutex> cv_lock(playersMutex);

			// For every client
			for (int i = 0; i < gameState.num_clients; i++) {
				// Grab player i
				PlayerState playerState = gameState.players[i];

				// Not me
				if (playerState.clientId == *myID) continue;

				// Otherwise, Update or create new player
				if (remotePlayers.find(playerState.clientId) == remotePlayers.end()) {		
					remotePlayers[i] = std::make_unique<Entity>(playerPos, playerTex, true);
				}
				else {
					if (!isPaused) remotePlayers[i]->setPosition({playerState.x, playerState.y});
				}
			}
		}

		//Update NPCs
		{
			// Lock
			std::unique_lock<std::mutex> cv_lock(npcMutex);

			NPCState movingPlatformState = gameState.npcs[0];

			// Making Sure we arent paused
			if (!isPaused) movingPlat->setPosition({movingPlatformState.x, movingPlatformState.y});
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
	Entity player = Entity(playerPos, playerTex, true);

	Entity platform_1(platformPos_1, brickDim, brickTex, false, defaultVel);
	Entity localMovingPlat(movingPlatPos_1, brickDim, brickTex, false); //Moves to the Right


	//Creating and Setting Colliders
	Collider playerCol(0, 0, 0, 0);
	player.setCollider(&playerCol);

	Collider platformCol_1(0, 0, 0, 0);
	platform_1.setCollider(&platformCol_1);

	Collider movingPlatCol_1(0, 0, 0, 0);
	localMovingPlat.setCollider(&movingPlatCol_1);


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

	// Start the client and receive client id
	

	

	

	//////////////////////////////////////////////////
	//
	// Multi Threading
	//
	//////////////////////////////////////////////////

	// Start Networking Thread
	std::thread netThread(&networking, &myNetwork, &myID, &player, &localMovingPlat);

	// Start Rendering Thread
	std::thread renderThread(&rendering, &player, &localMovingPlat, &platform_1, &myID);


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
		platform_1.updatePosition(isPaused);
		localMovingPlat.updatePosition(isPaused);

		//////////////////////////////////////////////////
		//
		// Player Movement
		//
		//////////////////////////////////////////////////
		// Player Movement
		if (isKeyPressed(SDL_SCANCODE_W) || isKeyPressed(SDL_SCANCODE_SPACE)) {	// Jump
			player.changeVelocity({0, -playerJumpSpeed * d_time});
		}
		if (isKeyPressed(SDL_SCANCODE_A)) {	
			player.changeVelocity({-playerSpeed * d_time, 0});					// Left
		}
		if (isKeyPressed(SDL_SCANCODE_S)) {										// Down
			player.changeVelocity({0, playerJumpSpeed * d_time});
		}
		if (isKeyPressed(SDL_SCANCODE_D)) {										// Right
			player.changeVelocity({playerSpeed * d_time, 0});	
		}

		
		
		// Check if player is not coliding with anything
		if (!overlappingColliders(*player.collider, *platform_1.collider) && !overlappingColliders(*player.collider, *localMovingPlat.collider)) {
			player.changeVelocity({0, WorldPhysics::getGravity() * d_time});
		}

		player.updatePosition(isPaused);
		player.setVelocity({0,0});



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
