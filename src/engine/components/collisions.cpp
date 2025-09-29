/*
	This is a component file that manages collisions.
*/
#include "../headers/collisions.h"


Vector Collider::getTopLeft() {
	std::unique_lock<std::mutex> cv_lock(colliderMutex);
	return this->topLeft;
}

Vector Collider::getBottomRight() {
	std::unique_lock<std::mutex> cv_lock(colliderMutex);
	return this->bottomRight;
}

void Collider::setTopLeft(Vector newTopLeft) {
	std::unique_lock<std::mutex> cv_lock(colliderMutex);
	this->topLeft = newTopLeft;
}

void Collider::setBottomRight(Vector newBottomRight) {
	std::unique_lock<std::mutex> cv_lock(colliderMutex);
	this->bottomRight = newBottomRight;
}

/**
  This function takes in two colliders and provides a bool value depending on if they are overlapping or not.
  @param a, b are two Collider instances
  @return true if a and b collide, false otherwise
*/
bool overlappingColliders(const Collider &a, const Collider &b) {

	// Check if x values for a and b overlap at all
	bool overlappingX = false;

	if (a.topLeft.x < b.bottomRight.x && a.bottomRight.x > b.topLeft.x) {
		overlappingX = true;
	}
	// Check if y values for a and b overlap at all
	bool overlappingY = false;

	if (a.topLeft.y < b.bottomRight.y && a.bottomRight.y > b.topLeft.y) {
		overlappingY = true;
	}

	// If x and y values overlap, return true, otherwise return false
	if (overlappingX && overlappingY) {
		return true;
	}
	return false;
}