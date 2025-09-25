// Some of the content in this file was generated with Gemini 2.5 Pro.
// This citation is to abide by the syllabus requirement that "appropriate citations"
// must be given when referring to external sources.

#include "../headers/P2PClientManager.h"
#include <iostream>
#include <sstream>
#include <thread>

P2PClientManager::P2PClientManager() : m_context(std::make_unique<zmq::context_t>(1)) {}

P2PClientManager::~P2PClientManager() {
    clean_up();
}

bool P2PClientManager::join_network(const std::string& matchmaker_address, int matchmaker_port, int my_p2p_port) {
    // --- Step 1: Establish our own address and P2P publishing socket ---
    // Clients will connect to this socket to hear from us.
    m_publisher = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::pub);
    // Note: We need to find our actual IP, but for localhost testing '*' is fine.
    // In a real game, you'd replace '*' with your public IP address.
    std::string my_p2p_address = "tcp://*:" + std::to_string(my_p2p_port);
    m_publisher->bind(my_p2p_address);
    
    // --- Step 2: Contact Matchmaker to announce our presence ---
    zmq::socket_t handshake_socket(*m_context, zmq::socket_type::req);
    handshake_socket.connect("tcp://" + matchmaker_address + ":" + std::to_string(matchmaker_port));
    
    // Send our P2P address so the matchmaker can share it
    handshake_socket.send(zmq::buffer("tcp://" + matchmaker_address + ":" + std::to_string(my_p2p_port)));

    // Wait for reply to get our unique ID
    zmq::message_t reply;
    if (!handshake_socket.recv(reply)) {
        return false;
    }
    m_my_id = std::stoi(reply.to_string());
    std::cout << "Successfully joined network. My ID is: " << m_my_id << std::endl;

    // --- Step 3: Subscribe to the matchmaker's broadcast for updates ---
    m_matchmaker_subscriber = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::sub);
    m_matchmaker_subscriber->connect("tcp://" + matchmaker_address + ":" + std::to_string(matchmaker_port + 1));
    m_matchmaker_subscriber->set(zmq::sockopt::subscribe, "");

    return true;
}

void P2PClientManager::broadcast_state(const PlayerState& state) {
    if (m_publisher) {
        m_publisher->send(zmq::buffer(&state, sizeof(PlayerState)));
    }
}

std::vector<PlayerState> P2PClientManager::poll_peer_updates() {
    std::vector<PlayerState> updates;

    // First, check for any new peers from the matchmaker
    zmq::message_t matchmaker_msg;
    if (m_matchmaker_subscriber->recv(matchmaker_msg, zmq::recv_flags::dontwait)) {
        update_peer_connections(matchmaker_msg.to_string());
    }

    // Now, poll all connected peers for their game state
    for (auto const& [id, socket] : m_peer_subscribers) {
        zmq::message_t peer_msg;
        if (socket->recv(peer_msg, zmq::recv_flags::dontwait)) {
            if (peer_msg.size() == sizeof(PlayerState)) {
                updates.push_back(*peer_msg.data<PlayerState>());
            }
        }
    }
    return updates;
}

void P2PClientManager::update_peer_connections(const std::string& peer_list_str) {
    std::stringstream ss(peer_list_str);
    std::string segment;

    while(std::getline(ss, segment, '|')) {
        std::stringstream segment_ss(segment);
        std::string id_str, address;
        std::getline(segment_ss, id_str, ';');
        std::getline(segment_ss, address, ';');

        if (id_str.empty()) continue;

        int peer_id = std::stoi(id_str);

        // If this peer is not us and we're not already subscribed to them...
        if (peer_id != m_my_id && m_peer_subscribers.find(peer_id) == m_peer_subscribers.end()) {
            std::cout << "Discovered new peer " << peer_id << " at " << address << ". Subscribing..." << std::endl;
            auto new_subscriber = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::sub);
            new_subscriber->connect(address);
            new_subscriber->set(zmq::sockopt::subscribe, "");
            m_peer_subscribers[peer_id] = std::move(new_subscriber);
        }
    }
}

AllUpdates P2PClientManager::poll_updates() {
    AllUpdates all_updates;

    // 1. Check for messages from the Authoritative Server (matchmaker)
    zmq::message_t server_msg;
    if (m_matchmaker_subscriber->recv(server_msg, zmq::recv_flags::dontwait)) {
        std::string server_msg_str = server_msg.to_string();
        std::stringstream ss(server_msg_str);
        std::string prefix;
        std::getline(ss, prefix, '|');

        if (prefix == "PEERS") {
            // This is a peer list update
            std::string peer_data = server_msg_str.substr(prefix.length() + 1);
            update_peer_connections(peer_data);
        } else if (prefix == "NPCS") {
            // This is an NPC state update
            const char* data_start = static_cast<const char*>(server_msg.data()) + prefix.length() + 1;
            size_t data_size = server_msg.size() - (prefix.length() + 1);
            if (data_size == sizeof(NPCState)) {
                 const NPCState* state = reinterpret_cast<const NPCState*>(data_start);
                 all_updates.npc_states.push_back(*state);
            }
        }
    }

    // 2. Poll all connected peers for their player state
    for (auto const& [id, socket] : m_peer_subscribers) {
        zmq::message_t peer_msg;
        if (socket->recv(peer_msg, zmq::recv_flags::dontwait)) {
            if (peer_msg.size() == sizeof(PlayerState)) {
                all_updates.player_states.push_back(*peer_msg.data<PlayerState>());
            }
        }
    }
    return all_updates;
}

int P2PClientManager::get_my_id() const {
    return m_my_id;
}

void P2PClientManager::clean_up() {
    // Sockets must be closed before the context is terminated.
    m_publisher.reset();
    m_matchmaker_subscriber.reset();
    m_peer_subscribers.clear();
    m_context.reset();
}