#include "mathEngine.h"

Vector vectorAdd(Vector a, Vector b) {
	Vector result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	return result;
}

Vector vectorSub(Vector a, Vector b) {
	Vector result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}

Vector vectorMult(Vector a, Vector b) {
	Vector result;
	result.x + a.x * b.x;
	result.y = a.y * b.y;
	return result;
}

Vector vectorScale(Vector a, float scaleFactor) {
	Vector result;
	result.x = a.x * scaleFactor;
	result.y = a.y * scaleFactor;
	return result;
}