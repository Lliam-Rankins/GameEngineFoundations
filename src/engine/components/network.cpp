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
bool NetworkManager::startServer(int replyPort, int publishPort)
{
    // Create the context and store it in the member variable
    m_context = std::make_unique<zmq::context_t>(1);

    // Create the sockets and store them in the member variables
    m_replySocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::rep);
    m_publishSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::pub);

    std::string replyAddress = "tcp://*:" + std::to_string(replyPort);
    std::string publishAddress = "tcp://*:" + std::to_string(publishPort);

    // Bind em
    m_replySocket->bind(replyAddress);
    m_publishSocket->bind(publishAddress);

    m_role = Role::SERVER;

    m_isInitialized = true;

    return true;
}

/**
 * Starts the client.
 * @param serverAddress the server's address
 * @param requestPort the request port #
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

    return true;
}

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
 * Connects the client and the server together.
 * @return the integer value representing the client ID.
 */
int NetworkManager::connectAndHandshake()
{

    // If the role is not client or hasn't been started
    if (m_role != Role::CLIENT || !m_isInitialized)
    {
        return -1; // Throw an error and fail to connect, we need a client to connect
    }

    // Send the initial connection request
    PlayerState connectRequest;
    connectRequest.clientId = -1; // -1 signifies a new connection
    m_requestSocket->send(zmq::buffer(&connectRequest, sizeof(PlayerState)));

    // Wait for the server's reply
    zmq::message_t reply;
    auto result = m_requestSocket->recv(reply, zmq::recv_flags::none);

    if (!result.has_value() || result.value() == 0)
    {
        return -1; // Failed to get a reply from the server
    }

    // Create a new playerstate object based on the reply and return the ID.
    PlayerState assignedState = *reply.data<PlayerState>();
    return assignedState.clientId;
}

/**
 * Updates client and server state
 */
void NetworkManager::update()
{

    // If we're the server
    if (m_role == Role::SERVER)
    {
        zmq::message_t request;
        auto result = m_replySocket->recv(request, zmq::recv_flags::dontwait);

        // Check if we actually received a message from any client
        if (result.has_value() && result.value() > 0)
        {
            PlayerState receivedState = *request.data<PlayerState>();

            // If the ID is -1
            if (receivedState.clientId == -1)
            {
                // This is a new client connecting
                if (m_gameState.num_clients < MAX_PLAYERS)
                {
                    // Assign a new, unique ID
                    int newId = m_gameState.num_clients;
                    receivedState.clientId = newId;

                    // Add them to our game state.
                    m_gameState.players[newId] = receivedState;
                    m_gameState.num_clients++;

                    std::cout << "New player connected. Assigned ID: " << newId << std::endl;

                    // 3. Send their state back with the update ID.
                    // This completes the handshake.
                    m_replySocket->send(zmq::buffer(&receivedState, sizeof(PlayerState)));
                }
                else
                {
                    // Server is full, send a "full" message or an error state.
                    // For now, we'll just send an empty reply.
                    m_replySocket->send(zmq::buffer("FULL", 4));
                }
            }
            // Otherwise it's a normal client request
            else
            {
                // This is an existing client sending a regular update.
                // Find and update the player in our master game state.
                for (int i = 0; i < m_gameState.num_clients; ++i)
                {
                    if (m_gameState.players[i].clientId == receivedState.clientId)
                    {
                        m_gameState.players[i] = receivedState;
                        break;
                    }
                }

                // Send a simple "OK" reply to complete the request reply cycle
                m_replySocket->send(zmq::buffer("OK", 2));
            }
        }

        // After handling any incoming requests publish the latest world state
        m_publishSocket->send(zmq::buffer(&m_gameState, sizeof(GameState)));
    }

    // If we are a client
    if (m_role == Role::CLIENT)
    {
        zmq::message_t gameStateMsg;
        auto result = m_subscribeSocket->recv(gameStateMsg, zmq::recv_flags::dontwait);

        if (result.has_value() && result.value() > 0)
        {
            m_gameState = *gameStateMsg.data<GameState>();

            // FIX #3: Set the flag to true once you get the first message.
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
 * Closes sockets and contexts.
 */
void NetworkManager::cleanUp()
{
    if (m_isInitialized)
    {
        // Now we can close the member sockets because they exist
        if (m_requestSocket)
            m_requestSocket->close();
        if (m_subscribeSocket)
            m_subscribeSocket->close();
        if (m_replySocket)
            m_replySocket->close();
        if (m_publishSocket)
            m_publishSocket->close();
        if (m_context)
            m_context->close();

        // unique_ptr will handle deletion, but resetting them is good practice
        m_requestSocket.reset();
        m_subscribeSocket.reset();
        m_replySocket.reset();
        m_publishSocket.reset();
        m_context.reset();

        m_isInitialized = false;
        m_role = Role::NONE;
    }
}