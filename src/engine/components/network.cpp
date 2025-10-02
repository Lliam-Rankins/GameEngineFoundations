/**
 * Defines networking behavior.
 */

#include "../headers/network.h"
#include "../headers/protocol.h"
#include <zmq.hpp>
#include <iostream>

/**
 * Default constructor.
 */
NetworkManager::NetworkManager() : m_role(Role::NONE), m_isInitialized(false), m_hasReceivedFirstState(false) {}

/**
 * Destructor
 */
NetworkManager::~NetworkManager()
{
    cleanUp();
}

/**
 * Starts the server.
 * @param replyPort the reply port #
 * @param publishPort the publish port #
 */
bool NetworkManager::startServer(int startReplyPort, int publishPort, int handshakePort)
{

    // Create the context and store it in the member variable
    m_context = std::make_unique<zmq::context_t>(1);

    // Create the sockets and store them in the member variables
    m_publishSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::pub);
    m_handshakeSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::rep);

    // Bind em
    m_publishSocket->bind("tcp://*:" + std::to_string(publishPort));
    m_handshakeSocket->bind("tcp://*:" + std::to_string(handshakePort));

    // Loop through each index in m_clientArr and give them an id, port number,  and specific thread
    for(int i = 0; i < MAX_PLAYERS; i++) {
        int clientPortNum = startReplyPort + i;

        m_clientArr[i].id = i;
        m_clientArr[i].portNum = clientPortNum;
        m_clientArr[i].thread = std::thread(&NetworkManager::readClient, this, i, clientPortNum);
    }

    // Initialize thread to handle incoming client messages
    m_updateThread = std::thread(&NetworkManager::messageLooper, this);
    // Initialize thread to handle incoming client handshakes
    m_handshakeThread = std::thread(&NetworkManager::handleHandshakes, this);

    // Set role to server, isInitialized is true and running is true
    m_role = Role::SERVER;

    m_isInitialized = true;

    m_running = true;

    return true;
}

/**
 * Starts the client.
 * @param serverAddress the server's address
 * @param requestPort the request port #, this should be made by adding the client's id (returned by connectAndHandshake) to a base port
 * @param subscribePort the subscribe port #
 */
bool NetworkManager::startClient(const std::string &serverAddress, int requestPort, int subscribePort)
{
    // Create the context
    m_context = std::make_unique<zmq::context_t>(1);

    // Create the subscribe socket
    m_subscribeSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::sub);

    // Create the request socket
    m_requestSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::req);

    // The request address string
    std::string requestAddress = "tcp://" + serverAddress + ":" + std::to_string(requestPort);
    std::string subscribeAddress = "tcp://" + serverAddress + ":" + std::to_string(subscribePort);

    // Connect
    m_subscribeSocket->connect(subscribeAddress);
    m_requestSocket->connect(requestAddress);

    // Set the subscribe pattern to listen for any subscription updates
    m_subscribeSocket->set(zmq::sockopt::subscribe, "");

    m_role = Role::CLIENT;
    m_isInitialized = true;
    m_running = true;

    return true;
}

/**
 * Sends the players state to the server
 * @param state is the PlayerState being sent to the server
 */
void NetworkManager::sendPlayerState(const PlayerState &state)
{
    if (m_role == Role::CLIENT && m_isInitialized)
    {
        // 1. Send the player's current state to the server.
        m_requestSocket->send(zmq::buffer(&state, sizeof(PlayerState)));

        // 2. Wait for the simple "OK" confirmation to complete the REQ/REP cycle.
        // This gets the blocking call out of the main update loop.
        zmq::message_t confirmation;
        auto result = m_requestSocket->recv(confirmation);

        if (!result)
        {
            // This block will run if recv() fails.
            // You could log an error here if you want.
            std::cerr << "Warning: Failed to receive server confirmation." << std::endl;
        }
    }
}

/**
 * Sends the players state to the server
 * @param state is the PlayerState being sent to the server
 */
void NetworkManager::sendNPCState(const NPCState &state)
{
    if (m_role == Role::SERVER && m_isInitialized)
    {
        // Publish game state to all clients
        m_publishSocket->send(zmq::buffer(&state, sizeof(NPCState)));
    }
}

// This is used to update m_gameState's NPCs and Players without changing num_clients
void NetworkManager::setGameState(const GameState& newState) {
    {
        std::lock_guard<std::mutex> lock(m_gameStateMut);
        m_gameState = newState;

    }
}

GameState NetworkManager::getGameState() {
    std::lock_guard<std::mutex> lock(m_gameStateMut);
    return m_gameState;
}

void NetworkManager::updateGameStateNPC(NPCState &npc) {
    std::lock_guard<std::mutex> lock(m_gameStateMut);
    m_gameState.npcs[npc.objectId] = npc;
}

/**
 * Connects the client and the server together.
 * @return the integer value representing the client ID.
 */
int NetworkManager::connectAndHandshake(const std::string& serverAddress, int handshakePort) {
    // Create a temporary context for the handshake
    zmq::context_t tempContext(1);
    zmq::socket_t handshakeSocket(tempContext, zmq::socket_type::req);

    // Connect the handshake
    handshakeSocket.connect("tcp://" + serverAddress + ":" + std::to_string(handshakePort));

    // Create an empty PlayerState as a connect request, make sure id is defaulted
    PlayerState connectRequest{};
    connectRequest.clientId = -1;

    // Send the connect request to the handshake socket
    handshakeSocket.send(zmq::buffer(&connectRequest, sizeof(PlayerState)));

    // Attempt to get a reply from the handshake
    zmq::message_t reply;
    if (!handshakeSocket.recv(reply)) {
        return -1;
    }

    // Get the PlayerState sent in the reply and return its client id for the client to now use
    PlayerState assigned = *reply.data<PlayerState>();
    return assigned.clientId;
}

// Thread function run by server thread to handle incoming handshakes
void NetworkManager::handleHandshakes() {
    // While server is running
    while(m_running) {
        // Create a request
        zmq::message_t req;
        // If no message is received, continue looping
        if(!m_handshakeSocket->recv(req, zmq::recv_flags::dontwait)) {
            continue;
        }
        // Get the connect request from the handshaking client
        PlayerState request = *req.data<PlayerState>();
        // Create a response PlayerState
        PlayerState response;
        {
            // Lock GameState!
            std::lock_guard<std::mutex> lock(m_gameStateMut);

            // If there is room for another client, get the current num_clients for an id and then increment num_clients
            if(m_gameState.num_clients < MAX_PLAYERS) {
                response.clientId = m_gameState.num_clients++;
            } else {
                response.clientId = -1;
            }
        }
        m_handshakeSocket->send(zmq::buffer(&response, sizeof(PlayerState)));
    }
}

// Called by client to check if there are any new gamestate updates
void NetworkManager::update() {
    // Create a new GameState message and try too receive it from the server
    zmq::message_t gameStateMessage;
    auto result = m_subscribeSocket->recv(gameStateMessage, zmq::recv_flags::dontwait);
    // If a new GameState was successfully received, update client's GameState
    if(result.has_value() && result.value() > 0) {
        std::lock_guard<std::mutex> lock(m_gameStateMut);
        m_gameState = *gameStateMessage.data<GameState>();
        m_hasReceivedFirstState = true;
    }
}

/*
 * This allows the game logic to get the most recent snapshot of the world
 * as dictated by the server.
 *
 * @return An std::optional containing the GameState if one has been received,
 * or std::nullopt if the client is still waiting for the first update.
 */
std::optional<GameState> NetworkManager::getLatestGameState()
{
    // If we haven't received the first broadcast from the server yet,
    // return an empty optional to signify "no data available."
    if (!m_hasReceivedFirstState)
    {
        return std::nullopt;
    }

    // Otherwise, return the latest GameState we have stored.
    return m_gameState;
}

/**
 * Function called by a server thread to continuously send out new messages to the clients.
 */
void NetworkManager::messageLooper() {
    // While the server is running
    while(m_running) {
        // Get the current  GameState
        GameState current;
        // Lock it all up
        {
            std::lock_guard<std::mutex> lock(m_gameStateMut);
            current = m_gameState;
        }
        // Publish game state to all clients
        m_publishSocket->send(zmq::buffer(&current, sizeof(GameState)));
        // Delay maybe?
        std::this_thread::sleep_for(std::chrono::milliseconds(32));
    }
}

/**
 * Closes sockets and contexts.
 */
void NetworkManager::cleanUp()
{
    if (m_isInitialized)
    {
        m_running = false;

        // Now we can close the member sockets because they exist
        if (m_requestSocket)
            m_requestSocket->close();
        if (m_subscribeSocket)
            m_subscribeSocket->close();
        if (m_publishSocket)
            m_publishSocket->close();
        if (m_context)
            m_context->close();

        // unique_ptr will handle deletion, but resetting them is good practice
        m_requestSocket.reset();
        m_subscribeSocket.reset();
        m_publishSocket.reset();
        m_context.reset();

        m_isInitialized = false;
        m_role = Role::NONE;

        // Close all threads
        for (int i = 0; i < MAX_PLAYERS; ++i) {
            if (m_clientArr[i].thread.joinable())
            m_clientArr[i].thread.detach(); 
        }

        if (m_updateThread.joinable()) {
            m_updateThread.join();
        }
    }
}

/**
 * Called by server thread, one per client, to continuously read input from the designated client.
 */
void NetworkManager::readClient(int id, int portNum) {
    // Create a client reply socket
    zmq::socket_t clientRep(*m_context, zmq::socket_type::rep);
    clientRep.bind("tcp://*:" + std::to_string(portNum));

    // Continuously loop while the server is running
    while(m_running) {
        // Create and try to receive a client message, if none then restart loop
        zmq::message_t message;
        if(!clientRep.recv(message, zmq::recv_flags::dontwait)) {
            continue;
        }

        // Decode the message
        PlayerState clientState = *message.data<PlayerState>();

        // Update the GameState accordingly
        {
            std::lock_guard<std::mutex> lock(m_gameStateMut);
            m_gameState.players[id] = clientState;

            // If this is a new client, increment client count
            if (m_gameState.num_clients <= id) {
                m_gameState.num_clients = id + 1;
            }
        }
        clientRep.send(zmq::buffer(""));
    }
}