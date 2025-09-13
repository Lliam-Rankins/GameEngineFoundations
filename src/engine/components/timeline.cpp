/*
    This file manages time for the game server and clients.
*/
#include "../headers/timeline.h"

Timeline::Timeline() {
    //Get the ms measurement for the real time of the timeline
    this->realStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    this->realTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    this->deltaTime = 0;
    this->gameTime = 0;
    this->timeScale = 1.0;
    bool paused = false;
}

void Timeline::updateTime() {
    // Get the current real time
    long long currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    // Get the change in time between updates, dilate it by the timeScale
    long long tempDeltaTime = (long long) (currentTime - this->realTime) * this->timeScale;
    // Update the realTime
    this->realTime = currentTime;
    // Added the dilated deltaTime to the gameTime
    this->gameTime += tempDeltaTime;
    // If game is not paused, update deltaTime, otherwise deltaTime = 0 so anything that needs to update based on it will not
    if(!paused) {
        this->deltaTime = tempDeltaTime;
    } else {
        this->deltaTime = 0;
    }
}


void Timeline::pauseTime() {
    // Pause the time if it is not already
    if(!this->paused) {
        this->paused = true;
    }
}
void Timeline::unpauseTime() {
    // Unpause the time if it is currently
    if(this->paused) {
        this->paused = false;
    }
}
void Timeline::setTimeScale(float scale) {
    // Set timeScale as long as the param is inclusively between .5 and 2
    if(!((scale >= 0.5) && (scale <= 2.0))) {
        return;
    }
    this->timeScale = scale;
}

long long Timeline::getDT() {
    // Return the deltaTime field
    return this->deltaTime;
}