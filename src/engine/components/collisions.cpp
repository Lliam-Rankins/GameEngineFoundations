/*
	This is a component file that manages collisions.
*/
#include "../headers/collisions.h"


// Vector Collider::getTopLeft() {
// 	std::unique_lock<std::mutex> cv_lock(colliderMutex);
// 	return this->topLeft;
// }

// Vector Collider::getBottomRight() {
// 	std::unique_lock<std::mutex> cv_lock(colliderMutex);
// 	return this->bottomRight;
// }

// void Collider::setTopLeft(Vector newTopLeft) {
// 	std::unique_lock<std::mutex> cv_lock(colliderMutex);
// 	this->topLeft = newTopLeft;
// }

// void Collider::setBottomRight(Vector newBottomRight) {
// 	std::unique_lock<std::mutex> cv_lock(colliderMutex);
// 	this->bottomRight = newBottomRight;
// }

// FIX THIS DOC LATER, this just calculates a collider and returns it, use this when updating pos
void updateCollider(GameObject &obj) {
	if(!(obj.hasComponent("Position") && obj.hasComponent("Dimensions"))) {
		// Error
	}
	Vector pos = obj.getComponent<Vector>("Position");
	Vector dim = obj.getComponent<Vector>("Dimensions");
	Collider newCol;
	newCol.topLeft.x = pos.x;
	newCol.topLeft.y = pos.y;
	newCol.bottomLeft.x = pos.x + dim.x;
	newCol.bottomRight.y = pos.y + dim.y;
	return newCol;
}


/**
  This function takes in two colliders and provides a bool value depending on if they are overlapping or not.
  @param a, b are two Collider instances
  @return true if a and b collide, false otherwise
*/
bool overlappingColliders1(GameObject &objA, GameObject &objB) {

	if(!(objA.hasComponent("Position") && objA.hasComponent("Dimensions") && objB.hasComponent("Position") && objB.hasComponent("Dimensions"))) {
		// Error
	}

	Vector posA = objA.getComponent<Vector>("Position");
	Vector dimA = objA.getComponent<Vector>("Dimensions");

	Vector posB = objB.getComponent<Vector>("Position");
	Vector dimB = objB.getComponent<Vector>("Dimensions");

	Collider a;
	a.topLeft.x = posA.x;
	a.topLeft.y = posA.y;
	a.bottomLeft.x = posA.x + dimA.x;
	a.bottomRight.y = posA.y + dimA.y;

	Collider b;
	b.topLeft.x = posB.x;
	b.topLeft.y = posB.y;
	b.bottomLeft.x = posB.x + dimB.x;
	b.bottomRight.y = posB.y + dimB.y;

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

/**
  This function takes in two colliders and provides a bool value depending on if they are overlapping or not.
  @param a, b are two Collider instances
  @return true if a and b collide, false otherwise
*/
bool overlappingColliders2(Collider a, Collider b) {

	if(!(objA.hasComponent("Position") && objA.hasComponent("Dimensions") && objB.hasComponent("Position") && objB.hasComponent("Dimensions"))) {
		// Error
	}

	Vector posA = objA.getComponent<Vector>("Position");
	Vector dimA = objA.getComponent<Vector>("Dimensions");

	Vector posB = objB.getComponent<Vector>("Position");
	Vector dimB = objB.getComponent<Vector>("Dimensions");

	Collider a;
	a.topLeft.x = posA.x;
	a.topLeft.y = posA.y;
	a.bottomLeft.x = posA.x + dimA.x;
	a.bottomRight.y = posA.y + dimA.y;

	Collider b;
	b.topLeft.x = posB.x;
	b.topLeft.y = posB.y;
	b.bottomLeft.x = posB.x + dimB.x;
	b.bottomRight.y = posB.y + dimB.y;

	// Check if x values for a and b overlap at all
	bool overlappingX = false;

	if (a.topLeft.x < b.bottomRight.x && a.bottomRight.x > b.topLeft.x) {
		overlappingX = true;
	}
	// Check if y values for a and b overlap at all
	bool overlappingY = false;

	if (a.topLeft.y > b.bottomRight.y && a.bottomRight.y < b.topLeft.y) {
		overlappingY = true;
	}

	// If x and y values overlap, return true, otherwise return false
	if (overlappingX && overlappingY) {
		return true;
	}
	return false;
}