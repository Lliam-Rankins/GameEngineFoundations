/**
 * Defines networking behavior. Some of the content in this file was edited with AI tools.
    This citation is to abide by the syllabus requirement that "appropriate citations"
    must be given when referring to external sources. More information is available upon request.
 */

#include "../headers/network.h"
#include "../headers/protocol.h"
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
                                 std::vector<GameObject *> &objectList, std::mutex &objectMutex, std::vector<std::shared_ptr<Event>> &clientEventList, std::mutex &eventListMutex)
{
    // Create the context
    m_context = std::make_unique<zmq::context_t>(1);

    // New object stuff
    m_masterObjectList = &objectList;
    m_objectListMutex = &objectMutex;
    m_masterEventList = &clientEventList;
    m_eventListMutex = &eventListMutex;

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

        // For stats we will handle parsing below; continue draining to get latest
    }

    if (!gotAny)
    {
        // No messages available right now
        return;
    }

    // duration since last processed update (in ms)
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(lastRecvThisCall - lastUpdateTime).count();
    lastUpdateTime = lastRecvThisCall;

    // Avoid noisy per-message prints on the client which can slow rendering.
    static int recv_count = 0;
    static auto recv_stats_start = std::chrono::steady_clock::now();
    static long long latency_total_ms = 0;
    static int latency_samples = 0;
    // We processed the latest message; treat it as our received update.
    // Increment recv_count by 1 for the latest message (we could also count
    // the number drained, but that would bias msgs/sec upward in bursts).
    recv_count++;

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

    // Compute one-way latency estimate (requires clocks to be approximately synced)
    auto client_recv_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();
    long long latency_ms = (long long)client_recv_time - (long long)server_ts_ms;

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
    buffer += sizeof(NPCState) * MAX_NPCS;
    size_t num_events;
    memcpy(&num_events, buffer, sizeof(size_t));
    buffer += sizeof(size_t);
    if(num_events > 0) {
        newState.eventList.resize(num_events);
        memcpy(newState.eventList.data(), buffer, num_events * sizeof(NetworkEvent));
        buffer += num_events * sizeof(NetworkEvent);
        //std::cout << newState.eventList[1].type << std::endl;
    }
    newState.num_events = num_events;
    //std::cout << newState.num_events << std::endl;
    

    // accumulate latency stats for this received message
    latency_total_ms += latency_ms;
    latency_samples++;

    // Inter-arrival / missing-packet stats
    static uint32_t last_seq = 0;
    static bool have_last_seq = false;
    static long long interarrival_total_ms = 0;
    static int interarrival_samples = 0;
    static int missed_in_window = 0;

    if (have_last_seq)
    {
        // compute delta between last_seq and current seq (handle wrap)
        uint32_t delta = (seq >= last_seq) ? (seq - last_seq) : (UINT32_MAX - last_seq + 1 + seq);
        // delta includes the number of sequence increments since last_seq
        // drained_count is how many messages we actually received in this call
        int missed = 0;
        if ((uint32_t)drained_count < delta)
        {
            missed = (int)(delta - (uint32_t)drained_count);
            missed_in_window += missed;
        }
    }
    last_seq = seq;
    have_last_seq = true;

    auto now_stats = std::chrono::steady_clock::now();
    auto recv_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now_stats - recv_stats_start).count();

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
            newNpc->setComponent("position", Vector{npcState.x, npcState.y});
            m_masterObjectList->push_back(newNpc);
        }
        else
        {
            // This NPC already exists. Update its network target position for interpolation.
            obj->setComponent("position", Vector(npcState.x, npcState.y));
        }
    }
    // --- Process Events ---
    {
        std::lock_guard<std::mutex> lock3(*m_eventListMutex);
        for(const auto &event : newState.eventList) {
            if(event.type == -1) continue;

            //std::cout << "Event type: " << event.type << std::endl;

            if(event.type == 1) {
                auto collisionEvent = std::make_shared<CollisionEvent>(event.timestamp, event.id1, event.id2);
                m_masterEventList->push_back(collisionEvent);
            } else if(event.type == 2) {
                auto inputEvent = std::make_shared<InputEvent>(event.timestamp, event.action, event.id1);
                m_masterEventList->push_back(inputEvent);
            } else if(event.type == 3) {
                auto deathEvent = std::make_shared<DeathEvent>(event.timestamp, event.id1);
                m_masterEventList->push_back(deathEvent);
            } else if(event.type == 4) {
                auto spawnEvent = std::make_shared<SpawnEvent>(event.timestamp, event.id1, event.x, event.y);
                m_masterEventList->push_back(spawnEvent);
            }

            std::cout << "Event registered!" << event.type << std::endl;
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

// void NetworkManager::update()
// {
//     zmq::message_t gameStateMessage;
//     bool gotAny = false;
//     static auto lastUpdateTime = std::chrono::steady_clock::now();
//     static auto lastRecvTime = std::chrono::steady_clock::now();
//     auto firstRecvTime = std::chrono::steady_clock::time_point();
//     auto lastRecvThisCall = std::chrono::steady_clock::time_point();

//     int drained_count = 0;

//     while (true)
//     {
//         zmq::message_t msg;
//         // non-blocking recv
//         bool ok = static_cast<bool>(m_subscribeSocket->recv(msg, zmq::recv_flags::dontwait));
//         if (!ok)
//             break;
//         // capture the most recent message
//         gameStateMessage = std::move(msg);
//         auto now_recv = std::chrono::steady_clock::now();
//         if (!gotAny)
//         {
//             firstRecvTime = now_recv;
//             gotAny = true;
//         }
//         lastRecvThisCall = now_recv;
//         drained_count++;

//     }

//     if (!gotAny)
//     {
//         // No messages available 
//         return;
//     }

//     // Parse the latest incoming message (server timestamp + payload)
//     GameState newState;
//     const char *buffer = gameStateMessage.data<const char>();

//     // Read server timestamp (ms since epoch) for one-way latency measurement
//     uint64_t server_ts_ms = 0;
//     memcpy(&server_ts_ms, buffer, sizeof(uint64_t));
//     buffer += sizeof(uint64_t);

//     // Read sequence number (uint32_t) to detect missed packets
//     uint32_t seq = 0;
//     memcpy(&seq, buffer, sizeof(uint32_t));
//     buffer += sizeof(uint32_t);
//     size_t num_players;
//     memcpy(&num_players, buffer, sizeof(size_t));
//     buffer += sizeof(size_t);
//     if (num_players > 0)
//     {
//         newState.players.resize(num_players);
//         memcpy(newState.players.data(), buffer, num_players * sizeof(PlayerState));
//         buffer += num_players * sizeof(PlayerState);
//     }
//     memcpy(&newState.npcs, buffer, sizeof(NPCState) * MAX_NPCS);


//     if (!m_masterObjectList || !m_objectListMutex)
//         return;

//     std::lock_guard<std::mutex> lock(*m_objectListMutex);

//     std::set<int> activeIds;
//     for (const auto &playerState : newState.players)
//         activeIds.insert(playerState.clientId);
//     for (const auto &npcState : newState.npcs)
//     {
//         if (npcState.objectId != -1)
//             activeIds.insert(npcState.objectId);
//     }

//     // --- Process Players ---
//     for (const auto &playerState : newState.players)
//     {
//         GameObject *obj = findGameObjectByClientId(playerState.clientId, *m_masterObjectList);
//         if (!obj)
//         {
//             // This is a new player (could be us or someone else). Create them.

//             GameObject *newPlayer = new GameObject();
//             newPlayer->setComponent("is_player", true);
//             newPlayer->setComponent("client_id", playerState.clientId);
//             newPlayer->setComponent("position", Vector(playerState.x, playerState.y));
//             m_masterObjectList->push_back(newPlayer);
//         }
//         else if (playerState.clientId != m_clientId)
//         {
//             // This is a remote player that already exists. Update their position.
//             obj->setComponent("position", Vector(playerState.x, playerState.y));
//         }
//     }

//     // --- Process NPCs ---
//     for (const auto &npcState : newState.npcs)
//     {
//         if (npcState.objectId == -1)
//             continue;

//         GameObject *obj = findGameObjectByNpcId(npcState.objectId, *m_masterObjectList);
//         if (!obj)
//         {
//             // This is a new NPC. Create it.
//             GameObject *newNpc = new GameObject();
//             newNpc->setComponent("is_npc", true);
//             newNpc->setComponent("npc_id", npcState.objectId);
//             newNpc->setComponent("position", Vector{npcState.x, npcState.y});
//             m_masterObjectList->push_back(newNpc);
//         }
//         else
//         {
//             // This NPC already exists. Update its position for interpolation.
//             obj->setComponent("position", Vector(npcState.x, npcState.y));
//         }
//     }

//     /////////////////////////////////////////
//     //  Ai Use, ChatGPT: provided the original disconect code and explained different variables and what this was supposed to do, and asked it to re write to actually work
//     /////////////////////////////////////////
//     auto it = m_masterObjectList->begin();
//     while (it != m_masterObjectList->end())
//     {
//         GameObject *obj = *it;
//         bool isPlayer = obj->hasComponent("is_player");
//         bool isNpc = obj->hasComponent("is_npc");

//         int id = -1;
//         if (isPlayer)
//             id = obj->getComponent<int>("client_id");
//         else if (isNpc)
//             id = obj->getComponent<int>("npc_id");

//         // If it's a player or NPC and no longer active, remove it
//         if ((isPlayer || isNpc) && activeIds.find(id) == activeIds.end())
//         {
//             // Optionally: log or trigger any cleanup here
//             delete obj; // free memory if you own it
//             it = m_masterObjectList->erase(it);
//         }
//         else
//         {
//             ++it;
//         }
//     }
// }

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

            // Timing Out 
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

        // Need to make object
        {
            std::lock_guard<std::mutex> lock(*m_objectListMutex);
            GameObject *playerObj = findGameObjectByClientId(id, *m_masterObjectList);
            if (playerObj) // Have matching
            {
                playerObj->setComponent("position", Vector(clientState.x, clientState.y));
            }
            else // No existing remote player
            {
                playerObj = new GameObject();
                playerObj->setComponent("is_player", true);
                playerObj->setComponent("client_id", clientState.clientId);
                playerObj->setComponent("position", Vector(clientState.x, clientState.y));

                m_masterObjectList->push_back(playerObj);
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_gameStateMut);
            m_gameState.num_events += clientState.num_events;
            std::cout << m_gameState.num_events << std::endl;
            for(int i = 0; i < clientState.num_events; i++) {
                m_gameState.eventList.push_back(clientState.events[i]);
            }
        }

        // Send the simple acknowledgment but avoid logging it per message
        clientRep.send(zmq::buffer(""));
    }
}

void NetworkManager::sendServerEvents(NetworkEvent serverEvents[], int eventCt) {
    std::lock_guard<std::mutex> lock (m_gameStateMut);
    for(int i = 0; i < eventCt; i++) {
        m_gameState.eventList.push_back(serverEvents[i]);
        m_gameState.num_events++;
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

// void NetworkManager::messageLooper()
// {
//     // Reusable serialization buffer to avoid per-tick allocations.
//     std::vector<char> reuse_buf;

//     while (m_running)
//     {
//         GameState state_to_send;

//         {
//             std::lock_guard<std::mutex> lock(*m_objectListMutex);

//             // Reserve space for efficiency
//             state_to_send.players.reserve(m_clients.size());

//             int npc_idx = 0;
//             // Initialize all NPCs to inactive
//             for (int i = 0; i < MAX_NPCS; ++i)
//                 state_to_send.npcs[i].objectId = -1;

//             for (const auto &obj : *m_masterObjectList)
//             {
//                 if (obj->hasComponent("is_player"))
//                 {
//                     PlayerState p;
//                     p.clientId = obj->getComponent<int>("client_id");
//                     Vector pos = obj->getComponent<Vector>("position");
//                     p.x = pos.x;
//                     p.y = pos.y;
//                     state_to_send.players.push_back(p);
//                 }
//                 else if (obj->hasComponent("is_npc") && npc_idx < MAX_NPCS)
//                 {
//                     NPCState &n = state_to_send.npcs[npc_idx];
//                     n.objectId = obj->getComponent<int>("npc_id");
//                     Vector pos = obj->getComponent<Vector>("position");
//                     n.x = pos.x;
//                     n.y = pos.y;
//                     npc_idx++;
//                 }
//             }

//             state_to_send.num_clients = state_to_send.players.size();
//         }

//         const size_t num_players = state_to_send.players.size();
//         const size_t players_data_size = num_players * sizeof(PlayerState);
//         const size_t npc_data_size = sizeof(NPCState) * MAX_NPCS;
//         const size_t total_size = sizeof(uint64_t) + sizeof(size_t) + players_data_size + npc_data_size;

//         if (reuse_buf.size() < total_size)
//             reuse_buf.resize(total_size);
//         char *buffer = reuse_buf.data();

//         // Write server timestamp first (ms since epoch)
//         uint64_t server_ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
//                                     std::chrono::system_clock::now().time_since_epoch())
//                                     .count();
//         memcpy(buffer, &server_ts_ms, sizeof(uint64_t));
//         buffer += sizeof(uint64_t);

//         // Write sequence number (uint32_t) to detect missed packets on client
//         static uint32_t publish_seq = 0;
//         uint32_t seq = publish_seq++;
//         memcpy(buffer, &seq, sizeof(uint32_t));
//         buffer += sizeof(uint32_t);

//         memcpy(buffer, &num_players, sizeof(size_t));
//         buffer += sizeof(size_t);
//         if (num_players > 0)
//         {
//             memcpy(buffer, state_to_send.players.data(), players_data_size);
//             buffer += players_data_size;
//         }
//         memcpy(buffer, &state_to_send.npcs, npc_data_size);

//         bool send_ok = false;
//         auto send_start = std::chrono::steady_clock::now();
//         try
//         {
//             // Use non-blocking send so publisher isn't stalled by slow subscribers
//             auto sent = m_publishSocket->send(zmq::buffer(reuse_buf.data(), total_size), zmq::send_flags::dontwait);
//             send_ok = static_cast<bool>(sent);
//         }
//         catch (const zmq::error_t &e)
//         {
//             if (e.num() == EAGAIN)
//             {
//                 send_ok = false;
//             }
//             else
//             {
//                 std::cerr << "[Network] publish send threw zmq::error_t: " << e.what() << " (num=" << e.num() << ")" << std::endl;
//                 send_ok = false;
//             }
//         }
//         catch (const std::exception &e)
//         {
//             std::cerr << "[Network] publish send threw exception: " << e.what() << std::endl;
//             send_ok = false;
//         }

//         std::this_thread::sleep_for(std::chrono::milliseconds(16));
//     }
// }

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


// In network.cpp
void NetworkManager::stopServer()
{
    std::cout << "NetworkManager::stopServer() called." << std::endl;
    m_running = false; // Signal all threads to stop

    // 1. Join main server threads first
    if (m_handshakeThread.joinable())
    {
        m_handshakeThread.join();
    }
    if (m_updateThread.joinable())
    {
        m_updateThread.join();
    }
    std::cout << "[Network] Main server threads joined." << std::endl;

    // 2. Safely get a list of client threads to join
    std::vector<std::thread> client_threads_to_join;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        std::cout << "[Network] Moving " << m_clients.size() << " client threads to join vector..." << std::endl;
        for (auto &pair : m_clients)
        {
            if (pair.second.thread.joinable())
            {
                // Move the thread object into our local vector
                client_threads_to_join.push_back(std::move(pair.second.thread));
            }
        }
        m_clients.clear(); // The map is now empty
    } // m_clientsMutex is RELEASED here

    // 3. Join all client threads (now safe, no lock is held)
    std::cout << "[Network] Joining " << client_threads_to_join.size() << " client threads..." << std::endl;
    for (auto &th : client_threads_to_join)
    {
        th.join();
    }
    std::cout << "[Network] All client threads joined." << std::endl;

    // 4. Cleanup ZMQ sockets
    cleanUp();
}

int NetworkManager::getConnectedClientCount()
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    return m_clients.size();
}

void NetworkManager::setNetworkingStrategy(const std::string &strategy)
{
    // Use a lock in case the messageLooper thread reads it at the same time
    std::lock_guard<std::mutex> lock(m_strategyMutex);
    m_strategy = strategy;
    std::cout << "NetworkManager strategy set to: " << m_strategy << std::endl;
}
