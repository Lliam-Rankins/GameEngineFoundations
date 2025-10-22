#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "../headers/GameObject.h" // Assuming this path
#include "../headers/network.h"    // Assuming this path

/**
 * Initializes the game state with a variable number of objects.
 */
void initialize_test_game_state(
    std::vector<GameObject*>& list,
    std::mutex& mutex,
    int num_static_objects,
    int num_moving_objects);

/**
 * A generic update function that will move any "is_npc" object for the test.
 */
void update_test_moving_object(GameObject* obj, float dt);

/**
 * The main server loop, refactored into a callable function.
 * This is what the performance harness will call and time.
 */
void run_server_experiment(
    long num_iterations,
    const std::string& strategy,
    int num_static_objects,
    int num_moving_objects,
    int num_clients_to_wait_for
);