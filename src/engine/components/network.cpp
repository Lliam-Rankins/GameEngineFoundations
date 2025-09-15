/**
 * Defines networking behavior.
 */

#include "../headers/network.h"
#include "../headers/protocol.h"
#include <zmq.hpp>

/**
 * Default constructor.
 */
NetworkManager::NetworkManager() : m_role(Role::NONE), m_isInitialized(false) {}

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
    std::string requestAddress = serverAddress + ":" + std::to_string(requestPort);
    std::string subscribeAddress = serverAddress + ":" + std::to_string(subscribePort);

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
        // Send the player's current state to the server.
        m_requestSocket->send(zmq::buffer(&state, sizeof(PlayerState)));
    }
}

/**
 * Updates client and server state
 */
void NetworkManager::update()
{
    // Server
    if (m_role == Role::SERVER)
    {
        // Wait for request
        zmq::message_t request;
        auto result = m_replySocket->recv(request, zmq::recv_flags::dontwait);

        if (result.has_value() && result.value() > 0)
        {
            PlayerState receivedState = *request.data<PlayerState>();

            // Confirm Recive with empty msg
            zmq::message_t reply;
            m_replySocket->send(zmq::buffer("OK", 2));

            bool foundPlayer = false;
            for (int i = 0; i < m_gameState.num_clients; i++)
            {
                if (m_gameState.players[i].clientId == receivedState.clientId)
                {
                    m_gameState.players[i] = receivedState;
                    foundPlayer = true;
                    break;
                }
            }

            if (!foundPlayer && m_gameState.num_clients < MAX_PLAYERS)
            {
                // Assign the new client an ID. For now, we'll trust the one they sent,
                // but in the future the server should assign this.
                m_gameState.players[m_gameState.num_clients] = receivedState;
                m_gameState.num_clients++;
            }
        }

        m_publishSocket->send(zmq::buffer(&m_gameState, sizeof(GameState)));
    }

    // Client
    if (m_role == Role::CLIENT)
    {
        // Create and Send Player State
        zmq::message_t reply;
        m_requestSocket->recv(reply, zmq::recv_flags::dontwait);

        // --- 2. Listen for the GameState broadcast ---
        zmq::message_t gameStateMsg;
        auto result = m_subscribeSocket->recv(gameStateMsg, zmq::recv_flags::dontwait);

        if (result.has_value() && result.value() > 0)
        {
            // We received a world update!
            // Update our local copy of the GameState.
            m_gameState = *gameStateMsg.data<GameState>();
        }
    }
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