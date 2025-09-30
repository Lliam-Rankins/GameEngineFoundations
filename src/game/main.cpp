
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

// Initialize the window and renderer using SDL method
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// Window Variables
Vector windowSize = {1440, 1080};

// Starting Positions
Vector playerPos = {100, 100};
Vector platformPos_1 = {100, 400};
Vector movingPlatPos_1 = {500, 600};

// Defaults
Vector defaultVel = {0, 0};


// Game Vars
float playerSpeed = 300.0;
float playerJumpSpeed = 300.0;
float movingPlatSpeed = 20;

int gravity = 200;

bool isPaused = false;


// Multiplayer Data
std::thread myNetworkThread;
PlayerState myPlayerState;


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
void gameStateChange(NetworkManager myNetwork) {
	// Loop infinetly
	// while (true) {
	// 	myNetwork.
	// }
}

void getInput()

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
	SDL_Texture* playerTex = IMG_LoadTexture(renderer, "../media/darkworld_character_morwen_idle.png");
	texCheck(playerTex);

	SDL_Texture* brickTex = IMG_LoadTexture(renderer, "../media/darkworld_platform_brick_idle.png");
	texCheck(brickTex);

	// Create Dimensions
	Vector playerDim = {playerTex->w, playerTex->h};
	Vector brickDim = {brickTex->w, brickTex->h};

	// Entity creation
	Entity player(playerPos, playerDim, playerTex, true, defaultVel);

	Entity platform_1(platformPos_1, brickDim, brickTex, false, defaultVel);
	Entity movingPlat_1(movingPlatPos_1, brickDim, brickTex, false, {movingPlatSpeed, 0}); //Moves to the Right


	//Creating and Setting Colliders
	Collider playerCol(0, 0, 0, 0);
	player.setCollider(&playerCol);

	Collider platformCol_1(0, 0, 0, 0);
	platform_1.setCollider(&platformCol_1);

	Collider movingPlatCol_1(0, 0, 0, 0);
	movingPlat_1.setCollider(&movingPlatCol_1);


	// Setting Gravity
	WorldPhysics::setGravity(gravity);


	// Time Line Setup
	Timeline timeline;



	// // Network Setup
	NetworkManager myNetwork;

	// Start the client
	const int REQUEST_PORT = 5555;
	const int SUBSCRIBE_PORT = 5556;
	const int HANDSHAKE_PORT = 5557;

	myNetwork.startClient("localhost", REQUEST_PORT, SUBSCRIBE_PORT);

	// Start the client and receive client id
	int clientId = myNetwork.connectAndHandshake("localhost", HANDSHAKE_PORT);
	if (clientId == -1) {
		std::cerr << "Client Failed to be created" << std::endl;
		exit(1);
	}

	// Start Networking Thread
	// std::thread(&gameStateChange, myNetwork);
	
	



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

		// Update moving platforms position
		if (movingPlat_1.position.x > movingPlatPos_1.x + 100) movingPlat_1.velocity.x = -movingPlatSpeed * d_time;
		if (movingPlat_1.position.x < movingPlatPos_1.x - 100) movingPlat_1.velocity.x = movingPlatSpeed * d_time;
		movingPlat_1.updatePosition(isPaused);

		platform_1.updatePosition(isPaused);


		//////////////////////////////////////////////////
		//
		// Player Movement
		//
		//////////////////////////////////////////////////
		// Player Movement
		if (isKeyPressed(SDL_SCANCODE_W) || isKeyPressed(SDL_SCANCODE_SPACE)) {	// Jump
				player.velocity.y = -playerJumpSpeed * d_time;
				player.updatePosition(isPaused);
		}
		if (isKeyPressed(SDL_SCANCODE_A)) {										// Left
			player.velocity.x = -playerSpeed * d_time;
			player.updatePosition(isPaused);
		}
		if (isKeyPressed(SDL_SCANCODE_S)) {										// Down
			player.velocity.y = playerSpeed * d_time;
			player.updatePosition(isPaused);
		}
		if (isKeyPressed(SDL_SCANCODE_D)) {										// Right
			player.velocity.x = playerSpeed * d_time;
			player.updatePosition(isPaused);
		}

		
		
		// Check if player is coliding with anything
		if (!overlappingColliders(*player.collider, *platform_1.collider) && !overlappingColliders(*player.collider, *movingPlat_1.collider)) {
			player.velocity.y += WorldPhysics::getGravity() * d_time;
		}

		player.updatePosition(isPaused);
		player.velocity = {0, 0};

		//////////////////////////////////////////////////
		//
		// Rendering
		//
		//////////////////////////////////////////////////

		// Setup the Screen
		setupScreen(renderer);

		renderEntity(player);
		renderEntity(platform_1);
		renderEntity(movingPlat_1);

		// Clear screen
		//SDL_RenderClear(renderer);
		
		refreshScreen(renderer);

	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
