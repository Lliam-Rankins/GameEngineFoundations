/*
	This is the new main file for the client, rebuilt from scratch
	to use the GameObject and Component model. Some of the content in this file was edited with AI tools.
	This citation is to abide by the syllabus requirement that "appropriate citations"
	must be given when referring to external sources. More information is available upon request.
*/
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include <cmath>
#include <memory>
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/input.h"
#include "../engine/headers/network.h"
#include "../engine/headers/protocol.h"
#include "../engine/headers/timeline.h"
#include "../engine/headers/GameObject.h"
#include "../engine/headers/gameUtils.h"

SDL_Renderer *renderer = nullptr;
SDL_Window *window = nullptr;

const float SCREEN_W = 1920.0f;
const float SCREEN_H = 1080.0f;

// Replace with your actual world extents:
const float WORLD_WIDTH = 10000.0f;
const float WORLD_HEIGHT = 8000.0f;

const int ACTION_MOVE_LEFT = 1;
const int ACTION_MOVE_RIGHT = 2;
const int ACTION_JUMP = 3;
const int ACTION_STOP_X = 4;

std::vector<std::shared_ptr<Event>> clientOutboxList;
std::mutex clientOutboxMutex;
std::vector<GameObject *> clientObjectList;
std::mutex clientObjectListMutex;
std::vector<std::shared_ptr<Event>> clientEventList;
std::mutex eventListMutex;
NetworkEvent outgoingEvents[32];
int numOutgoingEvents = 0;
std::mutex outgoingEventsMutex;

/**
 * Runs the client network thread
 */
void client_network_thread(bool *running, NetworkManager *netManager, std::vector<GameObject *> *objectList, std::mutex *objectMutex, int clientId)
{
	Timeline networkTimeline;
	float timeSinceLastSend = 0.0f;

	while (*running)
	{
		networkTimeline.update();
		float dt = networkTimeline.getDeltaTime();
		timeSinceLastSend += dt;

		if (timeSinceLastSend >= 0.033f)
		{
			timeSinceLastSend = 0.0f;

			GameObject *localPlayer = nullptr;

			{
				// We must lock the mutex to safely search the list
				std::lock_guard<std::mutex> lock(*objectMutex);
				localPlayer = findGameObjectByClientId(clientId, *objectList);
			}

			if (localPlayer)
			{
				PlayerState myState;
				myState.clientId = clientId;
				Vector pos = localPlayer->getComponent<Vector>("position");
				myState.x = pos.x;
				myState.y = pos.y;
				try
				{
					netManager->sendPlayerState(myState);
				}
				catch (const std::exception &e)
				{
					std::cerr << "Network send failed (might be connecting): " << e.what() << std::endl;
				}
			}
		}

		// This is the ONLY place update() is called.
		netManager->update();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

void sendPlayerThread(bool *running, NetworkManager *network, std::vector<GameObject *> *list, std::mutex *mut, NetworkEvent *localEvents, int *numEvents, std::mutex *eventMut, int id)
{
	// While the game is running...
	while (*running)
	{
		// Get this player...
		GameObject *myPlayer = nullptr;
		{
			// Lock it all down
			std::lock_guard<std::mutex> lock(*mut);
			myPlayer = findGameObjectByClientId(id, *list);
		}
		// If I couldn't get the player,  restart
		if (!myPlayer)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
			continue;
		}
		// Create a PlayerState based on current player data and send it out
		PlayerState playerState;
		playerState.clientId = id;
		playerState.x = myPlayer->getComponent<Vector>("position").x;
		playerState.y = myPlayer->getComponent<Vector>("position").y;

		{
			std::lock_guard<std::mutex> lock2(*eventMut);
			playerState.num_events = (*numEvents);
			for (int i = 0; i < playerState.num_events; i++)
			{
				playerState.events[i] = localEvents[i];
			}
			// std::cout << "Num Local Events: " << (*numEvents) << std::endl;
			(*numEvents) = 0;
		}
		network->sendPlayerState(playerState);
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}

/**
 * Helper function to queue and record events
 */
void QueueAndRecordEvent(
	std::shared_ptr<Event> event,
	EventManager &eventManager,
	NetworkEvent *outbox,
	int *outboxCount,
	std::mutex &outboxMutex,
	bool isPlayingReplay)
{
	// Don't process new events while playing back a replay
	if (isPlayingReplay)
	{
		return;
	}

	// 1. Queue the event locally (so our own client reacts)
	eventManager.QueueEvent(event);

	// 2. Check if it's an event we need to send to the server
	EventTypeID type = event->GetEventTypeID();
	bool shouldSend = (type == InputEvent::STATIC_EVENT_TYPE_ID ||
					   type == DeathEvent::STATIC_EVENT_TYPE_ID);

	if (shouldSend)
	{
		NetworkEvent netEvent;
		netEvent.timestamp = event->timestamp;

		if (type == InputEvent::STATIC_EVENT_TYPE_ID)
		{
			auto input = static_cast<InputEvent *>(event.get());
			netEvent.type = 3; // Corresponds to InputEvent (from protocol.h)
			netEvent.action = input->action;
			netEvent.id1 = input->playerID;
		}
		else if (type == DeathEvent::STATIC_EVENT_TYPE_ID)
		{
			auto death = static_cast<DeathEvent *>(event.get());
			netEvent.type = 1; // Corresponds to DeathEvent (from protocol.h)
			netEvent.id1 = death->entityID;
		}

		// --- ADD TO OUTBOX (Thread-Safe) ---
		std::lock_guard<std::mutex> lock(outboxMutex);
		if (*outboxCount < 32)
		{ // Don't overflow the buffer
			outbox[*outboxCount] = netEvent;
			(*outboxCount)++;
		}
	}
}

int main(int argc, char *argv[])
{
	initializeSDL();
	createWindowAndRenderer(&window, &renderer);

	SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/player/idle/idle-1.png");
	SDL_Texture *platformTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/ENVIRONMENT/props/control-box-3.png");
	SDL_Texture *policeTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/vehicles/v-police.png");
	SDL_Texture *platform2Texture = IMG_LoadTexture(renderer, "media/warped city files/Assets/ENVIRONMENT/props/control-box-1.png");
	SDL_Texture *hotelSignTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/ENVIRONMENT/props/hotel-sign.png");
	SDL_Texture *turretTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/misc/turret/turret-1.png");
	SDL_Texture *droneTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/misc/drone/drone-1.png");

	if (!playerTexture || !platformTexture || !policeTexture || !platform2Texture)
	{
		SDL_Log("Could not load one or more textures: %s", SDL_GetError());
		return 1;
	}

	NetworkManager networkManager;
	const int HANDSHAKE_PORT = 5557;
	const int SUBSCRIBE_PORT = 5556;
	int myRequestPort;

	// --- START REPLACEMENT ---
	int myClientId = -1;
	const int MAX_RETRIES = 10; // Try for 10 seconds
	int retries = 0;

	std::cout << "[Client] Started. Attempting to connect to server..." << std::endl;

	// Keep trying to connect until we succeed or run out of retries
	while (myClientId == -1 && retries < MAX_RETRIES)
	{
		// This will print "Sent handshake..." from inside your network code
		myClientId = networkManager.connectAndHandshake("localhost", HANDSHAKE_PORT, myRequestPort);

		if (myClientId == -1)
		{
			retries++;
			std::cerr << "[Client] Connection attempt " << retries << " failed. Retrying in 1 second..." << std::endl;
			// Wait 1 second before trying again
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}

	// If we still failed after all retries, then we give up.
	if (myClientId == -1)
	{
		std::cerr << "[Client] FAILED to connect to server after " << MAX_RETRIES << " attempts." << std::endl;
		return 1; // Now it's a real failure
	}

	std::cout << "[Client] Successfully connected with Client ID: " << myClientId << std::endl;

	if (myClientId == -1)
	{
		std::cerr << "Failed to connect to the server." << std::endl;
		return 1;
	}

	networkManager.startClient("localhost", myRequestPort, SUBSCRIBE_PORT, clientObjectList, clientObjectListMutex, clientEventList, eventListMutex);
	networkManager.setClientId(myClientId);
	{
		GameObject *platform = new GameObject();
		platform->setComponent("is_platform", true);
		platform->setComponent("object_id", -1);
		platform->setComponent("position", Vector(1920 / 2.0f, 1080 / 2.0f));
		platform->setComponent("dimensions", Vector(62.0f, 30.0f));
		platform->setComponent("texture", platformTexture);
		clientObjectList.push_back(platform);

		GameObject *platform2 = new GameObject();
		platform2->setComponent("is_platform", true);
		platform2->setComponent("object_id", -2);
		platform2->setComponent("position", Vector(1920 / 2.0f + 300.0f, 1080 / 2.0f + 150.0f));
		platform2->setComponent("dimensions", Vector(32.0f, 30.0f));
		platform2->setComponent("texture", platform2Texture);
		clientObjectList.push_back(platform2);

		GameObject *platform3 = new GameObject();
		platform3->setComponent("is_platform", true);
		platform3->setComponent("object_id", -4);
		platform3->setComponent("position", Vector(10.0f, 50.0f));
		platform3->setComponent("dimensions", Vector(62.0f, 30.0f));
		platform3->setComponent("texture", platformTexture);
		clientObjectList.push_back(platform3);

		GameObject *turret = new GameObject();
		turret->setComponent("is_platform", true);
		turret->setComponent("object_id", -3);
		turret->setComponent("position", Vector(2100.0f, 1080 / 2.0f));
		turret->setComponent("dimensions", Vector(25.0f, 23.0f));
		turret->setComponent("texture", turretTexture);
		clientObjectList.push_back(turret);

		GameObject *drone = new GameObject();
		drone->setComponent("is_npc", true);
		drone->setComponent("npc_id", 2);
		drone->setComponent("position", Vector(2100.0f, 1080 / 2.0f - 200.0f));
		drone->setComponent("dimensions", Vector(55.0f, 52.0f));
		drone->setComponent("texture", droneTexture);
		clientObjectList.push_back(drone);

		// Spawn Point 1 (Above platform 1)
		GameObject *spawnPoint1 = new GameObject();
		spawnPoint1->setComponent("is_spawnpoint", true);
		spawnPoint1->setComponent("spawn_id", 1);
		spawnPoint1->setComponent("position", Vector(1920 / 2.0f, 1080 / 2.0f - 50.0f)); // (960, 490)
		clientObjectList.push_back(spawnPoint1);

		// Spawn Point 2 (Above platform 2)
		GameObject *spawnPoint2 = new GameObject();
		spawnPoint2->setComponent("is_spawnpoint", true);
		spawnPoint2->setComponent("spawn_id", 2);
		// Positioned slightly above platform 2
		spawnPoint2->setComponent("position", Vector(1920 / 2.0f + 300.0f, 1080 / 2.0f + 150.0f - 50.0f)); // (1260, 640)
		clientObjectList.push_back(spawnPoint2);

		// Death Zone 1 (Gap between P1 and P2)
		GameObject *deathZone1 = new GameObject();
		deathZone1->setComponent("is_deathzone", true);
		deathZone1->setComponent("spawn_id", 1);
		deathZone1->setComponent("position", Vector(1117.5f, 800.0f));
		deathZone1->setComponent("dimensions", Vector(100.0f, 400.0f));
		clientObjectList.push_back(deathZone1);

		// Death Zone 2 (Gap between P2 and Turret)
		GameObject *deathZone2 = new GameObject();
		deathZone2->setComponent("is_deathzone", true);
		deathZone2->setComponent("spawn_id", 2);
		deathZone2->setComponent("position", Vector(1681.75f, 800.0f));
		deathZone2->setComponent("dimensions", Vector(400.5f, 400.0f));
		clientObjectList.push_back(deathZone2);
	}

	GameObject *localPlayer = nullptr;
	// It is a camera whoa
	GameObject *cameraObject = new GameObject();
	cameraObject->setComponent("position", Vector(0.0f, 0.0f));

	Timeline mainTimeline;
	bool running = true;
	bool cameraInitialized = false;
	SDL_Event event;
	EventManager eventManager;

	eventManager.RegisterListener(CollisionEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e)
								  {
									  const auto &collision = static_cast<const CollisionEvent &>(e);
									  {
										  if (collision.objectB_ID == -1 || collision.objectB_ID == -2 || collision.objectB_ID == -3 || collision.objectB_ID == -4)
										  {
											  localPlayer->setComponent("velocity", Vector{localPlayer->getComponent<Vector>("velocity").x, 0});
										  }
									  }
									  // TODO: More collision stuff?
								  });

	// Find this listener:
	eventManager.RegisterListener(DeathEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e)
								  {
									  const auto &death = static_cast<const DeathEvent &>(e);

									  // Only process if it's our player
									  if (localPlayer && death.entityID == myClientId)
									  {
										  std::cout << "Handling DeathEvent for player " << myClientId << std::endl;

										  // Find the default spawn point (e.g., spawn_id 1)
										  // You can make this logic more complex (e.g., find closest)
										  Vector respawnPos = {1920 / 2.0f, 1080 / 2.0f - 50.0f}; // Default
										  bool spawnFound = false;
										  for (auto &spObj : clientObjectList)
										  {
											  if (spObj->hasComponent("is_spawnpoint") && spObj->getComponent<int>("spawn_id") == 1)
											  {
												  respawnPos = spObj->getComponent<Vector>("position");
												  spawnFound = true;
												  break;
											  }
										  }

										  // Create and queue a SpawnEvent
										  auto spawn = std::make_shared<SpawnEvent>(
											  mainTimeline.getDeltaTime(), // Or getCurrentTime()
											  myClientId,
											  respawnPos.x,
											  respawnPos.y);
											  QueueAndRecordEvent(spawn, eventManager, clientOutboxList, clientOutboxMutex);
									  } });

	eventManager.RegisterListener(SpawnEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e)
								  {
		const auto &spawn = static_cast<const SpawnEvent &>(e);
		localPlayer->setComponent("position", Vector{spawn.x, spawn.y}); });

	// Find this listener:
	eventManager.RegisterListener(InputEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e)
								  {
									  const auto &input = static_cast<const InputEvent &>(e);

									  // Only act on events for our local player
									  if (!localPlayer || input.playerID != myClientId)
										  return;

									  Vector currentVel = localPlayer->getComponent<Vector>("velocity");

									  switch (input.action)
									  {
									  case ACTION_MOVE_LEFT:
										  currentVel.x = -300.0f;
										  break;
									  case ACTION_MOVE_RIGHT:
										  currentVel.x = 300.0f;
										  break;
									  case ACTION_JUMP:
										  currentVel.y = -300.0f;
										  break;
									  case ACTION_STOP_X:
										  currentVel.x = 0.0f;
										  break;
									  }
									  localPlayer->setComponent("velocity", currentVel); });

	std::thread networkThread(
		sendPlayerThread,
		&running,
		&networkManager,
		&clientObjectList,
		&clientObjectListMutex,
		outgoingEvents,		  
		&numOutgoingEvents,	  
		&outgoingEventsMutex, 
		myClientId);

	while (running)
	{
		mainTimeline.update();
		float dt = mainTimeline.getDeltaTime();
		if (dt <= 0)
			continue;

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
				running = false;

			if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
			{
				std::shared_ptr<InputEvent> input = nullptr;
				float time = mainTimeline.getDeltaTime();

				switch (event.key.scancode)
				{
				case SDL_SCANCODE_A:
					input = std::make_shared<InputEvent>(time, ACTION_MOVE_LEFT, myClientId);
					break;
				case SDL_SCANCODE_D:
					input = std::make_shared<InputEvent>(time, ACTION_MOVE_RIGHT, myClientId);
					break;
				case SDL_SCANCODE_SPACE:
					input = std::make_shared<InputEvent>(time, ACTION_JUMP, myClientId);
					break;
				default:
					break;
				}
				if (input)
				{
					QueueAndRecordEvent(input, eventManager, outgoingEvents, outgoingEventsMutex, isPlaying);
				}
			}
			else if (event.type == SDL_EVENT_KEY_UP)
			{
				std::shared_ptr<InputEvent> input = nullptr;
				float time = mainTimeline.getDeltaTime();

				switch (event.key.scancode)
				{
				case SDL_SCANCODE_A:
				case SDL_SCANCODE_D:
					input = std::make_shared<InputEvent>(time, ACTION_STOP_X, myClientId);
					break;
				default:
					break;
				}
				if (input)
				{
					QueueAndRecordEvent(input, eventManager, clientOutboxList, clientOutboxMutex);
				}
			}
		}

		std::lock_guard<std::mutex> lock(clientObjectListMutex);

		{
			// Lock the *event* list
			std::lock_guard<std::mutex> eventLock(eventListMutex);

			if (!clientEventList.empty())
			{
				std::cout << "Received " << clientEventList.size() << " events from server." << std::endl;

				// Queue all received events into our local EventManager
				for (const auto &netEvent : clientEventList)
				{
					QueueAndRecordEvent(netEvent, eventManager, clientOutboxList, clientOutboxMutex);
				}
				// Clear the list now that we've processed them
				clientEventList.clear();
			}
		}

		// --- Link to Local Player ---
		if (!localPlayer)
		{
			localPlayer = findGameObjectByClientId(myClientId, clientObjectList);
		}

		if (localPlayer && !cameraInitialized)
		{
			Vector ppos = localPlayer->getComponent<Vector>("position");
			Vector initialCam = Vector(ppos.x - SCREEN_W / 2.0f, ppos.y - SCREEN_H / 2.0f);
			cameraObject->setComponent("position", initialCam);
			cameraInitialized = true;
		}

		// --- Input System ---
		if (localPlayer)
		{
			Vector currentVel = localPlayer->getComponent<Vector>("velocity");
			currentVel.x = 0;
			if (isKeyPressed(SDL_SCANCODE_A))
				currentVel.x = -300.0f;
			if (isKeyPressed(SDL_SCANCODE_D))
				currentVel.x = 300.0f;
			if (isKeyPressed(SDL_SCANCODE_SPACE))
				currentVel.y = -300.0f;
			localPlayer->setComponent("velocity", currentVel);
		}

		if (localPlayer)
		{
			Vector cameraPos = cameraObject->getComponent<Vector>("position");
			Vector playerPos = localPlayer->getComponent<Vector>("position");

			float xDifference = playerPos.x - cameraPos.x - (SCREEN_W / 2.0f);
			cameraPos.x += xDifference * 0.05f;

			float yDifference = playerPos.y - cameraPos.y - (SCREEN_H / 2.0f);
			cameraPos.y += yDifference * 0.05f;

			if (cameraPos.x < 0.0f)
				cameraPos.x = 0.0f;
			if (cameraPos.y < 0.0f)
				cameraPos.y = 0.0f;
			if (cameraPos.x > WORLD_WIDTH - SCREEN_W)
				cameraPos.x = WORLD_WIDTH - SCREEN_W;
			if (cameraPos.y > WORLD_HEIGHT - SCREEN_H)
				cameraPos.y = WORLD_HEIGHT - SCREEN_H;

			cameraObject->setComponent("position", cameraPos);
		}

		// --- Physics & Interpolation System ---
		for (auto &obj : clientObjectList)
		{
			if (obj->hasComponent("position"))
			{
				if (obj == localPlayer)
				{
					// Apply physics directly to our own player for responsiveness
					Vector vel = obj->getComponent<Vector>("velocity");
					Vector pos = obj->getComponent<Vector>("position");
					vel.y += WorldPhysics::getGravity() * dt;
					pos.x += vel.x * dt;
					pos.y += vel.y * dt;
					obj->setComponent("position", pos);
					obj->setComponent("velocity", vel);
				}
				else if (obj->hasComponent("is_npc") && obj->hasComponent("net_position"))
				{
					// For NPCs, smoothly interpolate towards their network target using
					// time-correct exponential smoothing. This adapts smoothing to the
					// client's frame delta so motion stays smooth even if dt varies.
					Vector pos = obj->getComponent<Vector>("position");
					Vector netPos = obj->getComponent<Vector>("net_position");
					// Smoothing time constant (seconds). ~0.1 gives ~100ms response.
					const float tau = 0.1f;
					float alpha = 1.0f - std::exp(-dt / tau);
					// Clamp alpha to a sensible range
					if (alpha < 0.0f)
						alpha = 0.0f;
					if (alpha > 1.0f)
						alpha = 1.0f;
					pos.x = pos.x + (netPos.x - pos.x) * alpha;
					pos.y = pos.y + (netPos.y - pos.y) * alpha;
					obj->setComponent("position", pos);
				}
			}
		}

		for (auto &playerObj : clientObjectList)
		{
			if (!playerObj->hasComponent("is_player") || !playerObj->hasComponent("position") || !playerObj->hasComponent("dimensions") || !playerObj->hasComponent("velocity"))
				continue;

			Vector ppos = playerObj->getComponent<Vector>("position");
			Vector pdim = playerObj->getComponent<Vector>("dimensions");
			Vector pvel = playerObj->getComponent<Vector>("velocity");

			float pleft = ppos.x - pdim.x / 2.0f;
			float pright = ppos.x + pdim.x / 2.0f;
			float pbottom = ppos.y + pdim.y / 2.0f;

			for (auto &platObj : clientObjectList)
			{
				if (!platObj->hasComponent("is_platform") || !platObj->hasComponent("position") || !platObj->hasComponent("dimensions"))
					continue;

				Vector platPos = platObj->getComponent<Vector>("position");
				Vector platDim = platObj->getComponent<Vector>("dimensions");

				float ptop = platPos.y - platDim.y / 2.0f;
				float pleftPlat = platPos.x - platDim.x / 2.0f;
				float prightPlat = platPos.x + platDim.x / 2.0f;

				// Check horizontal overlap and vertical intersection
				bool horizOverlap = (pright > pleftPlat && pleft < prightPlat);
				float prev_pbottom = (ppos.y - pvel.y * dt) + pdim.y / 2.0f;

				bool movingDown = (pvel.y > 0);
				bool wasAbove = (prev_pbottom <= ptop);
				bool isNowOnOrBelow = (pbottom >= ptop);

				// Vertical intersection check
				bool vertIntersect = (movingDown && wasAbove && isNowOnOrBelow);
				if (horizOverlap && vertIntersect)
				{
					// Snap player to platform top
					ppos.y = ptop - pdim.y / 2.0f;
					pvel.y = 0.0f;
					playerObj->setComponent("position", ppos);
					playerObj->setComponent("velocity", pvel);

					auto collision = std::make_shared<CollisionEvent>(
						mainTimeline.getDeltaTime(),			   // Or getCurrentTime()
						playerObj->getComponent<int>("client_id"), // Assumes player has client_id
						platObj->getComponent<int>("object_id"));
					QueueAndRecordEvent(collision, eventManager, clientOutboxList, clientOutboxMutex);
				}
			}
		}

		if (localPlayer)
		{
			// Get player's location
			Vector ppos = localPlayer->getComponent<Vector>("position");
			Vector pdim = localPlayer->getComponent<Vector>("dimensions");
			float pleft = ppos.x - pdim.x / 2.0f;
			float pright = ppos.x + pdim.x / 2.0f;
			float ptop = ppos.y - pdim.y / 2.0f;
			float pbottom = ppos.y + pdim.y / 2.0f;

			for (auto &dzObj : clientObjectList)
			{
				if (dzObj->hasComponent("is_deathzone") && dzObj->hasComponent("position") && dzObj->hasComponent("dimensions"))
				{
					// Get death zone's area
					Vector dzPos = dzObj->getComponent<Vector>("position");
					Vector dzDim = dzObj->getComponent<Vector>("dimensions");
					float dzleft = dzPos.x - dzDim.x / 2.0f;
					float dzright = dzPos.x + dzDim.x / 2.0f;
					float dztop = dzPos.y - dzDim.y / 2.0f;
					float dzbottom = dzPos.y + dzDim.y / 2.0f;

					// Collision check
					bool collision = (pright > dzleft && pleft < dzright && pbottom > dztop && ptop < dzbottom);

					if (collision)
					{
						std::cout << "Player " << myClientId << " hit death zone." << std::endl;
						// Create a death event for our player
						auto death = std::make_shared<DeathEvent>(
							mainTimeline.getDeltaTime(), // Or getCurrentTime()
							myClientId);
						QueueAndRecordEvent(death, eventManager, clientOutboxList, clientOutboxMutex);
						// Stop checking for other death zones now
						break;
					}
				}
			}
		}
		eventManager.ProcessEvents(mainTimeline.getDeltaTime());
		// Rendering
		setupScreen(renderer);
		{
			Vector cameraPos = cameraObject->getComponent<Vector>("position");
			for (auto &obj : clientObjectList)
			{
				if (!obj->hasComponent("texture"))
				{
					if (obj->hasComponent("is_player"))
						obj->setComponent("texture", playerTexture);
					else if (obj->hasComponent("is_npc"))
					{
						if (obj->getComponent<int>("npc_id") == 0)
							obj->setComponent("texture", policeTexture);
						else if (obj->getComponent<int>("npc_id") == 1)
							obj->setComponent("texture", hotelSignTexture);
						else if (obj->getComponent<int>("npc_id") == 2)
							obj->setComponent("texture", droneTexture);
					}
					else if (obj->hasComponent("is_platform"))
						obj->setComponent("texture", platformTexture);
				}
				if (obj->hasComponent("texture") && obj->hasComponent("position") && obj->hasComponent("dimensions"))
				{
					SDL_Texture *tex = obj->getComponent<SDL_Texture *>("texture");
					Vector pos = obj->getComponent<Vector>("position");
					Vector dim = obj->getComponent<Vector>("dimensions");

					float screenX = pos.x - cameraPos.x - (dim.x / 2.0f);
					float screenY = pos.y - cameraPos.y - (dim.y / 2.0f);

					SDL_FRect destRect = {screenX, screenY, dim.x, dim.y};
					SDL_RenderTexture(renderer, tex, NULL, &destRect);
				}
			}
		}
		refreshScreen(renderer);
	}

	running = false;
	networkThread.join();
	networkManager.cleanUp();

	// Clean up textures
	SDL_DestroyTexture(playerTexture);
	SDL_DestroyTexture(platformTexture);
	SDL_DestroyTexture(policeTexture);
	SDL_DestroyTexture(platform2Texture);

	// Clean up GameObjects
	for (auto &obj : clientObjectList)
	{
		delete obj;
	}
	clientObjectList.clear();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
