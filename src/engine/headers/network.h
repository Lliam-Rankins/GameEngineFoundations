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
     */
    bool startServer(int replyPort, int publishPort, int handshakePort);

    /**
     * Starts the client and connects to the server address at the port given.
     * @param serverAddress the address of the server
     * @param requestPort the port for requests
     * @param subscribePort the port for subscribes
     * @return whether or not the client was successfully started
     */
    bool startClient(const std::string &serverAddress, int requestPort, int subscribePort);

    /**
     * Closes connections and contexts.
     */
    void cleanUp();

    /**
     * Sends player states.
     */
    void sendPlayerState(const PlayerState& state);

    /**
     * Function that runs constantly to update client and server state.
     */
    void update();

    /**
        Function that is used to update m_gameState
    */
    void setGameState(const GameState& newState);

    GameState getGameState();

    /**
     * Connects to the server and performs a handshake to get a client ID.
     * @return The unique client ID assigned by the server, or -1 on failure.
     */
    int connectAndHandshake(const std::string& serverAddress, int handshakePort);

    //void handleHandshakes();

    /**
     * Gets the latest game state information
     */
    std::optional<GameState> getLatestGameState();

    
private:
    // Enum to track whether we are a server, a client, or uninitialized.
    enum class Role
    {
        NONE,
        SERVER,
        CLIENT
    };

    // Struc to help handle and keep track of each client
    struct Client {
        int id;
        int portNum;
        std::thread thread;
        std::unique_ptr<zmq::socket_t> replySocket;
    };

    Client m_clientArr[MAX_PLAYERS];
    std::mutex m_gameStateMut;

    Role m_role;

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

    bool m_running;

    // A flag for if we have received the first game state
    bool m_hasReceivedFirstState;

    // A complete game state object
    GameState m_gameState;

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