#include "../headers/input.h"
#include "../headers/EventManager.h"  
#include <iostream>

InputManager::InputManager(EventManager& evtManager, int playerID)
    : eventManager(evtManager), 
      numKeys(0),
      currentTime_ms(0),
      playerID(playerID)
{
    keyboardState = SDL_GetKeyboardState(&numKeys);
    if (numKeys > 0) {
        previousKeyboardState.resize(numKeys, false);
    } else {
        std::cerr << "Error: Could not get keyboard state size from SDL." << std::endl;
    }
    currentTime_ms = SDL_GetTicks();
}

void InputManager::bindKey(SDL_Scancode key, const int actionId, InputType type)
{
    m_keyBindings[key] = {type, actionId};

    // If a key is complex, it needs to be tracked in the state map
    if (type == InputType::SingleComplex) {
        m_keyStates[key] = KeyState{};
    }
}

void InputManager::bindChord(const std::vector<SDL_Scancode>& keys, const int actionId)
{
    m_chordBindings.push_back({keys, actionId});

    // Add all keys in this chord to the state map for tracking
    for (auto key : keys) {
        m_keyStates[key] = KeyState{};
    }

    // Sort chords by size, descending.
    // This ensures a 4-key chord is checked before a 2-key chord
    // that might be a subset of it.
    std::sort(m_chordBindings.begin(), m_chordBindings.end(), 
        [](const auto& a, const auto& b) {
            return a.keys.size() > b.keys.size();
        }
    );
}

/**
 * @brief The main update loop
 */
void InputManager::update()
{
    // 1. Get time and reset per-frame flags
    currentTime_ms = SDL_GetTicks();
    resetKeyStates(); // Resets 'consumed' flag

    // 2. Update timers for any complex keys just pressed
    updateTimers();

    // 3. Check for and fire chords. This will 'consume' keys.
    processChords();

    // 4. Process all single keys
    // (Fires Simple keys, and non-consumed Complex keys)
    processKeys();

    // 5. Save state for next frame
    savePreviousState();
}


/**
 * @brief Helper Function to reset consumed state
 */
void InputManager::resetKeyStates()
{
    for (auto& pair : m_keyStates) {
        pair.second.consumed = false;
    }
}

/**
 * @brief Helper Function to save previous key state
 */
void InputManager::savePreviousState()
{
    if (numKeys > 0) {
        for (int i = 0; i < numKeys; ++i) {
            previousKeyboardState[i] = keyboardState[i];
        }
    }
}

/**
 * @brief Input checking functions
 */
bool InputManager::isDown(SDL_Scancode code)
{
    return (code >= 0 && code < numKeys) && keyboardState[code];
}

bool InputManager::isPressed(SDL_Scancode code)
{
    return (code >= 0 && code < numKeys) && keyboardState[code] && !previousKeyboardState[code];
}

bool InputManager::isReleased(SDL_Scancode code)
{
    return (code >= 0 && code < numKeys) && !keyboardState[code] && previousKeyboardState[code];
}

/**
 * @brief Input Time Processing Functions
 */
void InputManager::updateTimers()
{
    // Update pressTime for any complex key that was just pressed
    for (auto& pair : m_keyStates) {
        if (isPressed(pair.first)) {
            pair.second.pressTime = currentTime_ms;
        }
    }
}

/**
 * @brief Function to process chords
 */
void InputManager::processChords()
{
    // Iterate all defined chords (already sorted from longest to shortest)
    for (const auto& binding : m_chordBindings) {
        
        bool chordTriggeredThisFrame = false;
        bool allKeysPending = true;
        Uint64 oldestPress = currentTime_ms;
        Uint64 newestPress = 0;

        for (auto key : binding.keys) {
            auto it = m_keyStates.find(key);
            // This should never happen if bound correctly, but good to check
            if (it == m_keyStates.end()) { 
                allKeysPending = false;
                break;
            }
            
            KeyState& state = it->second;

            // If any key wasn't pressed (timer is 0), chord isn't complete
            if (state.pressTime == 0) {
                allKeysPending = false;
                break;
            }

            // Check if this key was the one just pressed
            if (isPressed(key)) {
                chordTriggeredThisFrame = true;
            }

            // Find the time window
            if (state.pressTime < oldestPress) oldestPress = state.pressTime;
            if (state.pressTime > newestPress) newestPress = state.pressTime;
        }

        // Conditions to fire:
        // 1. A key in the chord was *just* pressed.
        // 2. All keys in the chord have a valid pressTime.
        // 3. The time between the first and last key is within the chord registration window.
        if (chordTriggeredThisFrame && allKeysPending && (newestPress - oldestPress < CHORD_WINDOW_MS)) {
            
            // Final check: are any of these keys already consumed by a longer chord?
            bool alreadyConsumed = false;
            for (auto key : binding.keys) {
                if (m_keyStates[key].consumed) {
                    alreadyConsumed = true;
                    break;
                }
            }

            if (!alreadyConsumed) {
                auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), binding.actionId, playerID);
                eventManager.QueueEvent(inputEvent);
                // Consume and reset all keys in this particular chord
                for (auto key : binding.keys) {
                    m_keyStates[key].consumed = true;
                    m_keyStates[key].pressTime = 0;
                }
            }
        }
    }
}

/**
 * @brief This function processes through all single key bindings
 */
void InputManager::processKeys()
{
    // Iterate all single key bindings
    for (const auto& pair : m_keyBindings) {
        SDL_Scancode key = pair.first;
        const KeyBinding& binding = pair.second;

        if (binding.type == InputType::Simple) {
            // Simple Key Logic
            if (isDown(key)) {
                auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), binding.actionId, playerID);
                eventManager.QueueEvent(inputEvent);
            }
        } 
        else if (binding.type == InputType::SingleComplex) {
            // Complex Single Key Logic
            auto it = m_keyStates.find(key);
            if (it == m_keyStates.end()) continue; 
            
            KeyState& state = it->second;
            
            bool fire = false;
            // Case 1: Key was released quickly (before timeout)
            // if (isPressed(key) && state.pressTime > 0) {
            //     fire = true;
            // } 
            // Case 2: Key was held past the timeout window
            if (state.pressTime > 0 && (currentTime_ms - state.pressTime >= CHORD_WINDOW_MS)) {
                fire = true;
            }

            // Fire if not consumed by a chord and event is not empty
            // if (fire && !state.consumed && !binding.actionId.empty()) {
            //     auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), binding.actionId, 0);
            //     eventManager.QueueEvent(inputEvent);
            // }
            if (fire) {
                auto inputEvent = std::make_shared<InputEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), binding.actionId, playerID);
                eventManager.QueueEvent(inputEvent);
            }
            
            // Reset timer on release OR if key fired
            if (isReleased(key)) {
                state.pressTime = 0; 
            }
        }
    }
}