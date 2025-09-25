// Some of the content in this file was generated with Gemini 2.5 Pro.
// This citation is to abide by the syllabus requirement that "appropriate citations"
// must be given when referring to external sources.
#pragma once
#include <zmq.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

// A simple structure to hold peer information
struct PeerInfo {
    int id;
    std::string p2p_address;
};

class P2PServerManager {
public:
    P2PServerManager();
    ~P2PServerManager();

    // Starts the server and binds the necessary ports
    void start(int handshake_port, int broadcast_port);

    // Checks for new clients without blocking. Returns the new peer's ID if one connected.
    std::optional<int> check_for_new_peers();

    // Broadcasts the current list of all connected peers
    void broadcast_peer_list();
    
    // Generic function to broadcast any type of data with a message prefix
    void broadcast_data(const std::string& prefix, const void* data, size_t data_size);

    void clean_up();

private:
    std::unique_ptr<zmq::context_t> m_context;
    std::unique_ptr<zmq::socket_t> m_handshake_socket;
    std::unique_ptr<zmq::socket_t> m_broadcast_socket;

    std::map<int, PeerInfo> m_connected_peers;
    int m_next_peer_id = 0;
};