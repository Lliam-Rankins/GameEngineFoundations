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
// void updateCollider(GameObject &obj) {
// 	if(!(obj.hasComponent("Position") && obj.hasComponent("Dimensions"))) {
// 		std::cout << "Improper object!" << std::endl;
// 	}
// 	Vector pos = obj.getComponent<Vector>("Position");
// 	Vector dim = obj.getComponent<Vector>("Dimensions");
// 	Collider newCol;
// 	newCol.topLeft.x = pos.x;
// 	newCol.topLeft.y = pos.y;
// 	newCol.bottomLeft.x = pos.x + dim.x;
// 	newCol.bottomRight.y = pos.y + dim.y;
// 	obj.setComponent("Collider", newCol);
// }


/**
  This function takes in two colliders and provides a bool value depending on if they are overlapping or not.
  @param a, b are two Collider instances
  @return true if a and b collide, false otherwise
*/
bool overlappingColliders1(GameObject& objA, GameObject& objB) {

	Collider a;
	a.topLeft = objA.getComponent<Vector>("Position");
	a.bottomRight.x = a.topLeft.x + objA.getComponent<Vector>("Dimensions").x;
	a.bottomRight.y = a.topLeft.y + objA.getComponent<Vector>("Dimensions").y;

	Collider b;
	b.topLeft = objB.getComponent<Vector>("Position");
	b.bottomRight.x = b.topLeft.x + objB.getComponent<Vector>("Dimensions").x;
	b.bottomRight.y = b.topLeft.y + objB.getComponent<Vector>("Dimensions").y;

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
bool overlappingColliders2(GameObject& objA, GameObject& objB) {

	Collider a;
	a.topLeft = objA.getComponent<Vector>("Position");
	a.bottomRight.x = a.topLeft.x + objA.getComponent<Vector>("Dimensions").x;
	a.bottomRight.y = a.topLeft.y + objA.getComponent<Vector>("Dimensions").y;

	Collider b;
	b.topLeft = objB.getComponent<Vector>("Position");
	b.bottomRight.x = b.topLeft.x + objB.getComponent<Vector>("Dimensions").x;
	b.bottomRight.y = b.topLeft.y + objB.getComponent<Vector>("Dimensions").y;

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