
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"


int main(int argc, char* argv[])
{
	// Initialize the SDL library
    
	initializeSDL();
	
	// Initialize the window and renderer using SDL method
	SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
	createWindowAndRenderer(&window, &renderer);
	

	SDL_Texture* brickTexture = IMG_LoadTexture(renderer, "media/brick.png");
	if (!brickTexture) {
		SDL_Log("Culd not load image: %s", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	// Entity creation

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
				}
				if (isKeyPressed(SDL_SCANCODE_D)) {
				}
				if (isKeyPressed(SDL_SCANCODE_E)) {
					// Spawn a box that falls onto the static platform based on keyboard input
					// You'll need to detect and handle collisions as 
				}
				if (isKeyPressed(SDL_SCANCODE_GRAVE)) {
					constantSizeScale = !constantSizeScale;
				}
			}
		}

		// Rendering
		setupScreen(renderer);
		//SDL_RenderPresent(renderer);

		SDL_FRect destRect = { 0, 0, 350, 100 };
    	SDL_RenderTexture(renderer, brickTexture, NULL, &destRect);

		// Clear screen
		//SDL_RenderClear(renderer);
		
		refreshScreen(renderer);

	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
