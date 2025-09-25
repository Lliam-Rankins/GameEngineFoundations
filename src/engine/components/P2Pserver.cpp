// Some of the content in this file was generated with Gemini 2.5 Pro.
// This citation is to abide by the syllabus requirement that "appropriate citations"
// must be given when referring to external sources.
#include <zmq.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <thread>
#include <chrono>

// A simple structure to hold peer information
struct PeerInfo {
    int id;
    std::string address;
};

// Function to serialize the list of peers into a single string
std::string serialize_peers(const std::map<int, PeerInfo>& peers) {
    std::stringstream ss;
    for (const auto& pair : peers) {
        ss << pair.second.id << ";" << pair.second.address << "|";
    }
    return ss.str();
}

int main() {
    const int HANDSHAKE_PORT = 5555;
    const int BROADCAST_PORT = 5556;

    zmq::context_t context(1);

    // Socket for clients to send their initial connection request
    zmq::socket_t handshake_socket(context, zmq::socket_type::rep);
    handshake_socket.bind("tcp://*:" + std::to_string(HANDSHAKE_PORT));

    // Socket to broadcast the updated list of peers to everyone
    zmq::socket_t broadcast_socket(context, zmq::socket_type::pub);
    broadcast_socket.bind("tcp://*:" + std::to_string(BROADCAST_PORT));

    std::cout << "P2P Matchmaker Server started..." << std::endl;
    std::cout << "Listening for handshakes on port " << HANDSHAKE_PORT << std::endl;
    std::cout << "Broadcasting peer updates on port " << BROADCAST_PORT << std::endl;

    std::map<int, PeerInfo> connected_peers;
    int next_peer_id = 0;

    while (true) {
        // Wait for a new client to connect and send their P2P address
        zmq::message_t request;
        handshake_socket.recv(request, zmq::recv_flags::none);
        std::string p2p_address(static_cast<char*>(request.data()), request.size());
        
        std::cout << "Received handshake from a new peer at " << p2p_address << std::endl;

        // Assign a new ID and store the peer's info
        PeerInfo new_peer;
        new_peer.id = next_peer_id;
        new_peer.address = p2p_address;
        connected_peers[next_peer_id] = new_peer;
        
        // Reply to the new peer with their assigned ID
        handshake_socket.send(zmq::buffer(std::to_string(next_peer_id)));
        
        // Serialize the full list of peers
        std::string peer_list_str = serialize_peers(connected_peers);

        // Give ZMQ a moment to establish connections before broadcasting
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Broadcast the updated list of all peers to ALL connected clients
        broadcast_socket.send(zmq::buffer(peer_list_str));
        std::cout << "Broadcasted updated peer list." << std::endl;

        next_peer_id++;
    }

    return 0;
}