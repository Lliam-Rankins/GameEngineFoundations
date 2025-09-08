
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/collisions.h"

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
float playerSpeed = 10.0;
float playerJumpSpeed = 10.0;
float movingPlatSpeed = .5;

int gravity = 9.8;


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

	SDL_Texture* brickTex = IMG_LoadTexture(renderer, "media/brick.png");
	texCheck(brickTex);


	// Entity creation
	Entity player(playerPos, {playerTex->w, playerTex->h}, playerTex, true, defaultVel);

	Entity platform_1(platformPos_1, {brickTex->w, brickTex->h}, brickTex, false, defaultVel);
	Entity movingPlat_1(movingPlatPos_1, {brickTex->w, brickTex->h}, brickTex, false, {movingPlatSpeed, 0}); //Moves to the Right


	//Creating and Setting Colliders
	Collider playerCol(0, 0, 0, 0);
	player.setCollider(&playerCol);

	Collider platformCol_1(0, 0, 0, 0);
	platform_1.setCollider(&platformCol_1);

	Collider movingPlatCol_1(0, 0, 0, 0);
	movingPlat_1.setCollider(&movingPlatCol_1);


	// Setting Gravity
	WorldPhysics::setGravity(gravity);



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

				// Player Movement
				if (isKeyPressed(SDL_SCANCODE_W) || isKeyPressed(SDL_SCANCODE_SPACE)) {	// Jump
					player.velocity.y = -playerJumpSpeed;
					player.updatePosition();
				}
				if (isKeyPressed(SDL_SCANCODE_A)) {										// Left
					player.velocity.x = -playerSpeed;
					player.updatePosition();
				}
				if (isKeyPressed(SDL_SCANCODE_S)) {										// Down
					// Not needed?
				}
				if (isKeyPressed(SDL_SCANCODE_D)) {										// Right
					player.velocity.x = playerSpeed;
					player.updatePosition();
				}

				// Change Scaling Mode
				if (isKeyPressed(SDL_SCANCODE_GRAVE)) {									// Change Scaling Mode
					constantSizeScale = !constantSizeScale;
				}
			}
		}

		//////////////////////////////////////////////////
		//
		// Gameplay Updates
		//
		//////////////////////////////////////////////////

		// Player Gravity
		player.velocity = {0, WorldPhysics::getGravity()};
		player.updatePosition();
		

		// Update moving platforms position
		if (movingPlat_1.position.x > movingPlatPos_1.x + 100) movingPlat_1.velocity.x = -movingPlatSpeed;
		if (movingPlat_1.position.x < movingPlatPos_1.x - 100) movingPlat_1.velocity.x = movingPlatSpeed;
		movingPlat_1.updatePosition();

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
