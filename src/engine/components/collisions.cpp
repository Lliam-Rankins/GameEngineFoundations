/*
	This is a component file that manages collisions.
*/
#include "collisions.h"

/**
  This function takes in two colliders and provides a bool value depending on if they are overlapping or not.
  @param a, b are two Collider instances
  @return true if a and b collide, false otherwise
*/
bool overlappingColliders(Collider a, Collider b) {

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