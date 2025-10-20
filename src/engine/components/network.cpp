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
bool NetworkManager::startServer(int startReplyPort, int publishPort, int handshakePort, int clientTimeout)
{
    // Assign client timeout value
    m_clientTimeout = clientTimeout;

    // Create the context and store it in the member variable
    m_context = std::make_unique<zmq::context_t>(1);

    // Create the sockets and store them in the member variables
    m_publishSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::pub);
    m_handshakeSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::rep);
    m_startReplyPort = startReplyPort;

    // Bind em
    m_publishSocket->bind("tcp://*:" + std::to_string(publishPort));
    m_handshakeSocket->bind("tcp://*:" + std::to_string(handshakePort));

    // Set role to server, isInitialized is true and running is true
    m_role = Role::SERVER;

    m_isInitialized = true;

    m_running = true;

    // Initialize thread to handle incoming client messages
    m_updateThread = std::thread(&NetworkManager::messageLooper, this);
    // Initialize thread to handle incoming client handshakes
    m_handshakeThread = std::thread(&NetworkManager::handleHandshakes, this);

    return true;
}

/**
 * Starts the server.
 * @param replyPort the reply port #
 * @param publishPort the publish port #
 */
bool NetworkManager::startServer(int startReplyPort, int publishPort, int handshakePort) {
    return NetworkManager::startServer(startReplyPort, publishPort, handshakePort, 50);
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

    m_role = Role::CLIENT;
    m_isInitialized = true;
    m_running = true;

    // Connect
    m_subscribeSocket->connect(subscribeAddress);
    m_requestSocket->connect(requestAddress);

    // Set the subscribe pattern to listen for any subscription updates
    m_subscribeSocket->set(zmq::sockopt::subscribe, "");

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

// This is used to update m_gameState's NPCs and Players without changing num_clients
void NetworkManager::setGameState(const GameState &newState)
{
    {
        std::lock_guard<std::mutex> lock(m_gameStateMut);
        m_gameState = newState;
    }
}

GameState NetworkManager::getGameState()
{
    std::lock_guard<std::mutex> lock(m_gameStateMut);
    return m_gameState;
}

/**
 * Connects the client and the server together.
 * @return the integer value representing the client ID.
 */
int NetworkManager::connectAndHandshake(const std::string &serverAddress, int handshakePort)
{
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
    if (!handshakeSocket.recv(reply))
    {
        return -1;
    }

    // Get the PlayerState sent in the reply and return its client id for the client to now use
    PlayerState assigned = *reply.data<PlayerState>();
    return assigned.clientId;
}

// Thread function run by server thread to handle incoming handshakes
void NetworkManager::handleHandshakes()
{
    // While server is running
    while (m_running)
    {
        // Create a request
        zmq::message_t req;
        // If no message is received, continue looping
        if (!m_handshakeSocket->recv(req, zmq::recv_flags::dontwait))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        int newClientId = m_nextClientId++;
        int newClientPort = m_startReplyPort + newClientId;
        // Lock the mutex and add a new client connection
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            ClientConnection newConnection;
            newConnection.id = newClientId;
            newConnection.thread = std::thread(&NetworkManager::readClient, this, newClientId, newClientPort);
            m_clients[newClientId] = std::move(newConnection);
        }

        // Create an initial state for the player in the game world.
        {
            std::lock_guard<std::mutex> lock(m_playerStatesMutex);
            PlayerState initialState;
            initialState.clientId = newClientId;
            // Set initial position based on spawn points here...
            m_playerStates[newClientId] = initialState;
        }

        std::cout << "New client connected. ID: " << newClientId << ", Port: " << newClientPort << std::endl;

        PlayerState response;
        response.clientId = newClientId;
        m_handshakeSocket->send(zmq::buffer(&response, sizeof(PlayerState)));
    }
}

// Called by client to check if there are any new gamestate updates
void NetworkManager::update()
{
    // Create a new GameState message and try too receive it from the server
    zmq::message_t gameStateMessage;
    auto result = m_subscribeSocket->recv(gameStateMessage, zmq::recv_flags::dontwait);
    // If a new GameState was successfully received, update client's GameState
    if (result.has_value() && result.value() > 0)
    {
        GameState newState;
        const char *buffer = gameStateMessage.data<const char>();

        size_t num_players;
        memcpy(&num_players, buffer, sizeof(int));
        buffer += sizeof(int);

        if (num_players > 0)
        {
            newState.players.resize(num_players);
            const size_t players_data_size = num_players * sizeof(PlayerState);
            memcpy(newState.players.data(), buffer, players_data_size);
            buffer += players_data_size;
        }

        const size_t npc_data_size = sizeof(NPCState) * MAX_NPCS;
        memcpy(&newState.npcs, buffer, npc_data_size);

        {
            std::lock_guard<std::mutex> lock(m_gameStateMut);
            m_gameState = std::move(newState);
            m_hasReceivedFirstState = true;
        }

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
void NetworkManager::messageLooper()
{
    // While the server is running
    while (m_running)
    {
        // Get the current  GameState
        GameState state_to_send;
        {
            std::lock_guard<std::mutex> lock(m_playerStatesMutex);
            state_to_send.players.reserve(m_playerStates.size());
            for (const auto &pair : m_playerStates)
            {
                state_to_send.players.push_back(pair.second);
            }
            state_to_send.num_clients = state_to_send.players.size();
        }
        const size_t num_players = state_to_send.players.size();
        const size_t players_data_size = num_players * sizeof(PlayerState);
        const size_t npc_data_size = sizeof(NPCState) * MAX_NPCS;
        const size_t total_size = sizeof(int) + players_data_size + npc_data_size;

        zmq::message_t message(total_size);
        char *buffer = message.data<char>();

        // Copy num_players, then player data, then NPC data into the buffer.
        memcpy(buffer, &num_players, sizeof(int));
        buffer += sizeof(int);
        if (num_players > 0)
        {
            memcpy(buffer, state_to_send.players.data(), players_data_size);
            buffer += players_data_size;
        }
        memcpy(buffer, &state_to_send.npcs, npc_data_size);

        m_publishSocket->send(message, zmq::send_flags::none);
        std::this_thread::sleep_for(std::chrono::milliseconds(32));
    }
}

/**
 * Closes sockets and contexts.
 */
void NetworkManager::cleanUp()
{
    if (!m_isInitialized || !m_running)
        return;

    m_running = false;
    if (m_handshakeThread.joinable())
        m_handshakeThread.join();
    if (m_updateThread.joinable())
        m_updateThread.join();

    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (auto &pair : m_clients)
        {
            if (pair.second.thread.joinable())
            {
                pair.second.thread.join();
            }
        }
        m_clients.clear();
    }

    m_playerStates.clear();
    // Now we can close the member sockets because they exist
    m_requestSocket.reset();
    m_subscribeSocket.reset();
    m_publishSocket.reset();
    m_handshakeSocket.reset();
    if (m_context)
    {
        m_context->close();
        m_context.reset();
    }
    m_isInitialized = false;
    m_role = Role::NONE;
}

/**
 * Called by server thread, one per client, to continuously read input from the designated client.
 */
void NetworkManager::readClient(int id, int portNum)
{
    // Create a client reply socket
    zmq::socket_t clientRep(*m_context, zmq::socket_type::rep);
    clientRep.bind("tcp://*:" + std::to_string(portNum));

    // Create time since last update
    std::chrono::high_resolution_clock::time_point lastTimeRecv;
    lastTimeRecv = std::chrono::high_resolution_clock::now();

    // Continuously loop while the server is running
    while (m_running)
    {
        // Create and try to receive a client message, if none then restart loop
        zmq::message_t message;
        if (!clientRep.recv(message, zmq::recv_flags::dontwait))
        {
            // Check duration of elapsed time
            std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
            int elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTimeRecv).count();
            if (elapsedTime >= m_clientTimeout) {
                // Timeout Detected, disconect current client
                {
                    // Lock Clients and erase this client
                    std::lock_guard<std::mutex> lock(m_clientsMutex);
                    m_clients.erase(id);
                }
                {
                    // Lock Player states before erasing
                    std::lock_guard<std::mutex> lock(m_playerStatesMutex);
                    m_playerStates.erase(id);
                }

                // Stop reading client (Break)
                break;
            };

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        // Received Message, reset timer;
        lastTimeRecv = std::chrono::high_resolution_clock::now();

        // Decode the message
        PlayerState clientState = *message.data<PlayerState>();
        {
            std::lock_guard<std::mutex> lock(m_playerStatesMutex);
            m_playerStates[id] = clientState;
        }
        clientRep.send(zmq::buffer(""));
    }
}

void NetworkManager::updateNpcState(const NPCState& npcState) {
    std::lock_guard<std::mutex> lock(m_gameStateMut);
    // Find the right NPC in the array (assuming ID is the index) and update it
    if (npcState.objectId >= 0 && npcState.objectId < MAX_NPCS) {
        m_gameState.npcs[npcState.objectId] = npcState;
    }
}