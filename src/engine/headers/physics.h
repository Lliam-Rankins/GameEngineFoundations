/*
	This is a header file that contains the function declarations for all functions in physics.cpp.
*/
#ifndef PHYSICS_H
#define PHYSICS_H

class WorldPhysics {
    public:

    //Use these functions to set and get the gravity variable to apply to entities
    static void setGravity(const int gravity) {gravityWeight = gravity;};
    static int getGravity() {return gravityWeight;};

    private:
    static int gravityWeight;
};

// class PhysicsBody {
    
// }
#endif
