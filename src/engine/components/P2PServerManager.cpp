// Some of the content in this file was generated with Gemini 2.5 Pro.
// This citation is to abide by the syllabus requirement that "appropriate citations"
// must be given when referring to external sources.
#include "../headers/P2PServerManager.h"
#include <sstream>

// (Helper function to serialize peer data - can be moved to a utility file later)
std::string serialize_peers_with_prefix(const std::map<int, PeerInfo>& peers) {
    std::stringstream ss;
    ss << "PEERS|";
    for (const auto& pair : peers) {
        ss << pair.second.id << ";" << pair.second.p2p_address << "|";
    }
    return ss.str();
}

P2PServerManager::P2PServerManager() : m_context(std::make_unique<zmq::context_t>(1)) {}
P2PServerManager::~P2PServerManager() { clean_up(); }

void P2PServerManager::start(int handshake_port, int broadcast_port) {
    m_handshake_socket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::rep);
    m_handshake_socket->bind("tcp://*:" + std::to_string(handshake_port));

    m_broadcast_socket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::pub);
    m_broadcast_socket->bind("tcp://*:" + std::to_string(broadcast_port));
}

std::optional<int> P2PServerManager::check_for_new_peers() {
    zmq::message_t request;
    if (m_handshake_socket->recv(request, zmq::recv_flags::dontwait)) {
        std::string p2p_address(static_cast<char*>(request.data()), request.size());
        
        PeerInfo new_peer = {m_next_peer_id, p2p_address};
        m_connected_peers[m_next_peer_id] = new_peer;
        
        m_handshake_socket->send(zmq::buffer(std::to_string(m_next_peer_id)));
        
        int new_id = m_next_peer_id;
        m_next_peer_id++;
        return new_id;
    }
    return std::nullopt;
}

void P2PServerManager::broadcast_peer_list() {
    std::string peer_list_str = serialize_peers_with_prefix(m_connected_peers);
    m_broadcast_socket->send(zmq::buffer(peer_list_str));
}

void P2PServerManager::broadcast_data(const std::string& prefix, const void* data, size_t data_size) {
    std::string full_prefix = prefix + "|";
    zmq::message_t message(data_size + full_prefix.length());
    
    memcpy(message.data(), full_prefix.data(), full_prefix.length());
    memcpy(static_cast<char*>(message.data()) + full_prefix.length(), data, data_size);
    
    m_broadcast_socket->send(message, zmq::send_flags::none);
}

void P2PServerManager::clean_up() {
    m_handshake_socket.reset();
    m_broadcast_socket.reset();
    m_context.reset();
}