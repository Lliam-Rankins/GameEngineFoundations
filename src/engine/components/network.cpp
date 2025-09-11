/**
 * Defines networking behavior.
 */

#include "../headers/network.h"
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
 * Updates client or server state
 */
void update()
{
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