/*
	This is a component file that outlines and implements rendering.
*/
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include "../headers/input.h"
#include "../headers/entities.h"

const int WINDOW_WIDTH = 1920;  /*  Width of the game window to be created*/
const int WINDOW_HEIGHT = 1080; /*  Height of the game window to be created*/
const int FRAME_COUNT = 8;     /*  Number of frames in the spritesheet */
const int FRAME_WIDTH = 512;   /*  Width of the frame in the spritesheet */
const int FRAME_HEIGHT = 512;  /*  Height of the frame in the spritesheet */
const int ANIMATION_DELAY = 100;/* Number of iterations between the animation frames (determines delay) */

int main(int argc, char* argv[])
{
	// Initialize the SDL library
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return 1;
	}

	// Create window and renderer
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	// Initialize the window and renderer using SDL method
	if (!SDL_CreateWindowAndRenderer("Project", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
		SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
		SDL_Quit();
		return 1;
	}


	SDL_Surface* brickTexture = IMG_Load("media/brick.png");
	if (!brickTexture) {
		SDL_Log("Could not load image: %s", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	// Entity creation
	OrderedPair pos{ WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 };
	OrderedPair dim{ 100, 100 };
	Velocity vel{ {0,0}, 0 };

	// Create the entity here:

	// Main game loop condition variable
	bool running = true;

	// SDL_Event to capture event of window being closed
	SDL_Event event;

	// The main game loop
	while (running) {
		// Poll for events
		while (SDL_PollEvent(&event)) {

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
					// Add logic to handle scaling
				}
			}
		}
		// Rendering
		SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);

		// Set Background color to "Air Force" blue
		SDL_SetRenderDrawColor(renderer, 89, 139, 175, 255);

		// Clear screen
		SDL_RenderClear(renderer);

	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
