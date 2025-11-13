#pragma once

#include <SDL3/SDL.h>
#include <map>
#include <string>
#include <variant>
#include "../headers/struct.h"

/**
 * @file GameObject.hpp
 * @brief Declaration of the GameObject class and its component type.
 *
 * This header file defines the public interface for the GameObject class,
 * which uses a pure component model. It's intended to be included by any part
 * of the application that needs to interact with GameObjects.
 */

// A type alias for a std::variant that can hold different data types for a component.
using Component = std::variant<int, float, bool, SDL_Color, SDL_Texture*, std::string, Vector>;

/**
 * @class GameObject
 * @brief Represents an entity in the game world as a collection of components.
 */
class GameObject {
public:
    // The map that stores the object's components.
    std::map<std::string, Component> components;

    /**
     * @brief Sets or updates a component on the GameObject.
     * @param key The name of the component.
     * @param value The value of the component.
     */
    void setComponent(const std::string& key, Component value);

    /**
     * @brief Retrieves a component's value, casting it to the specified type.
     * @tparam T The expected data type of the component.
     * @param key The name of the component to retrieve.
     * @return The value of the component cast to type T.
     */
    template <typename T>
    T getComponent(const std::string& key) {
        // Template implementations must be in the header file.
        return std::get<T>(components.at(key));
    }

    /**
     * @brief Checks if the GameObject has a specific component.
     * @param key The name of the component to check for.
     * @return True if the component exists, false otherwise.
     */
    bool hasComponent(const std::string& key) const;

    /**
     * AI USE DISCLOSURE
     * Asked ChatGPT to generate a clone function for the game object class
     * after providing the game objects header
     */
    GameObject clone() const {
        GameObject copy;
        for (const auto& [key, value] : components) {
            copy.components[key] = value;
        }
        return copy;
    }
};
