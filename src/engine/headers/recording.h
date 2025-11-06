#pragma once
#include <functional>
#include <map>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include "../headers/Event.h"
#include "../headers/GameObject.h"
#include "../headers/EventManager.h"

class RecordingManager : public EventManager {
    public:
        // Constructor to get access to the master object list
        RecordingManager(std::vector<GameObject *> *m_masterObjectList, std::mutex *m_objectListMutex, std::vector<std::vector<std::shared_ptr<Event>>> &eventList,
                        std::mutex &eventMutex);
        
        // On Event, calls other functions
        void onEvent(const Event &);
		
        // Recording Controls
        void startRecording();
		void stopRecording();
        void startPlayback();

        // Record Event
        void recordEvent(const Event& e);


    private:
        std::vector<GameObject *> *m_masterObjectList;
        
        // Local Copys for recording
        std::vector<GameObject *> *savedGameState;
        std::priority_queue<std::shared_ptr<Event>, CompareEvent>
            recordedEventQueue;

        bool recording = false;
};