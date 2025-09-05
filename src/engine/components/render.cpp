/*
	This is a component file that outlines and implements rendering.
*/
#include "../headers/render.h"
#include "../headers/physics.h"
#include "../headers/collisions.h"

void initializeSDL() {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		exit(1);
	}
}

void createWindowAndRenderer() {
	if (!SDL_CreateWindowAndRenderer("Project", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
		SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
		SDL_Quit();
		exit(1);
	}
}

void setupScreen() {
	SDL_SetRenderDrawColor(renderer, 89, 139, 175, 255);
	SDL_RenderClear(renderer);
	
}

void refreshScreen() {
	SDL_RenderPresent(renderer);
}

void renderEntity(SDL_Renderer* renderer, const Entity& e) {
    SDL_FRect rect = { e.position.x - e.dimensions.x / 2.0f, e.position.y - e.dimensions.y / 2.0f, e.dimensions.x, e.dimensions.y };
    SDL_RenderTexture(renderer, e.texture, NULL, &rect);
}

int textureError(){
	SDL_Log("Could not load image: %s", SDL_GetError());
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 1;
}

int main(int argc, char* argv[])
{
	// Initialize the SDL library
	initializeSDL();

	// Initialize the window and renderer using SDL method
	createWindowAndRenderer();
	
	// Don't show the player initially
	bool showPlayer= false;

	SDL_Texture* platformTexture = IMG_LoadTexture(renderer, "media/warped city files/warped city files/Assets/ENVIRONMENT/props/control-box-3.png");
	if (!platformTexture) {
		textureError();
	}
	SDL_Texture* playerTexture = IMG_LoadTexture(renderer, "media/warped city files/warped city files/Assets/SPRITES/player/idle/idle-1.png");

	Vector platPos = { WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 };
	Vector platDim = { 62, 30 };
	Vector vel = { 0, 0 };

	Entity platform(platPos, platDim, platformTexture, false, vel);
	// Create a collider for the platform
	Collider platformCollider(
		platPos.x - platDim.x / 2.0f,
		platPos.y - platDim.y / 2.0f,
		platPos.x + platDim.x / 2.0f,
		platPos.y + platDim.y / 2.0f
	);
	platform.setCollider(&platformCollider);

	Vector playerPos = { WINDOW_WIDTH / 2, platPos.y - 100};
	Vector playerDim = { 71, 67 };
	vel = { 0, 0 };

	Entity player(playerPos, playerDim, playerTexture, true, vel);

	Collider playerCollider(
		playerPos.x - playerDim.x / 2.0f,
		playerPos.y - playerDim.y / 2.0f,
		playerPos.x + playerDim.x / 2.0f,
		playerPos.y + playerDim.y / 2.0f
	);

	player.setCollider(&playerCollider);


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
				if (isKeyPressed(SDL_SCANCODE_ESCAPE)) {
					running = false;
				}
				if (isKeyPressed(SDL_SCANCODE_A)) {
					player.velocity = { -1, 0 };
					player.updatePosition();
				}
				if (isKeyPressed(SDL_SCANCODE_D)) {
					player.velocity = { 1, 0 };
					player.updatePosition();
				}
				if (isKeyPressed(SDL_SCANCODE_E)) {
					// Render the player
					showPlayer = !showPlayer;
					player.position = { WINDOW_WIDTH / 2.0f, platPos.y - 300.0f }; // Reset position.
                    player.velocity = { 0, 0 }; // Reset velocity.
				}
				if (isKeyPressed(SDL_SCANCODE_GRAVE)) {
					constantSizeScale = !constantSizeScale;
				}
				if (isKeyPressed(SDL_SCANCODE_SPACE)) {
					player.velocity = {0, -30};
					player.updatePosition();
				}
			}
		}
		// Rendering
		setupScreen();

		// Render the platform
		renderEntity(renderer, platform);

		if (player.physicsApplied) {
			player.velocity.y += Physics::getGravity();
		}

		player.position.y += player.velocity.y;

		// Update collider to match new position
		player.collider->topLeft.y = player.position.y - player.dimensions.y / 2.0f;
		player.collider->bottomRight.y = player.position.y + player.dimensions.y / 2.0f;

		// Collision check
		if (overlappingColliders(*player.collider, *platform.collider)) {
			if (player.velocity.y > 0) {
				player.position.y = platform.position.y - platform.dimensions.y / 2.0f - player.dimensions.y / 2.0f;
				player.collider->topLeft.y = player.position.y - player.dimensions.y / 2.0f;
				player.collider->bottomRight.y = player.position.y + player.dimensions.y / 2.0f;
				player.velocity.y = 0;
			}
		}

		if (showPlayer) {
        	renderEntity(renderer, player);
    	}

		refreshScreen();

	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
