#include "../headers/GameObject.h"

//Sets or updates a component on the GameObject.
void GameObject::setComponent(const std::string& key, Component value) {
    (*components)[key] = value;
}


//Checks if the GameObject has a specific component.
bool GameObject::hasComponent(const std::string& key) const {
    return components->find(key) != components->end();
}

