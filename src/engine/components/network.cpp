/**
 * Defines networking behavior. Some of the content in this file was edited with AI tools.
    This citation is to abide by the syllabus requirement that "appropriate citations"
    must be given when referring to external sources. More information is available upon request.
 */

#include "../headers/network.h"
#include "../headers/protocol.h"
#include "../headers/gameUtils.h"
#include <zmq.hpp>
#include <iostream>
#include <set>
#include <algorithm>
#include <chrono>
#include <cstdint>

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

// Helper function to find a GameObject in the master list by its network ID.
GameObject *findLocalObject(int objectId, const std::vector<GameObject *> &objectList)
{
    for (GameObject *obj : objectList)
    {
        int id = -1;
        if (obj->hasComponent("client_id"))
        {
            id = obj->getComponent<int>("client_id");
        }
        else if (obj->hasComponent("npc_id"))
        {
            id = obj->getComponent<int>("npc_id");
        }
        if (id == objectId)
        {
            return obj;
        }
    }
    return nullptr; // Not found
}

/**
 * Starts the server.
 * @param replyPort the reply port
 * @param publishPort the publish port #
 */
bool NetworkManager::startServer(int startReplyPort, int publishPort, int handshakePort, int clientTimeout,
                                 std::vector<GameObject *> &objectList, std::mutex &objectMutex)
{

    m_masterObjectList = &objectList;
    m_objectListMutex = &objectMutex;

    // Create the context and store it in the member variable
    m_context = std::make_unique<zmq::context_t>(1);

    // Create the sockets and store them in the member variables
    m_publishSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::pub);
    m_handshakeSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::rep);
    m_startReplyPort = startReplyPort;

    // Bind em
    m_publishSocket->bind("tcp://*:" + std::to_string(publishPort));
    m_handshakeSocket->bind("tcp://*:" + std::to_string(handshakePort));

    // Increase send high-water mark so brief subscriber stalls don't block us
    try
    {
        m_publishSocket->set(zmq::sockopt::sndhwm, 1000);
    }
    catch (const zmq::error_t &e)
    {
        std::cerr << "[Network] Warning: failed to set sndhwm: " << e.what() << std::endl;
    }

    std::cout << "[Network] Server bound publish:" << publishPort << " handshake:" << handshakePort << std::endl;

    // Store the client timeout (in milliseconds). The caller supplies a value
    // which we'll interpret as seconds if it's small; to avoid accidental
    // immediate timeouts when callers pass small integers, treat the incoming
    // value as seconds if < 1000, otherwise assume it's already milliseconds.
    if (clientTimeout > 0 && clientTimeout < 1000)
        m_clientTimeout = clientTimeout * 1000; // treat as seconds
    else
        m_clientTimeout = clientTimeout; // already milliseconds or zero

    // Set role to server, isInitialized is true and running is true
    m_role = Role::SERVER;

    m_isInitialized = true;

    m_running = true;

    // Initialize thread to handle incoming client messages
    m_updateThread = std::thread(&NetworkManager::messageLooper, this);
    // Initialize thread to handle incoming client handshakes
    m_handshakeThread = std::thread(&NetworkManager::handleHandshakes, this);

    std::cout << "[Network] Server threads started (messageLooper, handleHandshakes)" << std::endl;

    return true;
}

/**
 * Starts the client.
 * @param serverAddress the server's address
 * @param requestPort the request port #, this should be made by adding the client's id (returned by connectAndHandshake) to a base port
 * @param subscribePort the subscribe port #
 */
bool NetworkManager::startClient(const std::string &serverAddress, int requestPort, int subscribePort,
                                 std::vector<GameObject *> &objectList, std::mutex &objectMutex)
{
    // Create the context
    m_context = std::make_unique<zmq::context_t>(1);

    // New object stuff
    m_masterObjectList = &objectList;
    m_objectListMutex = &objectMutex;

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

    std::cout << "[Network] Client connected subscribe:" << subscribeAddress << " request:" << requestAddress << std::endl;

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
        auto sendOk = m_requestSocket->send(zmq::buffer(&state, sizeof(PlayerState)));
        if (!sendOk)
        {
            std::cerr << "[Network] Warning: sendPlayerState send() returned false" << std::endl;
        }

        // 2. Wait for the simple "OK" confirmation to complete the REQ/REP cycle.
        zmq::message_t confirmation;
        auto result = m_requestSocket->recv(confirmation);

        if (!result)
        {
            std::cerr << "[Network] Warning: Failed to receive server confirmation." << std::endl;
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


/**
 * AI USE:
 * Using Gemini I explained the structure of the m_masterObject list, nature of the NPC/GameObjects,
 * and asked for a method to update this vector. Adjusted to work as needed.
 */
/**
 * Updates a specific NPC's state in the master game object list.
 * This function bridges the network NPCState (from server logic) back to 
 * the game engine's master list for synchronization.
 * * @param npc The incoming NPCState struct containing the new position and id.
 */
void NetworkManager::updateGameStateNPC(NPCState &npc) {
    // 1. Lock the mutex protecting the master object list
    // Assuming m_objectListMutex is now a direct member std::mutex
    std::lock_guard<std::mutex> lock(m_objectListMutex); 

    // 2. Iterate through the master list to find the matching NPC
    for (const auto& obj : m_masterObjectList) {
        
        // Check if the object is an NPC AND has a matching ID
        if (obj->hasComponent("is_npc")) {
            
            // Assuming getComponent<int>("npc_id") retrieves the NPC's unique identifier
            int object_npc_id = obj->getComponent<int>("npc_id");
            std::cout << object_npc_id << std::endl;

            if (object_npc_id == npc.objectId) {
                // 3. Update the components of the located NPC game object
                                
                // Assuming setComponent is a method to update object properties
                obj->setComponent("position", Vector{npc.x, npc.y}); 

                // 4. Optimization: Break once the object is found and updated
                return;
            }
        }
    }

    // No Entity Found, make new
    std::unique_ptr<GameObject> newNpcObject = std::make_unique<GameObject>();
    // 5. Initialize the necessary components from the NPCState struct
    
    // Add the tags/identifiers
    newNpcObject.get()->setComponent("is_npc", true);
    newNpcObject.get()->setComponent("npc_id", npc.objectId); 
    
    // Add the position component
    newNpcObject.get()->setComponent("position", Vector{npc.x, npc.y});

    
    // 5. Add the newly created (cloned) object to the master list
    m_masterObjectList.push_back(std::move(newNpcObject.release()));
    
    // std::cout << "[Server] Added new NPC with ID: " << clonedNpc.get()->getComponent<int>("npc_id") << " to master list." << std::endl;
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

// void NetworkManager::updateGameStateNPC(NPCState &npc) {
//     std::lock_guard<std::mutex> lock(m_gameStateMut);
//     m_gameState.npcs[npc.objectId] = npc;
// }

/**
 * Connects the client and the server together.
 * @return the integer value representing the client ID.
 */
int NetworkManager::connectAndHandshake(const std::string &serverAddress, int handshakePort, int &replyPort)
{
    zmq::context_t tempContext(1);
    zmq::socket_t handshakeSocket(tempContext, zmq::socket_type::req);
    handshakeSocket.connect("tcp://" + serverAddress + ":" + std::to_string(handshakePort));

    PlayerState connectRequest{};
    connectRequest.clientId = -1;
    handshakeSocket.send(zmq::buffer(&connectRequest, sizeof(PlayerState)));
    std::cout << "[Network] Sent handshake to " << serverAddress << ":" << handshakePort << std::endl;

    zmq::message_t reply;
    if (!handshakeSocket.recv(reply))
    {
        std::cerr << "[Network] Error: Handshake recv failed" << std::endl;
        return -1;
    }

    PlayerState assigned = *reply.data<PlayerState>();
    // The server now sends us the correct reply port in the 'x' field.
    replyPort = assigned.x;
    std::cout << "[Network] Handshake assigned clientId=" << assigned.clientId << " replyPort=" << replyPort << std::endl;
    return assigned.clientId;
}

// A helper function to find a spawn point. Could also be in gameUtils.h
GameObject *findAvailableSpawnPoint(std::vector<GameObject *> &objectList)
{
    for (auto &obj : objectList)
    {
        if (obj->hasComponent("is_spawnpoint"))
        {
            return obj;
        }
    }
    return nullptr;
}

void NetworkManager::update()
{
    zmq::message_t gameStateMessage;
    bool gotAny = false;
    static auto lastUpdateTime = std::chrono::steady_clock::now();
    static auto lastRecvTime = std::chrono::steady_clock::now();
    auto firstRecvTime = std::chrono::steady_clock::time_point();
    auto lastRecvThisCall = std::chrono::steady_clock::time_point();

    int drained_count = 0;

    while (true)
    {
        zmq::message_t msg;
        // non-blocking recv
        bool ok = static_cast<bool>(m_subscribeSocket->recv(msg, zmq::recv_flags::dontwait));
        if (!ok)
            break;
        // capture the most recent message
        gameStateMessage = std::move(msg);
        auto now_recv = std::chrono::steady_clock::now();
        if (!gotAny)
        {
            firstRecvTime = now_recv;
            gotAny = true;
        }
        lastRecvThisCall = now_recv;
        drained_count++;

    }

    if (!gotAny)
    {
        // No messages available 
        return;
    }

    // Parse the latest incoming message (server timestamp + payload)
    GameState newState;
    const char *buffer = gameStateMessage.data<const char>();

    // Read server timestamp (ms since epoch) for one-way latency measurement
    uint64_t server_ts_ms = 0;
    memcpy(&server_ts_ms, buffer, sizeof(uint64_t));
    buffer += sizeof(uint64_t);

    // Read sequence number (uint32_t) to detect missed packets
    uint32_t seq = 0;
    memcpy(&seq, buffer, sizeof(uint32_t));
    buffer += sizeof(uint32_t);
    size_t num_players;
    memcpy(&num_players, buffer, sizeof(size_t));
    buffer += sizeof(size_t);
    if (num_players > 0)
    {
        newState.players.resize(num_players);
        memcpy(newState.players.data(), buffer, num_players * sizeof(PlayerState));
        buffer += num_players * sizeof(PlayerState);
    }
    memcpy(&newState.npcs, buffer, sizeof(NPCState) * MAX_NPCS);


    if (!m_masterObjectList || !m_objectListMutex)
        return;

    std::lock_guard<std::mutex> lock(*m_objectListMutex);

    std::set<int> activeIds;
    for (const auto &playerState : newState.players)
        activeIds.insert(playerState.clientId);
    for (const auto &npcState : newState.npcs)
    {
        if (npcState.objectId != -1)
            activeIds.insert(npcState.objectId);
    }

    // --- Process Players ---
    for (const auto &playerState : newState.players)
    {
        GameObject *obj = findGameObjectByClientId(playerState.clientId, *m_masterObjectList);
        if (!obj)
        {
            // This is a new player (could be us or someone else). Create them.
            GameObject *newPlayer = new GameObject();
            newPlayer->setComponent("is_player", true);
            newPlayer->setComponent("client_id", playerState.clientId);
            newPlayer->setComponent("position", Vector(playerState.x, playerState.y));
            newPlayer->setComponent("velocity", Vector(0.0f, 0.0f));
            newPlayer->setComponent("dimensions", Vector(71.0f, 67.0f));
            m_masterObjectList->push_back(newPlayer);
        }
        else if (playerState.clientId != m_clientId)
        {
            // This is a remote player that already exists. Update their position.
            obj->setComponent("position", Vector(playerState.x, playerState.y));
        }
    }

    // --- Process NPCs ---
    for (const auto &npcState : newState.npcs)
    {
        if (npcState.objectId == -1)
            continue;

        GameObject *obj = findGameObjectByNpcId(npcState.objectId, *m_masterObjectList);
        if (!obj)
        {
            // This is a new NPC. Create it.
            GameObject *newNpc = new GameObject();
            newNpc->setComponent("is_npc", true);
            newNpc->setComponent("npc_id", npcState.objectId);
            newNpc->setComponent("position", Vector(npcState.x, npcState.y));
            // Set the initial network target position
            newNpc->setComponent("net_position", Vector(npcState.x, npcState.y));
            newNpc->setComponent("dimensions", Vector(163.0f, 60.0f));
            m_masterObjectList->push_back(newNpc);
        }
        else
        {
            // This NPC already exists. Update its network target position for interpolation.
            obj->setComponent("net_position", Vector(npcState.x, npcState.y));
        }
    }

    // --- Remove disconnected objects ---
    m_masterObjectList->erase(
        std::remove_if(m_masterObjectList->begin(), m_masterObjectList->end(),
                       [&](GameObject *obj)
                       {
                           int id = -1;
                           if (obj->hasComponent("client_id"))
                               id = obj->getComponent<int>("client_id");
                           else if (obj->hasComponent("npc_id"))
                               id = obj->getComponent<int>("npc_id");

                           if (id != -1 && activeIds.find(id) == activeIds.end())
                           {
                               delete obj;
                               return true;
                           }
                           return false;
                       }),
        m_masterObjectList->end());
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
void NetworkManager::handleHandshakes()
{
    while (m_running)
    {
        zmq::message_t req;
        if (!m_handshakeSocket->recv(req, zmq::recv_flags::dontwait))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        int newClientId = m_nextClientId++;

        {
            // Lock the master list before adding a new object
            std::lock_guard<std::mutex> lock(*m_objectListMutex);

            std::cout << "New client connecting. ID: " << newClientId << std::endl;
            GameObject *newPlayer = new GameObject();
            newPlayer->setComponent("is_player", true);
            newPlayer->setComponent("client_id", newClientId);

            // Find a spawn point for the new player
            GameObject *spawnPoint = findAvailableSpawnPoint(*m_masterObjectList);
            if (spawnPoint)
            {
                newPlayer->setComponent("position", spawnPoint->getComponent<Vector>("position"));
            }
            else
            {
                // Default spawn if none are found
                newPlayer->setComponent("position", Vector(0.0f, 0.0f));
            }

            newPlayer->setComponent("velocity", Vector(0.0f, 0.0f));
            newPlayer->setComponent("dimensions", Vector(71.0f, 67.0f));

            m_masterObjectList->push_back(newPlayer);
        }

        int newClientPort = m_startReplyPort + newClientId;
        std::cout << "[Network] Allocated reply port " << newClientPort << " for client " << newClientId << std::endl;
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);

            auto result = m_clients.try_emplace(newClientId);

            ClientConnection &newConnectionInMap = result.first->second;

            newConnectionInMap.id = newClientId;

            newConnectionInMap.thread = std::thread(&NetworkManager::readClient, this, newClientId, newClientPort);
            std::cout << "[Network] Spawned readClient thread for client " << newClientId << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        PlayerState response;
        response.clientId = newClientId;
        // Tell the client which reply port it should use for its REQ socket.
        response.x = newClientPort;
        m_handshakeSocket->send(zmq::buffer(&response, sizeof(PlayerState)));
        std::cout << "[Network] Handshake response sent for client " << newClientId << " port=" << newClientPort << std::endl;
    }
}
void NetworkManager::readClient(int id, int portNum)
{
    zmq::socket_t clientRep(*m_context, zmq::socket_type::rep);
    clientRep.bind("tcp://*:" + std::to_string(portNum));
    std::cout << "[Network] readClient bound REP socket on port " << portNum << " for client " << id << std::endl;
    auto lastTimeRecv = std::chrono::steady_clock::now();

    // Find our own thread object in the clients map to detach on exit
    std::thread *self_thread = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        if (m_clients.count(id))
        {
            self_thread = &m_clients.at(id).thread;
        }
    }

    while (m_running)
    {
        zmq::message_t message;
        if (!clientRep.recv(message, zmq::recv_flags::dontwait))
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsedTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTimeRecv).count();
            // Rate-limit 'no message' logs: only once every 500ms per client
            bool shouldLog = false;
            {
                std::lock_guard<std::mutex> clientsLock(m_clientsMutex);
                if (m_clients.count(id))
                {
                    auto &conn = m_clients.at(id);
                    auto sinceLastLog = std::chrono::duration_cast<std::chrono::milliseconds>(now - conn.lastNoMessageLog).count();
                    if (sinceLastLog >= 500)
                    {
                        shouldLog = true;
                        conn.lastNoMessageLog = now;
                    }
                }
            }
            if (elapsedTimeMs >= m_clientTimeout)
            {
                std::cout << "Client " << id << " timed out. Disconnecting." << std::endl;
                if (self_thread && self_thread->joinable())
                {
                    self_thread->detach();
                }
                // Lock both lists to safely remove the client
                std::lock_guard<std::mutex> clientsLock(m_clientsMutex);
                std::lock_guard<std::mutex> objectLock(*m_objectListMutex);

                m_masterObjectList->erase(
                    std::remove_if(m_masterObjectList->begin(), m_masterObjectList->end(),
                                   [id](GameObject *obj)
                                   {
                                       if (obj->hasComponent("client_id") && obj->getComponent<int>("client_id") == id)
                                       {
                                           delete obj;
                                           return true;
                                       }
                                       return false;
                                   }),
                    m_masterObjectList->end());

                m_clients.erase(id);

                break; // Exit the thread
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        lastTimeRecv = std::chrono::steady_clock::now();
        PlayerState clientState = *message.data<PlayerState>();

        // Rate-limit per-message logging to once per second to avoid console overhead
        static int local_recv_count = 0;
        static auto local_recv_start = std::chrono::steady_clock::now();
        local_recv_count++;
        auto now_local_stats = std::chrono::steady_clock::now();
        auto local_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_local_stats - local_recv_start).count();

        {
            std::lock_guard<std::mutex> lock(*m_objectListMutex);
            GameObject *playerObj = findGameObjectByClientId(id, *m_masterObjectList);
            if (playerObj)
            {
                playerObj->setComponent("position", Vector(clientState.x, clientState.y));
            }
        }

        // Send the simple acknowledgment but avoid logging it per message
        clientRep.send(zmq::buffer(""));
    }
}

void NetworkManager::messageLooper()
{
    // Reusable serialization buffer to avoid per-tick allocations.
    std::vector<char> reuse_buf;

    while (m_running)
    {
        GameState state_to_send;

        {
            std::lock_guard<std::mutex> lock(m_objectListMutex);

            // Reserve space for efficiency
            state_to_send.players.reserve(m_clients.size());

            int npc_idx = 0;
            // Initialize all NPCs to inactive
            for (int i = 0; i < MAX_NPCS; ++i)
                state_to_send.npcs[i].objectId = -1;

            for (const auto &obj : m_masterObjectList)
            {
                std::cout << obj << std::endl;
                if (obj->hasComponent("is_player"))
                {
                    PlayerState p;
                    p.clientId = obj->getComponent<int>("client_id");
                    Vector pos = obj->getComponent<Vector>("position");
                    p.x = pos.x;
                    p.y = pos.y;
                    state_to_send.players.push_back(p);
                }
                else if (obj->hasComponent("is_npc") && npc_idx < MAX_NPCS)
                {
                    NPCState &n = state_to_send.npcs[npc_idx];
                    n.objectId = obj->getComponent<int>("npc_id");
                    Vector pos = obj->getComponent<Vector>("position");
                    n.x = pos.x;
                    n.y = pos.y;
                    npc_idx++;
                }
            }

            state_to_send.num_clients = state_to_send.players.size();
        }

        const size_t num_players = state_to_send.players.size();
        const size_t players_data_size = num_players * sizeof(PlayerState);
        const size_t npc_data_size = sizeof(NPCState) * MAX_NPCS;
        const size_t total_size = sizeof(uint64_t) + sizeof(size_t) + players_data_size + npc_data_size;

        if (reuse_buf.size() < total_size)
            reuse_buf.resize(total_size);
        char *buffer = reuse_buf.data();

        // Write server timestamp first (ms since epoch)
        uint64_t server_ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch())
                                    .count();
        memcpy(buffer, &server_ts_ms, sizeof(uint64_t));
        buffer += sizeof(uint64_t);

        // Write sequence number (uint32_t) to detect missed packets on client
        static uint32_t publish_seq = 0;
        uint32_t seq = publish_seq++;
        memcpy(buffer, &seq, sizeof(uint32_t));
        buffer += sizeof(uint32_t);

        memcpy(buffer, &num_players, sizeof(size_t));
        buffer += sizeof(size_t);
        if (num_players > 0)
        {
            memcpy(buffer, state_to_send.players.data(), players_data_size);
            buffer += players_data_size;
        }
        memcpy(buffer, &state_to_send.npcs, npc_data_size);

        bool send_ok = false;
        auto send_start = std::chrono::steady_clock::now();
        try
        {
            // Use non-blocking send so publisher isn't stalled by slow subscribers
            auto sent = m_publishSocket->send(zmq::buffer(reuse_buf.data(), total_size), zmq::send_flags::dontwait);
            send_ok = static_cast<bool>(sent);
        }
        catch (const zmq::error_t &e)
        {
            if (e.num() == EAGAIN)
            {
                send_ok = false;
            }
            else
            {
                std::cerr << "[Network] publish send threw zmq::error_t: " << e.what() << " (num=" << e.num() << ")" << std::endl;
                send_ok = false;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "[Network] publish send threw exception: " << e.what() << std::endl;
            send_ok = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
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
