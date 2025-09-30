#include <iostream>
#include <thread> // Required for std::this_thread::sleep_for
#include <chrono> // Required for std::chrono::milliseconds

#include "../headers/network.h" // Include your NetworkManager

int main(int argc, char *argv[])
{
    std::cout << "Starting server..." << std::endl;

    NetworkManager serverManager;

    // Use the same ports you configured in the client
    const int REPLY_PORT = 6000;
    const int PUBLISH_PORT = 5556;
    const int HANDSHAKE_PORT = 5557;

    if (!serverManager.startServer(REPLY_PORT, PUBLISH_PORT, HANDSHAKE_PORT))
    {
        std::cerr << "Failed to start the server." << std::endl;
        return 1;
    }

    std::cout << "Server started successfully. Waiting for clients..." << std::endl;

    while (true)
    {
        // Idle the main thread.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // In this simple server, the loop never ends.
    // You would press Ctrl+C to stop it.
    // The NetworkManager's destructor will handle cleanup automatically.
    return 0;
}
