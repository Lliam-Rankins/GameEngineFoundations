/**
 * This header file includes declarations for network management.
 */

#pragma once
#include <string>
#include <memory>
#include <optional>

namespace zmq {
    class context_t;
    class socket_t;
}

class NetworkManager {
    public:
        NetworkManager();
        ~NetworkManager();

        /**
         * Starts the server at the listed port.
         * @param port the port number
         */
        bool startServer(int port);

        /**
         * Starts the client and connects to the server address at the port given.
         * @param serverAddress the address of the server
         * @param port the port number
         */
        bool startClient(const std::string& serverAddress, int port);

        /**
         * Closes connections and contexts.
         */
        void cleanUp();

        /**
         * Function that runs constantly to update client and server state.
         */
        void update();

    private:
        // Enum to track whether we are a server, a client, or uninitialized.
        enum class Role { NONE, SERVER, CLIENT };
        Role m_role;

        // A pointer to the current context
        std::unique_ptr<zmq::context_t> m_context;

        // A pointer to the current socket
        std::unique_ptr<zmq::socket_t> m_socket;

        // Boolean value to ensure that whatever needed to happen goes well before proceeding (starting server, etc)
        bool m_isInitialized;
    };