/*
	This is a component file that manages entities.
*/

#include "../headers/entities.h"
#include "../headers/GameObject.h"

std::mutex entityMutex;

void updatePosition(GameObject &obj, bool isPaused) {
	if(!(obj.hasComponent("position") && obj.hasComponent("velocity"))) {
		std::cout << "Improper object!" << std::endl;
	}
	std::unique_lock<std::mutex> cv_lock(entityMutex);
	Vector currPos = obj.getComponent<Vector>("position");
	Vector currVel = obj.getComponent<Vector>("velocity");

	Vector newPos = {currPos.x + currVel.x, currPos.y + currVel.y};

	if(!isPaused) {
		obj.setComponent("position", newPos);
	}
}