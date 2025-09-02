/*
	This is a header file that contains the function declarations for all functions in entities.cpp.
*/

#ifndef ENTITIES_H
#define ENTITIES_H

#include <SDL3/SDL_render.h>

class Entity {
    public:
    //Constructor (you may want a constructor which includes some arguments, like x,y position, texture file, etc.
    Entity();

    //Constructor (you may want a constructor which includes some arguments, like x,y position, texture file, etc.
    Entity(OrderedPair*, OrderedPair*, SDL_Texture*);

    //Destructor
    ~Entity();

    //Ordered pairs denoting location and dimensions. Leave null if the entity doesn't need a location or dimension.
    //Alternatively, you may want to eliminate pointers so the structs are stored within the entity itself instead of in separate memory.
    //Entity's bottom right point
    OrderedPair* position;
    //Distance up and left to top left point
    OrderedPair* dimensions;

    //Velocity containing direction and magnitude
    Velocity* velocity;
    //denotes if physics is applied to the entity
    bool physicsApplied;

    // Entity Texture
    SDL_Texture* texture;

    //Use this or a similar function if you want to have an update function in each entity.
    void update();

};

// Entity Constructor
Entity* entity(OrderedPair );

#endif //ENTITIES_H