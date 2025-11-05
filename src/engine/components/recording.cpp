#include "../headers/recording.h"

/**
 * Constructor for recording manager
 */
RecordingManager::RecordingManager(std::vector<GameObject *> *masterList, 
                                   std::mutex *mutex, 
                                   void (*renderFunction)(GameObject *, Vector))
    : m_masterObjectList(masterList),
      m_objectListMutex(mutex),
      render_func(renderFunction) {}

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
}


/**
 * Responsible for starting the recording logic
 */
void RecordingManager :: startRecording() {
    // Set flag for recording events
    *recording = &isRecording;

    // Start Recording Thread
	std::thread recordingThread(&recording, &savedGameStates, &m_masterObjectList, &m_objectListMutex);
}

void record(bool *recording, std::queue<std::vector<GameObject *>> *savedGameStates, std::vector<GameObject *> *m_masterObjectList, std::mutex *m_objectListMutex) {
    
    // While we are recording, push states onto queue
    while (*recording) {
        // Copy game state
        {
            std::lock_guard<std::mutex> lock(*m_objectListMutex);
            // Push game state
            (*savedGameStates).push(*m_masterObjectList);
        }

        // Have thread sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

/**
 * Responsible for stoping the recording logic
 */
void RecordingManager :: stopRecording() {
    // Set flag for recording events
    *recording = &notRecording;
}

/**
 * Responsible for starting the playback logic
 */
void RecordingManager :: startPlayback() {
    // Start Playback Thread
	std::thread playbackThread(&savedGameStates, &render_func);
}


void playback(std::queue<std::vector<GameObject *>> *savedGameStates, void (*renderObj)(GameObject *, Vector)) {    
    // While we have 
    while (!(*savedGameStates).empty()) {
        std::vector<GameObject *> gameState = (*savedGameStates).front();
        (*savedGameStates).pop();
        for (GameObject* object : gameState) {
            renderObj(object, Vector{0, 0});
        }

        // Have thread sleep, preserves time
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}