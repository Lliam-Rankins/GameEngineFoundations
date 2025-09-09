#ifndef STRUCT_H
#define STRUCT_H

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


#endif // STRUCT_H