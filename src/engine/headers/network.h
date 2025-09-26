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
//#include <condition_variable>

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
    bool startServer(int replyPort, int publishPort);

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
    // void update();

    /**
     * Connects to the server and performs a handshake to get a client ID.
     * @return The unique client ID assigned by the server, or -1 on failure.
     */
    int connectAndHandshake();

    /**
     * Gets the latest game state information
     */
    std::optional<GameState> getLatestGameState();

    void messageLooper();

    /**
     */
    void readClient(int id, int portNum);

    /**
     */
    void updateAllClients();

private:
    // Enum to track whether we are a server, a client, or uninitialized.
    enum class Role
    {
        NONE,
        SERVER,
        CLIENT
    };

    struct Client {
        int id;
        int portNum;
        std::thread thread;
        std::unique_ptr<zmq::socket_t> replySocket;
    }

    clients[3] m_clientArr;
    std::mutex m_gameStateMut;
    //std::condition_variable m_gameStateCV;

    Role m_role;

    std::thread m_updateThread;

    // A pointer to the current context
    std::unique_ptr<zmq::context_t> m_context;

    // The server's sockets
    std::unique_ptr<zmq::socket_t> m_replySocket;
    std::unique_ptr<zmq::socket_t> m_publishSocket;

    // The client's sockets
    std::unique_ptr<zmq::socket_t> m_requestSocket;
    std::unique_ptr<zmq::socket_t> m_subscribeSocket;

    // Boolean value to ensure that whatever needed to happen goes well before proceeding (starting server, etc)
    bool m_isInitialized;

    // A complete game state object
    GameState m_gameState;

    // A flag for if we have received the first game state
    bool m_hasReceivedFirstState;

};