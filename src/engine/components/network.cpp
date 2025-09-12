/**
 * Defines networking behavior.
 */

#include "../headers/network.h"
#include <zmq.hpp>

// Storage
GameState gameState[MAX_PLAYERS];
PlayerState playerState;


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
    replySocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::rep);
    publishSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::pub);

    std::string replyAddress = "tcp://*:" + std::to_string(replyPort);
    std::string publishAddress = "tcp://*:" + std::to_string(publishPort);

    // Bind em
    replySocket->bind(replyAddress);
    publishSocket->bind(publishAddress);

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
    subscribeSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::sub);

    // Create the request socket
    requestSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::req);

    // The request address string
    std::string requestAddress = serverAddress + ":" + std::to_string(requestPort);
    std::string subscribeAddress = serverAddress + ":" + std::to_string(subscribePort);

    // Connect
    subscribeSocket->connect(subscribeAddress);
    requestSocket->connect(requestAddress);

    // Set the subscribe pattern to listen for any subscription updates
    subscribeSocket->set(zmq::sockopt::subscribe, "");

    m_role = Role::CLIENT;
    m_isInitialized = true;

    return true;
}

/**
 * Updates client and server state
 */
void NetworkManager::update()
{
    // Server
    if (m_role == Role::SERVER) {
        // Wait for request
        zmq::message_t request;
        replySocket.get()->recv(request);

        // Confirm Recive with empty msg
        zmq::message_t reply;
        replySocket.get()->send(reply);


        // Code to decompile Player State
        PlayerState* playerState = reinterpret_cast<PlayerState*>(request.data());


        // Update Game State
        gameState->players[playerState->clientId] = *playerState;

        // Publish the updated playerState State
        zmq::message_t publish(&playerState, sizeof(GameState));
        publishSocket.get()->send(publish);
    }



    // Client
    if (m_role == Role::CLIENT) {
        // Create and Send Player State
        zmq::message_t request(&playerState, sizeof(PlayerState));
        requestSocket.get()->send(request);

        // Receive Confirmation from Server
        zmq::message_t reply;
        requestSocket.get()->recv(reply);



        // Receive Subcribe
        zmq::message_t subscribe;
        subscribeSocket.get()->recv(subscribe);

        // Parse Subscribe, update client gamestate?
        PlayerState* sentPlayerState = reinterpret_cast<PlayerState*>(subscribe.data());
        gameState->players[sentPlayerState->clientId] = *sentPlayerState;
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
        if (requestSocket)
            requestSocket->close();
        if (subscribeSocket)
            subscribeSocket->close();
        if (replySocket)
            replySocket->close();
        if (publishSocket)
            publishSocket->close();

        if (m_context)
            m_context->close();

        // unique_ptr will handle deletion, but resetting them is good practice
        requestSocket.reset();
        subscribeSocket.reset();
        replySocket.reset();
        publishSocket.reset();
        m_context.reset();

        m_isInitialized = false;
        m_role = Role::NONE;
    }
}