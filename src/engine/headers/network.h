/**
 * This header file includes declarations for network management.
 */
#pragma once

#include <string>
#include <memory>
#include <optional>
#include "protocol.h"
#include <thread>
#include <mutex>
#include <map>
#include <atomic>

namespace zmq
{
    class context_t;
    class socket_t;
}

class NetworkManager
{
public:
    NetworkManager();
    ~NetworkManager();

    /**
     * Starts the server at the listed port.
     * @param replyPort the port for replying
     * @param publishPort the port for publishing
     * @param handshakePort is the port to handle handshakes at from new clients
     */
    bool startServer(int replyPort, int publishPort, int handshakePort);

    /**
     * Starts the server at the listed port.
     * @param replyPort the port for replying
     * @param publishPort the port for publishing
     * @param handshakePort is the port to handle handshakes at from new clients
     * @param clientTimeout is ticks needed to consider a client 'disconected'
     */
    bool startServer(int startReplyPort, int publishPort, int handshakePort, int clientTimeout);

    /**
     * Starts the client and connects to the server address at the port given.
     * -- Run this after connectAndHandshake to generate a unique requestPort using the returned id
     * @param serverAddress the address of the server
     * @param requestPort the port for requests
     * @param subscribePort the port for subscribes
     * @return whether or not the client was successfully started
     */
    bool startClient(const std::string &serverAddress, int requestPort, int subscribePort);

    /**
     * Closes connections, contexts, and threads.
     */
    void cleanUp();

    /**
     * Sends player states.
     */
    void sendPlayerState(const PlayerState &state);

    /**
     * Function that runs constantly to update client and server state.
     */
    void update();

    /**
        Function that is used to update m_gameState
    */
    void setGameState(const GameState &newState);

    GameState getGameState();

    /**
     * Connects to the server and performs a handshake to get a client ID.
     * @return The unique client ID assigned by the server, or -1 on failure.
     */
    int connectAndHandshake(const std::string &serverAddress, int handshakePort);
    // void handleHandshakes();

    /**
     * Gets the latest game state information
     */
    std::optional<GameState> getLatestGameState();

    void updateNpcState(const NPCState &npcState);

private:
    // Enum to track whether we are a server, a client, or uninitialized.
    enum class Role
    {
        NONE,
        SERVER,
        CLIENT
    };

    // Struct to help handle and keep track of each client
    struct ClientConnection

    {
        int id;
        std::thread thread;
    };

    std::map<int, ClientConnection> m_clients;
    std::mutex m_clientsMutex;
    //std::mutex m_gameStateMut;
    std::map<int, PlayerState> m_playerStates;
    std::mutex m_playerStatesMutex;
    
    Role m_role;

    // The server's threads
    std::thread m_updateThread;
    std::thread m_handshakeThread;

    // A pointer to the current context
    std::unique_ptr<zmq::context_t> m_context;

    // The server's sockets
    std::unique_ptr<zmq::socket_t> m_publishSocket;
    std::unique_ptr<zmq::socket_t> m_handshakeSocket;

    // The client's sockets
    std::unique_ptr<zmq::socket_t> m_requestSocket;
    std::unique_ptr<zmq::socket_t> m_subscribeSocket;

    // Boolean value to ensure that whatever needed to happen goes well before proceeding (starting server, etc)
    bool m_isInitialized;

    // Integer value for tick count without receiving client information to consider them timedout
    int m_clientTimeout;

    std::atomic<bool> m_running{false};

    // A flag for if we have received the first game state
    bool m_hasReceivedFirstState;

    // A complete game state object
    GameState m_gameState;
    std::mutex m_gameStateMut;
    int m_startReplyPort;
    std::atomic<int> m_nextClientId{0};

    /**
        Thread function ran by one server thread to continuously check for and handle handshakes from new clients.
    */
    void handleHandshakes();

    /**
        Thread function ran by one server thread to continuously send out gameState updates to all clients.
    */
    void messageLooper();

    /**
        Thread function, one for each client, in which the server makes a thread to loop through this function in order to check for new client messages/updates.
        @param id is the client's id
        @param portNum is the port number to be connected at
     */
    void readClient(int id, int portNum);

};