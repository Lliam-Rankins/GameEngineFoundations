/*
	This is a component file that manages entities.
*/

#include "../headers/entities.h"
#include "../headers/GameObject.h"

std::mutex entityMutex;

void updatePosition(GameObject &obj, bool isPaused) {
	if(!(obj.hasComponent("Position") && obj.hasComponent("Velocity"))) {
		std::cout << "Improper object!" << std::endl;
	}
	std::unique_lock<std::mutex> cv_lock(entityMutex);
	Vector currPos = obj.getComponent<Vector>("Position");
	Vector currVel = obj.getComponent<Vector>("Velocity");

	Vector newPos = {currPos.x + currVel.x, currPos.y + currVel.y};

	if(!isPaused) {
		obj.setComponent("Position", newPos);
	}
}