/*
	This is a header file that contains the function declarations for all functions in render.cpp.
*/
#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include "../headers/entities.h"
#include "../headers/input.h"


const int WINDOW_WIDTH = 1920;  /*  Width of the game window to be created*/
const int WINDOW_HEIGHT = 1080; /*  Height of the game window to be created*/
const int FRAME_COUNT = 8;     /*  Number of frames in the spritesheet */
const int FRAME_WIDTH = 512;   /*  Width of the frame in the spritesheet */
const int FRAME_HEIGHT = 512;  /*  Height of the frame in the spritesheet */
const int ANIMATION_DELAY = 100;/* Number of iterations between the animation frames (determines delay) */

void initializeSDL();

void createWindowAndRenderer(SDL_Window **window, SDL_Renderer **renderer);

void setupScreen(SDL_Renderer *renderer);

void refreshScreen(SDL_Renderer *renderer);

#endif