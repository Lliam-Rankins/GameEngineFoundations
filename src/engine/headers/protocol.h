// protocol.h
#pragma once

#include <vector>
#include "../headers/struct.h"
#include "../headers/Event.h"
#include "../headers/EventManager.h"


const int MAX_NPCS = 16;


union Data {
    int i;
    float f;
    char str[16];
    bool b;
};

struct NetworkEvent {
    int type;
    int action;
    int id1;
    int id2;
    int x;
    int y;
    float timestamp;
};

// Data for a single player. Sent from Client -> Server.
struct PlayerState {
    int clientId = -1; // A unique ID for this player.
    float x = 0.0f;
    float y = 0.0f;
    int num_events = 0;
    NetworkEvent events[32];
};

// State of a non-player (server-controlled) character
struct NPCState {
    // Any generic object ID that can be used to represent the NPC
    int objectId;
    float x;
    float y;
};


// Complete data for the whole game world. Sent from Server -> all Clients.
struct GameState {
    int num_clients = 0;
    int num_events = 0;
    std::vector<PlayerState> players;
    std::vector<NetworkEvent> eventList;
    NPCState npcs[MAX_NPCS];
};



