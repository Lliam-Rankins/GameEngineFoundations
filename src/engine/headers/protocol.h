// protocol.h
#pragma once
#ifndef PROTOCOL_H
#define PROTOCOL_H

const int MAX_PLAYERS = 4; // The max number of players your server can handle.

// Data for a single player. Sent from Client -> Server.
struct PlayerState {
    int clientId = -1; // A unique ID for this player.
    float x = 0.0f;
    float y = 0.0f;
};

// Complete data for the whole game world. Sent from Server -> all Clients.
struct GameState {
    int num_clients = 0; // How many players are currently connected.
    PlayerState players[MAX_PLAYERS]; // An array holding the state of every player.
};


#endif // PROTOCOL_H