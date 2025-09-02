// Header file for mathematic structs and functions
#ifndef MATHENGINE_H
#define MATHENGING_H
#endif

// Vector struct with x and y values
struct Vector {
	float x, y;

	Vector() {
		x = 0;
		y = 0;
	}
	Vector(float initX, float initY) {
		x = initX;
		y = initY;
	}
};

// Function to add together two vectors, x + x and y + y
Vector vectorAdd(Vector a, Vector b);
// Function to subtract two vecterse, x - x, y - y
Vector vectorSub(Vector a, Vector b);
// Function to multiply two vectors, a * a, b * b
Vector vectorMult(Vector a, Vector b);
// Function to scale a vector, factor * x, factor * y
Vector vectorScale(Vector a, float scaleFactor);