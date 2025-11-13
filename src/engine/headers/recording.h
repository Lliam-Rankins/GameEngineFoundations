#pragma once
#include <functional>
#include <map>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include "../headers/Event.h"
#include "../headers/GameObject.h"
#include "../headers/EventManager.h"

class RecordingManager : public EventManager {
    public:
        // Constructor to get access to the master object list
        RecordingManager(int player_id, Vector defaultOffset, std::vector<GameObject *> *masterList, 
                         std::mutex *mutex, 
                         void (*renderObjectFunction)(GameObject *, Vector));

        // Event Handling
        void onEvent(const Event &);
		
        // Recording Controls
        void startRecording();
		void stopRecording();
        void startPlayback();

        // Record Event
        void recordEvent(const Event& e);


    private:
        // Pointers to outside game state
        std::vector<GameObject *> *m_masterObjectList;
        std::mutex *m_objectListMutex;
        
        // Local Copys for recording
        std::queue<std::vector<GameObject *>> savedGameStates;

        bool *recording;
        bool isRecording = true;
        bool notRecording = false;

        int player_id;
        Vector defaultOffset;

        // Render Obj function, takes a game object ptr and an offset vector
        void (*render_func)(GameObject *, Vector);
};