/*
	This is a component file that outlines and implements rendering.
*/
#include "../headers/render.h"

void initializeSDL() {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		exit(1);
	}
}

void createWindowAndRenderer(SDL_Window** window, SDL_Renderer** renderer) {
	if (!SDL_CreateWindowAndRenderer("Project", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, window, renderer)) {
		SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
		SDL_Quit();
		exit(1);
	}
}

void setupScreen(SDL_Renderer *renderer) {
	SDL_SetRenderDrawColor(renderer, 89, 139, 175, 255);
	SDL_RenderClear(renderer);
	
}

void refreshScreen(SDL_Renderer *renderer) {
	SDL_RenderPresent(renderer);
}


// int main(int argc, char* argv[])
// {
// 	// Initialize the SDL library
// 	initializeSDL();

// 	// Initialize the window and renderer using SDL method
// 	createWindowAndRenderer();
	

// 	SDL_Texture* brickTexture = IMG_LoadTexture(renderer, "media/brick.png");
// 	if (!brickTexture) {
// 		SDL_Log("Culd not load image: %s", SDL_GetError());
// 		SDL_DestroyRenderer(renderer);
// 		SDL_DestroyWindow(window);
// 		SDL_Quit();
// 		return 1;
// 	}

// 	// Entity creation
// 	Vector pos{ WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 };
// 	Vector dim{ 100, 100 };
// 	Vector vel{0,0};

// 	// Main game loop condition variable
// 	bool running = true;

// 	// SDL_Event to capture event of window being closed
// 	SDL_Event event;

// 	// Scaling Type bool
// 	bool constantSizeScale = true;

// 	// The main game loop
// 	while (running) {
// 		// Poll for events
// 		while (SDL_PollEvent(&event)) {

// 			// If event is Window Resize
// 			if (event.type == SDL_EVENT_WINDOW_RESIZED) {
// 				// Constant Scaling
// 				if (constantSizeScale) {
// 					// Resize as if the screen was still the same
// 					SDL_SetRenderLogicalPresentation(renderer, 1920, 1080, SDL_LOGICAL_PRESENTATION_STRETCH);
// 				}
// 				// Proportional Scaling
// 				else {
// 					//Get Window Size
// 					int w, h;
// 					SDL_GetWindowSize(window, &w, &h);
// 					// Resize as if the screen was still the same
// 					SDL_SetRenderLogicalPresentation(renderer, w, h, SDL_LOGICAL_PRESENTATION_STRETCH);
// 				}	
// 			}

// 			// Read input from input manager
// 			// If the event is close the window
// 			if (event.type == SDL_EVENT_QUIT)
// 				running = false;

// 			// Otherwise look for a key press
// 			else if (event.type == SDL_EVENT_KEY_DOWN) {
// 				// Testing to allow the keypress of "ESC" to exit the window.
// 				if (isKeyPressed(SDL_SCANCODE_ESCAPE)) {
// 					running = false;
// 				}
// 				if (isKeyPressed(SDL_SCANCODE_A)) {
// 				}
// 				if (isKeyPressed(SDL_SCANCODE_D)) {
// 				}
// 				if (isKeyPressed(SDL_SCANCODE_E)) {
// 					// Spawn a box that falls onto the static platform based on keyboard input
// 					// You'll need to detect and handle collisions as 
// 				}
// 				if (isKeyPressed(SDL_SCANCODE_GRAVE)) {
// 					constantSizeScale = !constantSizeScale;
// 				}
// 			}
// 		}
// 		// Rendering
// 		setupScreen();
// 		//SDL_RenderPresent(renderer);

// 		// Set Background color to "Air Force" blue
// 		//SDL_SetRenderDrawColor(renderer, 89, 139, 175, 255);
// 		SDL_FRect destRect = { 0, 0, 350, 100 };
//     	SDL_RenderTexture(renderer, brickTexture, NULL, &destRect);
// 		// Clear screen
// 		//SDL_RenderClear(renderer);
		
// 		refreshScreen();

// 	}

// 	SDL_DestroyRenderer(renderer);
// 	SDL_DestroyWindow(window);
// 	SDL_Quit();

// 	return 0;
// }
