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
	int x1;
	int y1;

	// Bottom Right Pos
	int x2;
	int y2;

	// Center Pos
	int xc;
	int yc;

	// Entity Type
	enum ent_type type;

} Entity;


Entity* entity(int x1, int y1, int x2, int y2, enum ent_type type);

#endif //ENTITIES_H