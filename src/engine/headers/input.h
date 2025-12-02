#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm> 
#include <chrono>

class EventManager; 

class InputManager {
public:
    // Defines if a key fires on press, or on chord combination
    enum class InputType {
        /** Simple Input Type: Fires immediately on press. Not part of any chord. */
        Simple,
        /**
         * SingleComplex: Part of a chord. Fires on release 
         * OR on timeout (only if not 'consumed' by a chord)
        */
        SingleComplex
    };

    InputManager(EventManager& evtManager);

    /**
     * @brief The main update loop. Called once per frame.
     */
    void update();

    /**
     * @brief Binds a single key to an event.
     * @param key The scancode to bind.
     * @param eventName The event to fire.
     * @param type Simple (fires on press) or SingleComplex (fires on release/timeout).
     */
    void bindKey(SDL_Scancode key, const int actionId, InputType type);

    /**
     * @brief Binds a multi-key chord to an event.
     * @param keys A vector of scancodes that make up the chord.
     * @param eventName The event to fire.
     */
    void bindChord(const std::vector<SDL_Scancode>& keys, const int actionId);


private:
    // State & Config
    struct KeyState {
        Uint64 pressTime = 0;
        bool consumed = false;
    };

    // The time window (in milliseconds) to allow for a chord to be completed
    static constexpr Uint32 CHORD_WINDOW_MS = 100;

    // Member Variables
    EventManager& eventManager;
    const bool* keyboardState;
    int numKeys;
    std::vector<bool> previousKeyboardState;
    Uint64 currentTime_ms;

    // Tracks the state (timer, consumed) for all keys that are part of chords
    std::map<SDL_Scancode, KeyState> m_keyStates;

    struct KeyBinding {
        InputType type;
        int actionId;
    };

    struct ChordBinding {
        std::vector<SDL_Scancode> keys;
        int actionId;
    };

    // Stores all single-key bindings (both Simple and SingleComplex)
    std::map<SDL_Scancode, KeyBinding> m_keyBindings;

    // Stores all chord bindings
    std::vector<ChordBinding> m_chordBindings;


    // Private Helper Functions
    void resetKeyStates();
    void savePreviousState();

    // Input Checkers
    bool isPressed(SDL_Scancode code);
    bool isReleased(SDL_Scancode code);
    bool isDown(SDL_Scancode code);

    // Logic Processors
    
    /**
     * @brief Updates pressTime for any complex key that was just pressed.
     */
    void updateTimers();

    /**
     * @brief Processes all N-key chords.
     * Fires events, consumes keys, and resets timers.
     */
    void processChords();

    /**
     * @brief Processes all single-key bindings.
     * Fires Simple keys on press.
     * Fires SingleComplex keys on release/timeout if not consumed.
     */
    void processKeys();
};