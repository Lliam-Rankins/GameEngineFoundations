/*
	This is a header file that contains the function declarations for all functions in render.cpp.
*/
#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>
#include "struct.h" // For the Vector struct

const int WINDOW_WIDTH = 1920;  /*  Width of the game window to be created*/
const int WINDOW_HEIGHT = 1080; /*  Height of the game window to be created*/

// A component responsible for rendering an entity
class RenderComponent
{
public:
	RenderComponent(SDL_Texture *tex);
	void render(SDL_Renderer *renderer, const Vector &position, const Vector &dimensions);

		// --- FIX: Add Move Semantics ---
	// Prevent copying to avoid issues with texture ownership.
	RenderComponent(const RenderComponent &) = delete;
	RenderComponent &operator=(const RenderComponent &) = delete;

	// Explicitly tell the compiler that moving is safe and how to do it.
	RenderComponent(RenderComponent &&) noexcept = default;
	RenderComponent &operator=(RenderComponent &&) noexcept = default;

private:
	SDL_Texture *m_texture;
};

void initializeSDL();

void createWindowAndRenderer(SDL_Window **window, SDL_Renderer **renderer);

void setupScreen(SDL_Renderer *renderer);

void refreshScreen(SDL_Renderer *renderer);

#endif