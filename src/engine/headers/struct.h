#ifndef STRUCT_H
#define STRUCT_H

//General purpose struct for ordered pairs for readability and simplicity
struct OrderedPair {
    float x;
    float y;
};

//Struct for simplicity
struct Velocity {
    OrderedPair direction;
    float magnitude;
};

#endif // STRUCT_H