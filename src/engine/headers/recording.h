#pragma once
#include <functional>
#include <map>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <iostream>
#include "../headers/Event.h"
#include "../headers/GameObject.h"
#include "../headers/EventManager.h"
#include "../headers/render.h"

class RecordingManager : public EventManager {
    public:
        // Constructor to get access to the master object list
        RecordingManager(SDL_Renderer *renderer, EventManager *eventManager, int player_id, Vector defaultOffset, std::vector<GameObject *> *masterList, 
                         std::mutex *mutex, 
                         void (*renderObjectFunction) (GameObject *, Vector));
		
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
        std::queue<std::vector<GameObject>> savedGameStates;

        bool isRecording = false;
        bool *recording = &isRecording;

        int player_id;
        Vector defaultOffset;

        SDL_Renderer *renderer;
        EventManager *eventManager;

        // Render Obj function, takes a game object ptr and an offset vector
        void (*render_func)(GameObject *, Vector);
};