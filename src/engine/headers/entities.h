/*
	This is a header file that contains the function declarations for all functions in entities.cpp.
*/

#ifndef ENTITIES_H
#define ENTITIES_H

// Entity Type Enum
enum ent_Type {
	Static,
	Moving,
	Controlled
};

struct {
	// Top Left Pos
	float x1;
	float y1;

	// Bottom Right Pos
	float x2;
	float y2;

	// Center Pos
	float xc;
	float yc;

	// Entity Type
	enum ent_type type;

} Entity;

// Entity Constructor
Entity* entity(float x1, float y1, float x2, float y2, float ent_type type);

#endif //ENTITIES_H