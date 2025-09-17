/*
    This file manages time for the game server and clients.
*/
#include "../headers/timeline.h"

Timeline::Timeline(long long tickSizeMs) {
    //Get the ms measurement for the real time of the timeline
    m_startTime = std::chrono::high_resolution_clock::now();
    m_lastUpdateTime = m_startTime;
    m_pauseStartTime = m_startTime;

    // Convert the tick size from milliseconds to nanoseconds for internal precision.
    m_tickSizeNs = tickSizeMs * 1000000;
    
    // Initialize all duration counters to zero.
    m_totalElapsedNs = 0;
    m_deltaTimeNs = 0;
    m_totalPausedNs = 0;

    // Initialize state variables
    m_timeScale = 1.0;
    m_isPaused = false;
}

// In timeline.cpp

void Timeline::update() {
    // 1. Get the current high-resolution time point ONCE.
    auto now = std::chrono::high_resolution_clock::now();

    if (m_isPaused) {
        // If game is paused no time advances.
        m_deltaTimeNs = 0;
    } else {
        // Calculate the duration from the last updated time
        auto elapsedDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_lastUpdateTime);
        
        // Apply the time scale to get this frame's delta time.
        m_deltaTimeNs = static_cast<long long>(elapsedDuration.count() * m_timeScale);
        
        // Add this frame's duration to the total elapsed game time.
        m_totalElapsedNs += m_deltaTimeNs;
    }
    
    // Update the 'last update' time to now, resetting for the next frame.
    m_lastUpdateTime = now;
}

void Timeline::pauseTime() {
    // Pause the time if it is not already
    if (!m_isPaused) {
        m_isPaused = true;
    }
}
void Timeline::unpauseTime() {
    // Unpause the time if it is currently
    if(m_isPaused) {
        m_isPaused = false;
    }
}

bool Timeline::isPaused() const {
    return m_isPaused;
}

long long Timeline::getElapsedTicks() const {
    if (m_tickSizeNs == 0) {
        return 0;
    }
    return m_totalElapsedNs / m_tickSizeNs;
}

float Timeline::getDeltaTime() const {
    return static_cast<float>(m_deltaTimeNs) / 1000000000.0f;
}

void Timeline::setTimeScale(float scale) {
    // Set timeScale as long as the param is inclusively between .5 and 2
    if(!((scale >= 0.5) && (scale <= 2.0))) {
        return;
    }
    m_timeScale = scale;
}
