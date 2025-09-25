/*
	This is a component file that outlines and implements rendering.
*/
#include "../headers/render.h"
#include "../headers/physics.h"
#include "../headers/collisions.h"
#include <cstdlib>

void initializeSDL() {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		exit(1);
	}
}

void createWindowAndRenderer(SDL_Window** window, SDL_Renderer** renderer) {
	if (!SDL_CreateWindowAndRenderer("Project", 1920, 1080, SDL_WINDOW_RESIZABLE, window, renderer)) {
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

int textureError(SDL_Renderer *renderer, SDL_Window *window){
	SDL_Log("Could not load image: %s", SDL_GetError());
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 1;
}

RenderComponent::RenderComponent(SDL_Texture* tex) : m_texture(tex) {}

void RenderComponent::render(SDL_Renderer* renderer, const Vector& position, const Vector& dimensions) {
    if (!m_texture) return;

    SDL_FRect destRect;
    destRect.x = position.x - dimensions.x / 2.0f;
    destRect.y = position.y - dimensions.y / 2.0f;

	destRect.w = dimensions.x;
    destRect.h = dimensions.y;

    SDL_RenderTexture(renderer, m_texture, NULL, &destRect);
}