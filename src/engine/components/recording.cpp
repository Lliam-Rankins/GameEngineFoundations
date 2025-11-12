#include "../headers/recording.h"

/**
 * Constructor for recording manager
 */
RecordingManager::RecordingManager(SDL_Renderer *renderer, EventManager *eventManager, int player_id, Vector defaultOffset, std::vector<GameObject *> *masterList, 
                                   std::mutex *mutex, 
                                   void (*renderFunction)(GameObject *, Vector))
    : eventManager(eventManager),
      renderer(renderer),
      player_id(player_id),
      defaultOffset(defaultOffset),
      m_masterObjectList(masterList),
      m_objectListMutex(mutex),
      render_func(renderFunction) {}

void record(bool *recording, std::queue<std::vector<GameObject>> *savedGameStates, std::vector<GameObject *> *m_masterObjectList, std::mutex *m_objectListMutex) {
    int x = 0;
    // While we are recording, push states onto queue
    while (*recording) {
        std::vector<GameObject> frameCopy;
        {
            std::lock_guard<std::mutex> lock(*m_objectListMutex);
            
            std::cout << x << std::endl;

            for (auto* obj : *m_masterObjectList) {
                frameCopy.push_back(obj->clone());
            }

            savedGameStates->push(std::move(frameCopy));

            x+=1;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    std::cout << "End Start Record Thread" << std::endl;
}


/**
 * Responsible for starting the recording logic
 */
void RecordingManager :: startRecording() {
    // Set flag for recording events
    *recording = true;

    // Start Recording Thread
	std::thread recordingThread(record, recording, &savedGameStates, m_masterObjectList, m_objectListMutex);
    recordingThread.detach();
}

/**
 * Responsible for stoping the recording logic
 */
void RecordingManager :: stopRecording() {
    // Set flag for recording events
    *recording = false;
}

void playback(SDL_Renderer *renderer, EventManager *eventManager, int player_id, Vector defaultOffset, std::queue<std::vector<GameObject>> *savedGameStates, void (*renderObj)(GameObject *, Vector)) {    
    // While we have 
    while (!savedGameStates->empty()) {
        // // Setup the Screen
		setupScreen(renderer);

        std::vector<GameObject> gameState = savedGameStates->front();
        savedGameStates->pop();

        Vector cameraOffset = {0, 0};

        // Find player object
        for (auto& object : gameState) {
            if (object.hasComponent("client_id") &&
                object.getComponent<int>("client_id") == player_id) {

                if (object.hasComponent("position")) {
                    cameraOffset = object.getComponent<Vector>("position");
                    break;
                }
            }
        }

        for (auto& object : gameState) {
            renderObj(&object, cameraOffset);
        }

        // Have thread sleep, preserves time
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Raise Stop Playback
    auto stopPlaybacEventk = std::make_shared<StopPlaybackEvent>(std::chrono::steady_clock::now().time_since_epoch().count());
    eventManager->QueueEvent(stopPlaybacEventk);
}

/**
 * Responsible for starting the playback logic
 */
void RecordingManager :: startPlayback() {
    // Start Playback Thread
	std::thread playbackThread(playback, renderer, eventManager, player_id, defaultOffset, &savedGameStates, render_func);
    playbackThread.detach();
}