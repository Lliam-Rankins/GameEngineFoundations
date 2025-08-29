/*
	This is a component file that manages collisions.
*/
#include "collision.h"

/**
  This function takes in two colliders and provides a bool value depending on if they are overlapping or not.
  @param a, b are two Collider instances
  @return true if a and b collide, false otherwise
*/
bool overlappingColliders(Collider a, Collider b) {

	// Check if x values for a and b overlap at all
	bool overlappingX = false;

	if (a.x1 < b.x2 && a.x2 > b.x1) {
		overlappingX = true;
	}
	// Check if y values for a and b overlap at all
	bool overlappingY = false;

	if (a.y1 < b.y2 && a.y2 > a.b1) {
		overlappingY = true;
	}

	// If x and y values overlap, return true, otherwise return false
	if (overlappingX && overlappingY) {
		return true;
	}
	return false;
}