/*
	This is a header file that contains the function declarations for all functions in input.cpp.
*/

#pragma once

#include <pthread.h>


// Mutex Lock for Input
pthread_mutex_t inputLock;

bool isKeyPressed(int scancode);