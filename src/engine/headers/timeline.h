/*
    This header file provides declarations of functions and structures used in timeline.cpp
*/

#pragma once

#include <chrono>
#include <atomic>
// Timeline class to maintain a time-state for the game
class Timeline
{
public:
    // Default constructor for timeline with tickSizeMs as 16.
    Timeline(long long tickSizeMs = 16);

    void update();

    long long getElapsedTicks() const;
    float getDeltaTime() const;

    void pauseTime();
    void unpauseTime();
    bool isPaused() const;
    void setTimeScale(float scale);
    float getTimeScale();

private:
    

    // Use the high_resolution_clock's native time_point for maximum precision
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_lastUpdateTime;

    // Store durations in nanoseconds using int64_t (long long)
    std::atomic<long long> m_currentTick;
    std::atomic<long long> m_tickCt;
    std::atomic<long long> m_tickSizeNs; // The duration of a single "tick" in nanoseconds
    std::atomic<long long> m_totalElapsedNs;
    std::atomic<long long> m_deltaTimeNs;

    // Variables for pausing
    std::chrono::high_resolution_clock::time_point m_pauseStartTime;
    std::atomic<long long> m_totalPausedNs;

    std::atomic<double> m_timeScale;
    std::atomic<bool> m_isPaused;
};
