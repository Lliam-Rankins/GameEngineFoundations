/**
 * Defines networking behavior.Some of the content in this file was generated with Gemini 2.5 Pro.
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
        // This gets the blocking call out of the main update loop.
        zmq::message_t confirmation;
        auto result = m_requestSocket->recv(confirmation);
        if (!result)
        {
            std::cerr << "[Network] Warning: Failed to receive server confirmation." << std::endl;
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
            // A more advanced system could check if the spawn point is occupied.
            return obj;
        }
    }
    return nullptr; // No spawn points found
}

void NetworkManager::update()
{
    // --- PART 1: RECEIVE AND DESERIALIZE (This part is unchanged) ---
    zmq::message_t gameStateMessage;
    auto result = m_subscribeSocket->recv(gameStateMessage, zmq::recv_flags::dontwait);

    if (!result.has_value() || result.value() <= 0)
    {
        // No new message, nothing to do.
        return;
    }

    std::cout << "[Network] Received GameState message of size " << gameStateMessage.size() << std::endl;

    // Create a GameState struct from the raw network buffer
    GameState newState;
    const char *buffer = gameStateMessage.data<const char>();

    size_t num_players;
    memcpy(&num_players, buffer, sizeof(size_t));
    buffer += sizeof(size_t);

    if (num_players > 0)
    {
        newState.players.resize(num_players);
        const size_t players_data_size = num_players * sizeof(PlayerState);
        memcpy(newState.players.data(), buffer, players_data_size);
        buffer += players_data_size;
    }

    const size_t npc_data_size = sizeof(NPCState) * MAX_NPCS;
    memcpy(&newState.npcs, buffer, npc_data_size);

    // --- PART 2: SYNCHRONIZE GAMEOBJECT LIST (THIS IS THE NEW LOGIC) ---

    // Before we modify the list, check if the pointers were set correctly.
    if (!m_masterObjectList || !m_objectListMutex)
    {
        return; // Safety check, do nothing if not properly initialized.
    }

    // Lock the mutex. No other thread (like the main game loop) can touch
    // the object list while we are modifying it.
    std::lock_guard<std::mutex> lock(*m_objectListMutex);

    // To efficiently find disconnected players, we first add all active IDs from
    // the new packet into a set for quick lookups.
    std::set<int> activeIds;
    for (const auto &playerState : newState.players)
    {
        activeIds.insert(playerState.clientId);
    }
    std::cout << "[Network] GameState contains " << newState.players.size() << " players" << std::endl;
    for (const auto &npcState : newState.npcs)
    {
        if (npcState.objectId != -1) // Assuming -1 means an inactive NPC slot
        {
            activeIds.insert(npcState.objectId);
        }
    }

    // --- Step 2a: Update existing objects and create new ones ---

    // Process all players from the new state
    for (const auto &playerState : newState.players)
    {
        // Don't update our own local player, as we are the authority on its position.
        // NOTE: You need to get your client ID into the NetworkManager to check this.
        // For now, we'll update all objects.
        GameObject *obj = findLocalObject(playerState.clientId, *m_masterObjectList);

        // If this playerState refers to our local client, do not overwrite the local
        // position if the object already exists. The client is authoritative for
        // its immediate movement and applies gravity locally; the server should
        // not stomp the client's position each update or it will teleport.
        if (playerState.clientId == m_clientId)
        {
            if (!obj)
            {
                // Create our local player object if it doesn't exist yet.
                std::cout << "New player detected with ID: " << playerState.clientId << ". Creating GameObject." << std::endl;
                GameObject *newPlayer = new GameObject();
                newPlayer->setComponent("is_player", true);
                newPlayer->setComponent("client_id", playerState.clientId);
                newPlayer->setComponent("position", Vector(playerState.x, playerState.y));
                newPlayer->setComponent("velocity", Vector(0.0f, 0.0f));
                newPlayer->setComponent("dimensions", Vector(71.0f, 67.0f));
                m_masterObjectList->push_back(newPlayer);
            }
            // If the object already exists and is our local player, skip updating
            // its position so the client's physics remains smooth and authoritative.
            std::cout << "[Network] Skipping server update for local player " << playerState.clientId << std::endl;
            continue;
        }

        if (obj)
        {
            // The object already exists (not our local player), so just update its position component.
            std::cout << "[Network] Updating remote player " << playerState.clientId << " to x=" << playerState.x << " y=" << playerState.y << std::endl;
            obj->setComponent("position", Vector(playerState.x, playerState.y));
        }
        else
        {
            // This is a new player we haven't seen before. Create a new GameObject.
                std::cout << "New player detected with ID: " << playerState.clientId << ". Creating GameObject." << std::endl;
            GameObject *newPlayer = new GameObject();
            newPlayer->setComponent("is_player", true);
            newPlayer->setComponent("client_id", playerState.clientId);
            newPlayer->setComponent("position", Vector(playerState.x, playerState.y));
            newPlayer->setComponent("velocity", Vector(0.0f, 0.0f));
            // Use the dimensions from your old main.cpp. A better system would send this.
            newPlayer->setComponent("dimensions", Vector(71.0f, 67.0f));

            // NOTE: The texture is missing! The rendering system in your main.cpp
            // will need to check for new objects and assign them a texture.

            m_masterObjectList->push_back(newPlayer);
        }
    }

    // Process all NPCs from the new state
    for (const auto &npcState : newState.npcs)
    {
        if (npcState.objectId == -1)
            continue; // Skip inactive NPCs

        // Use the NPC-specific lookup to avoid colliding with client IDs
        GameObject *obj = findGameObjectByNpcId(npcState.objectId, *m_masterObjectList);

        if (obj)
        {
            // This NPC already exists; set its network target position instead
            // of stomping the local position. The client will interpolate.
            std::cout << "[Network] Updating NPC " << npcState.objectId << " net_position=" << npcState.x << "," << npcState.y << std::endl;
            obj->setComponent("net_position", Vector(npcState.x, npcState.y));
        }
        else
        {
            // This is a new NPC. Create a new GameObject for it.
            std::cout << "New NPC detected with ID: " << npcState.objectId << ". Creating GameObject." << std::endl;
            GameObject *newNpc = new GameObject();
            newNpc->setComponent("is_npc", true);
            newNpc->setComponent("npc_id", npcState.objectId);
            newNpc->setComponent("position", Vector(npcState.x, npcState.y));
            newNpc->setComponent("net_position", Vector(npcState.x, npcState.y));
            newNpc->setComponent("velocity", Vector(0.0f, 0.0f));      // Server will dictate this
            newNpc->setComponent("dimensions", Vector(163.0f, 60.0f)); // Police car dimensions

            m_masterObjectList->push_back(newNpc);
        }
    }

    // --- Step 2b: Remove disconnected objects ---
    // We use the erase-remove idiom for compatibility.
    m_masterObjectList->erase(
        std::remove_if(m_masterObjectList->begin(), m_masterObjectList->end(),
                       [&](GameObject *obj)
                       {
                           int id = -1;
                           if (obj->hasComponent("client_id"))
                           {
                               // Don't remove ourself!
                               if (obj->getComponent<int>("client_id") == m_clientId)
                                   return false;
                               id = obj->getComponent<int>("client_id");
                           }
                           else if (obj->hasComponent("npc_id"))
                           {
                               id = obj->getComponent<int>("npc_id");
                           }

                                   if (id != -1 && activeIds.find(id) == activeIds.end())
                                   {
                                       // This object's ID was NOT in the latest packet. It has disconnected.
                                       std::cout << "Object with ID: " << id << " disconnected. Deleting GameObject." << std::endl;
                                       delete obj;  // Free the memory
                                       return true; // Return true to erase it from the vector
                                   }

                           return false; // Keep the object
                       }),
        m_masterObjectList->end());

    {
        std::lock_guard<std::mutex> gameStatelock(m_gameStateMut);
        m_gameState = std::move(newState);
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

        // --- BRIDGE LOGIC: CREATE GAMEOBJECT FOR NEW PLAYER ---
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

            // 1. Use try_emplace. This is the key change.
            // It finds the key `newClientId`. If it doesn't exist, it constructs
            // a ClientConnection object IN-PLACE using its default constructor.
            // This avoids any illegal move or copy operations.
            auto result = m_clients.try_emplace(newClientId);

            // 2. Get a reference to the object that is now safely inside the map.
            // result.first is an iterator to the map element. ->second gets the value.
            ClientConnection &newConnectionInMap = result.first->second;

            // 3. Now, populate the members of the object that's already in its final destination.
            newConnectionInMap.id = newClientId;
            // The 'running' flag is already true by default from the struct definition.

            // 4. Create the thread and move-assign it into the struct's thread member.
            // This is legal because std::thread is movable.
            newConnectionInMap.thread = std::thread(&NetworkManager::readClient, this, newClientId, newClientPort);
            std::cout << "[Network] Spawned readClient thread for client " << newClientId << std::endl;
        }
        // --- END FIX ---

        // The rest of the function proceeds as before...
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 2. We now PAUSE the handshake thread for a brief moment.
        // This gives the OS time to schedule the new readClient thread and
        // for that thread to successfully bind its REP socket to the new port.
        // This is the critical synchronization step that prevents the race condition.
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
    auto lastTimeRecv = std::chrono::high_resolution_clock::now();

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
            // --- TIMEOUT & DISCONNECT LOGIC ---
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTimeRecv).count();
            if (elapsedTime > 500)
            {
                std::cout << "[Network] readClient(" << id << ") no message for " << elapsedTime << "ms" << std::endl;
            }
            if (elapsedTime >= m_clientTimeout)
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

    lastTimeRecv = std::chrono::high_resolution_clock::now();
    PlayerState clientState = *message.data<PlayerState>();
    std::cout << "[Network] readClient(" << id << ") received PlayerState x=" << clientState.x << " y=" << clientState.y << std::endl;

        {
            std::lock_guard<std::mutex> lock(*m_objectListMutex);
            GameObject *playerObj = findGameObjectByClientId(id, *m_masterObjectList);
            if (playerObj)
            {
                playerObj->setComponent("position", Vector(clientState.x, clientState.y));
            }
        }

        clientRep.send(zmq::buffer(""));
        std::cout << "[Network] readClient(" << id << ") sent acknowledgment" << std::endl;
    }
}

void NetworkManager::messageLooper()
{
    while (m_running)
    {
        GameState state_to_send;

        // --- BRIDGE LOGIC: BUILD PACKET FROM GAMEOBJECTS ---
        {
            std::lock_guard<std::mutex> lock(*m_objectListMutex);

            // Reserve space for efficiency
            state_to_send.players.reserve(m_clients.size());

            int npc_idx = 0;
            // Initialize all NPCs to inactive
            for (int i = 0; i < MAX_NPCS; ++i)
                state_to_send.npcs[i].objectId = -1;

            for (const auto &obj : *m_masterObjectList)
            {
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
                // Platforms and other static objects can also be sent this way
                // if you add them to your GameState struct.
            }
            state_to_send.num_clients = state_to_send.players.size();
        }
        // --- END BRIDGE LOGIC ---

        // --- SERIALIZATION (Unchanged) ---
        const size_t num_players = state_to_send.players.size();
        const size_t players_data_size = num_players * sizeof(PlayerState);
        const size_t npc_data_size = sizeof(NPCState) * MAX_NPCS;
        const size_t total_size = sizeof(size_t) + players_data_size + npc_data_size;
        zmq::message_t message(total_size);
        char *buffer = message.data<char>();

        memcpy(buffer, &num_players, sizeof(size_t));
        buffer += sizeof(size_t);
        if (num_players > 0)
        {
            memcpy(buffer, state_to_send.players.data(), players_data_size);
            buffer += players_data_size;
        }
        memcpy(buffer, &state_to_send.npcs, npc_data_size);

        auto sent = m_publishSocket->send(message, zmq::send_flags::none);
        if (!sent)
        {
            std::cerr << "[Network] Warning: publish send failed" << std::endl;
        }
        else
        {
            std::cout << "[Network] Published GameState (players=" << state_to_send.players.size() << ")" << std::endl;
        }
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
