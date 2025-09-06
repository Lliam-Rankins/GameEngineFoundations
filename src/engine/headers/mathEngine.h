// Header file for mathematic structs and functions
#ifndef MATHENGINE_H
#define MATHENGING_H

#include "../headers/struct.h"



// Function to add together two vectors, x + x and y + y
Vector vectorAdd(Vector a, Vector b);
// Function to subtract two vecterse, x - x, y - y
Vector vectorSub(Vector a, Vector b);
// Function to multiply two vectors, a * a, b * b
Vector vectorMult(Vector a, Vector b);
// Function to scale a vector, factor * x, factor * y
Vector vectorScale(Vector a, float scaleFactor);

#endif