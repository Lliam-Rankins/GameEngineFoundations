/*
    This header file provides declarations of functions and structures used in timeline.cpp
*/

#ifndef TIMELINE_H
#define TIMELINE_H

#include <chrono>
// Timeline class to maintain a time-state for the game
class Timeline {
public:
    Timeline();
    // Real-world time
    long long realTime;
    // Real-world start time for the game, may not be useful but there in case
    long long realStartTime;
    // Starts at 0, grows from there, scales with timeScale
    long long gameTime;
    // Change in time between updateTime()s, scales with timeScale
    long long deltaTime;
    // 0.5-2.0, 1.0 is default, affects how time in game is measured so that we can change speeds as necessary
    float timeScale;
    // Is the timeline state paused?
    bool paused;

    // Calculate time updates
    void updateTime();
    // Pause time
    void pauseTime();
    // Unpause time
    void unpauseTime();
    // Change the scale of time for the timeline
    void setTimeScale(float scale);
    // Get the deltaTime var
    long long getDT();
};


#endif