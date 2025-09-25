/*
	This is a header file that contains the function declarations for all functions in input.cpp.
*/

#pragma once


#include <mutex>

// Mutex Lock for Input
std::mutex inputMutex;

bool isKeyPressed(int scancode);