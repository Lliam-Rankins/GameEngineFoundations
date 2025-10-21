/*
	This is a component file that outlines the physics system.
*/
#include "../headers/physics.h"
#include <pthread.h>

std::atomic<int> WorldPhysics::gravityWeight = 1000;
