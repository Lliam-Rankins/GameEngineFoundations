#include "../headers/recording.h"

void RecordingManager :: onEvent(const Event& e) {
    // Start Recording
    if (e.GetEventTypeID() == StartRecordingEvent::STATIC_EVENT_TYPE_ID) {
        startRecording();
    }

    // Stop Recording
    else if (e.GetEventTypeID() == StopRecordingEvent::STATIC_EVENT_TYPE_ID) {
        stopRecording();
    }

    // Start Playback
    else if (e.GetEventTypeID() == StartPlaybackEvent::STATIC_EVENT_TYPE_ID) {
        startPlayback();
    }

    // Record Event
    else if (recording) {
        recordEvent(e);
    }
}

/**
 * Responsible for starting the recording logic
 */
void RecordingManager :: startRecording() {
    // Make copy of object list, locking it

    // Set flag for recording events
    recording = true;
}

/**
 * Responsible for stoping the recording logic
 */
void RecordingManager :: stopRecording() {
    // Set flag for recording events
    recording = false;
}


/**
 * Responsible for starting the recording logic
 */
void RecordingManager :: recordEvent(const Event& e) {
    // Turn Events into shared Pointers and push onto recorded
    // TODO: might need logic for checking which events to push
    recordedEventQueue.push(std::make_shared<Event>(e));
}

/**
 * Responsible for stoping the recording logic
 */
void RecordingManager :: startPlayback() {
    // TODO : Playback Funk
}