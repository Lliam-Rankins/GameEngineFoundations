// Some of the content in this file was generated with Gemini 2.5 Pro.
// This citation is to abide by the syllabus requirement that "appropriate citations"
// must be given when referring to external sources.
#pragma once
#include <zmq.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../headers/protocol.h"

struct AllUpdates {
    std::vector<PlayerState> player_states;
    std::vector<NPCState> npc_states;
};

class P2PClientManager {
public:
    P2PClientManager();
    ~P2PClientManager();

    // Connects to the matchmaker, establishes its own publish port, and subscribes to peers.
    bool join_network(const std::string& matchmaker_address, int matchmaker_port, int my_p2p_port);

    // Broadcasts this client's state to all other peers.
    void broadcast_state(const PlayerState& state);

    // Checks all peer subscriptions for new messages and returns them.
    std::vector<PlayerState> poll_peer_updates();

    // Cleans up ZMQ resources.
    void clean_up();

    // This function now polls both peers and the server and returns all updates.
    AllUpdates poll_updates();

    int get_my_id() const;

private:
    std::unique_ptr<zmq::context_t> m_context;
    
    // One socket to publish our state
    std::unique_ptr<zmq::socket_t> m_publisher;
    
    // A separate socket to listen for updates from the matchmaker
    std::unique_ptr<zmq::socket_t> m_matchmaker_subscriber;

    // A map of sockets to listen to each of our peers
    std::map<int, std::unique_ptr<zmq::socket_t>> m_peer_subscribers;
    
    int m_my_id = -1;

    // Helper function to process peer list updates from the matchmaker
    void update_peer_connections(const std::string& peer_list_str);
};