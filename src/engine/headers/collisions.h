/*
	This is a component file that manages collisions.
*/
#include "collision.h"

public bool overlappingColliders(Collider a, Collider b) {

	bool overlappingX;

	if ((a.x1 >= b.x1 && a.x1 <= b.x2) || (a.x2 >= b.x1 && a.x2 <= b.x2)) {
		overlappingX = true;
	}

	bool overlappingY;

	if ((a.y1 >= b.y1 && a.y1 <= b.y2) || (a.y2 >= b.y1 && a.y2 <= b.y2)) {
		overlappingY = true;
	}

	if (overlappingX && overlapinngY) {
		return true;
	}
	return false;
}