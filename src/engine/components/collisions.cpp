/*
	This is a header file that contains the function declarations for all functions in collisions.cpp.
*/
struct Collider {
	// Top Left of Box
	float x1;
	float y1;

	// Bottom Right of Box
	float x2;
	float y2;

	// Default Constructor
	Collider() {
		x1 = 0;
		y1 = 0;
		x2 = 0;
		y2 = 0;
	}
	Collider(float initx1, float inity1, float initx2, float inity2) {
		x1 = initx1;
		y1 = inity1;
		x2 = initx2;
		y2 = inity2;
	}
};
