/*
	This is a component file that manages input detection.
*/
#include <SDL3/SDL.h>

// Include SDL_scancode for figuring out which keys have been pressed.
#include <SDL3/SDL_scancode.h>

#include "../headers/input.h"

/*
* This function determines if a certain key is pressed.
* @param scancode the number of the key.
*/
bool isKeyPressed(int scancode) {
	// A boolean value that represents the state of the keyboard - it's actually an array.
	std::unique_lock<std::mutex> cv_lock(inputMutex);
	const bool* keystate = SDL_GetKeyboardState(nullptr);
	return keystate[scancode] != 0;
}
