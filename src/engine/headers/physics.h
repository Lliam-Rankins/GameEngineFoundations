/*
	This is a header file that contains the function declarations for all functions in physics.cpp.
*/
#ifndef PHYSICS_H
#define PHYSICS_H

pthread_mutex_t physicsLock;

class WorldPhysics {
    public:

    //Use these functions to set and get the gravity variable to apply to entities
    static void setGravity(const int gravity) {
        pthread_mutex_lock(&physicsLock);
        gravityWeight = gravity;
        pthread_mutex_unlock(&physicsLock);
    };
    static int getGravity() {
        pthread_mutex_lock(&physicsLock);
        return gravityWeight;
        pthread_mutex_unlock(&physicsLock);
    };

    private:
    static int gravityWeight;
};

#endif
