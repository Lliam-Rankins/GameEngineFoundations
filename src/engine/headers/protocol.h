// protocol.h
#pragma once

#include <vector>
#include "../headers/struct.h"

const int MAX_NPCS = 16;


union Data {
    int i;
    float f;
    char str[16];
    bool b;
    Vector v;
};

// Data for a single player. Sent from Client -> Server.
struct PlayerState {
    int clientId = -1; // A unique ID for this player.
    float x = 0.0f;
    float y = 0.0f;
    Data dataArr[8];
};

// State of a non-player (server-controlled) character
struct NPCState {
    // Any generic object ID that can be used to represent the NPC
    int objectId;
    float x;
    float y;
    Data dataArr[4];
};

// Complete data for the whole game world. Sent from Server -> all Clients.
struct GameState {
    int num_clients = 0;
    std::vector<PlayerState> players; // 
    NPCState npcs[MAX_NPCS];
};


