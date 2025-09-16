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

void renderEntity(SDL_Renderer* renderer, const Entity& e) {
    SDL_FRect rect = { e.position.x - e.dimensions.x / 2.0f, e.position.y - e.dimensions.y / 2.0f, e.dimensions.x, e.dimensions.y };
    SDL_RenderTexture(renderer, e.texture, NULL, &rect);
}

int textureError(SDL_Renderer *renderer, SDL_Window *window){
	SDL_Log("Could not load image: %s", SDL_GetError());
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 1;
}
