/*
	This is a header file that contains the function declarations for all functions in collisions.cpp.
*/
#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <mutex>
#include "mathEngine.h"
#include "../headers/GameObject.h"


struct Collider {
	Vector topLeft;
	Vector bottomRight;
};

bool overlappingColliders1(GameObject &objA, GameObject &objB);
bool overlappingColliders2(GameObject &objA, GameObject &objB);

#endif // COLLISIONS_H