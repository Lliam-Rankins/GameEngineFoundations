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

class Entity {
    public:
    //Constructor (you may want a constructor which includes some arguments, like x,y position, texture file, etc.
    Entity();
    //Destructor
    ~Entity();

    //Ordered pairs denoting location and dimensions. Leave null if the entity doesn't need a location or dimension.
    //Alternatively, you may want to eliminate pointers so the structs are stored within the entity itself instead of in separate memory.
    OrderedPair* position;
    OrderedPair* dimensions;

    //Velocity containing direction and magnitude
    Velocity* velocity;
    //denotes if physics is applied to the entity
    bool physicsApplied;

    //Use this or a similar function if you want to have an update function in each entity.
    void update();

    //Include SDL Textures inside the Entity class
};

// Entity Constructor
Entity* entity(float x1, float y1, float x2, float y2, float ent_type type);

#endif //ENTITIES_H