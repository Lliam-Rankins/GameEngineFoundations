/*
	This is a header file that contains the function declarations for all functions in collisions.cpp.
*/
#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <mutex>
#include "mathEngine.h"

struct Collider {
	// Top Left of Box
	Vector topLeft;

	// Bottom Right of Box
	Vector bottomRight;

	// Mutex Lock for CoSllider
    std::mutex colliderMutex;

	// Default Constructor
	Collider() {
		topLeft.x = 0;
		topLeft.y = 0;
		bottomRight.x = 0;
		bottomRight.y = 0;
	}
	Collider(float initx1, float inity1, float initx2, float inity2) {
		topLeft.x = initx1;
		topLeft.y = inity1;
		bottomRight.x = initx2;
		bottomRight.y = inity2;
	}

	Vector getTopLeft();
	Vector getBottomRight();

	void setTopLeft(Vector newTopLeft);
	void setBottomRight(Vector newBottomRight);
};
// This function returns true if two colliders are overlapping, false otherwise
bool overlappingColliders(const Collider& a, const Collider& b);

#endif // COLLISIONS_H