/*
	This is the new main file for the client, rebuilt from scratch
	to use the GameObject and Component model. 	Some of the content in this file was generated with Gemini 2.5 Pro.
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
void client_network_thread(bool *running, NetworkManager *netManager, std::vector<GameObject *> *objectList, std::mutex *objectMutex, int clientId)
{
	Timeline networkTimeline;
	// An accumulator to track time passed since the last network send.
	float timeSinceLastSend = 0.0f;

	while (*running)
	{
		// 1. Update the timeline to calculate the delta time for this loop iteration.
		networkTimeline.update();
		float dt = networkTimeline.getDeltaTime();

		// 2. Add the delta time to our accumulator.
		timeSinceLastSend += dt;

		// 3. Check if enough time has passed to send the next update.
		if (timeSinceLastSend >= 0.033f)
		{ // 0.033s is roughly 30 times per second
			// Reset the accumulator.
			timeSinceLastSend = 0.0f;

			// Look up the local player under the object list mutex so we can
			// detect the GameObject created later by the network handshake.
			GameObject *localPlayer = nullptr;
			{
				std::lock_guard<std::mutex> lock(*objectMutex);
				localPlayer = findGameObjectByClientId(clientId, *objectList);
			}
			if (localPlayer)
			{
				// Send our local player's state to the server.
				PlayerState myState;
				myState.clientId = clientId;
				Vector pos = localPlayer->getComponent<Vector>("position");
				myState.x = pos.x;
				myState.y = pos.y;
				try
				{
					std::cout << "[Network] client_network_thread sending PlayerState x=" << myState.x << " y=" << myState.y << std::endl;
					netManager->sendPlayerState(myState);
				}
				catch (const std::exception &e)
				{
					// It's common for the first few sends to fail before the
					// connection is fully established. We just log it and continue.
					std::cerr << "Network send failed (might be connecting): " << e.what() << std::endl;
				}
			}
		}

		// 4. Receive the latest world state from the server (this is non-blocking).
		netManager->update();

		// Give the CPU a tiny break to prevent this thread from running at 100%.
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

int main(int argc, char *argv[])
{
	initializeSDL();
	createWindowAndRenderer(&window, &renderer);

	// Load all textures once at the start
	SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/player/idle/idle-1.png");
	SDL_Texture *platformTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/ENVIRONMENT/props/control-box-3.png");
	SDL_Texture *policeTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/vehicles/v-police.png");

	if (!playerTexture || !platformTexture || !policeTexture)
	{
		SDL_Log("Could not load one or more textures: %s", SDL_GetError());
		return 1;
	}

	std::vector<GameObject *> clientObjectList;
	std::mutex clientObjectListMutex;

	NetworkManager networkManager;
	const int HANDSHAKE_PORT = 5557;
	const int SUBSCRIBE_PORT = 5556;
	int myRequestPort;
	int myClientId = networkManager.connectAndHandshake("localhost", HANDSHAKE_PORT, myRequestPort);

	if (myClientId == -1)
	{
		std::cerr << "Failed to connect to the server." << std::endl;
		return 1;
	}

	std::cout << "Connected with Client ID: " << myClientId << std::endl;
	std::cout << "Server Reply Port is: " << myRequestPort << std::endl;

	// We now pass the port we received from the server to startClient.
	networkManager.startClient("localhost", myRequestPort, SUBSCRIBE_PORT,
							   clientObjectList, clientObjectListMutex);
	networkManager.setClientId(myClientId);

	// Create a local static platform once so the client has something to collide with.
	// This mirrors the server's static platform and prevents the player from falling through
	// if static objects are not broadcast to clients.
	{
		GameObject *platform = new GameObject();
		platform->setComponent("is_platform", true);
		platform->setComponent("object_id", -1);
		platform->setComponent("position", Vector(1920 / 2.0f, 1080 / 2.0f));
		platform->setComponent("dimensions", Vector(62.0f, 30.0f));
		platform->setComponent("texture", platformTexture);
		clientObjectList.push_back(platform);
	}
	// --- END CONNECTION FIX ---

	GameObject *localPlayer = nullptr;

	Timeline mainTimeline;
	bool running = true;
	SDL_Event event;

	std::cout << "Starting network thread..." << std::endl;
	std::thread networkThread(client_network_thread, &running, &networkManager, &clientObjectList, &clientObjectListMutex, myClientId);

	while (running)
	{
		mainTimeline.update();
		float dt = mainTimeline.getDeltaTime();
		const float MAX_DELTA_TIME = 1.0f / 20.0f; // Clamp delta time
		if (dt > MAX_DELTA_TIME)
			dt = MAX_DELTA_TIME;
		if (dt <= 0)
			continue;

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}
		}
		if (isKeyPressed(SDL_SCANCODE_ESCAPE))
		{
			running = false;
		}

		// Lock the mutex for the entire game logic update.
		// This prevents the network thread from changing data while we use it.
		std::lock_guard<std::mutex> lock(clientObjectListMutex);
		if (!localPlayer)
		{
			localPlayer = findGameObjectByClientId(myClientId, clientObjectList);
			if (localPlayer)
			{
				std::cout << "Local player object found!" << std::endl;
			}
		}

		if (localPlayer)
		{
			Vector currentVel = localPlayer->getComponent<Vector>("velocity");
			currentVel.x = 0; // Reset horizontal velocity each frame

			if (isKeyPressed(SDL_SCANCODE_A))
				currentVel.x = -300.0f;
			if (isKeyPressed(SDL_SCANCODE_D))
				currentVel.x = 300.0f;
			if (isKeyPressed(SDL_SCANCODE_SPACE))
			{
				// This is a simplified jump. A real game would check if the player is grounded.
				currentVel.y = -300.0f;
			}
			localPlayer->setComponent("velocity", currentVel);
		}

		// NOTE: Sending of PlayerState is handled by the dedicated network thread
		// (client_network_thread). Removing the duplicate send here prevents
		// multiple threads from using the same REQ socket and causing EFSM
		// errors like "Operation cannot be accomplished in current state".

		// This system loops through ALL objects and applies physics.
		for (auto &obj : clientObjectList)
		{
			if (obj->hasComponent("velocity") && obj->hasComponent("position"))
			{
				Vector vel = obj->getComponent<Vector>("velocity");
				Vector pos = obj->getComponent<Vector>("position");

				// Apply gravity only to players for now
				if (obj->hasComponent("is_player"))
				{
					vel.y += WorldPhysics::getGravity() * dt;
				}

				// Update position based on velocity
				pos.x += vel.x * dt;
				pos.y += vel.y * dt;

				obj->setComponent("position", pos);
				obj->setComponent("velocity", vel);
			}
		}

		// --- Simple collision resolution between players and platforms ---
		// This is a minimal AABB resolver: if the bottom of a player intersects
		// the top of a platform, snap the player to the platform top and zero Y velocity.
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
				bool vertIntersect = (pbottom >= ptop && pvel.y >= 0);
				if (horizOverlap && vertIntersect)
				{
					// Snap player to platform top
					ppos.y = ptop - pdim.y / 2.0f;
					pvel.y = 0.0f;
					playerObj->setComponent("position", ppos);
					playerObj->setComponent("velocity", pvel);
					std::cout << "[Collision] Snapped player to platform: pbottom=" << pbottom << " ptop=" << ptop << " horiz=" << horizOverlap << " vel.y=" << pvel.y << std::endl;
				}
				else
				{
					// Only log when player is far below the platform to avoid spam
					if (ppos.y > 600)
					{
						std::cout << "[Collision] No snap: player y=" << ppos.y << " bottom=" << pbottom << " platform top=" << ptop << " horiz=" << horizOverlap << " vel.y=" << pvel.y << std::endl;
					}
				}
			}
		}

		// --- NPC interpolation: smoothly move NPCs toward the last received network target ---
		for (auto &obj : clientObjectList)
		{
			if (!obj->hasComponent("is_npc") || !obj->hasComponent("position") || !obj->hasComponent("net_position"))
				continue;

			Vector pos = obj->getComponent<Vector>("position");
			Vector net = obj->getComponent<Vector>("net_position");
			// Simple linear interpolation
			float alpha = 0.15f; // smoothing factor
			pos.x = pos.x + (net.x - pos.x) * alpha;
			pos.y = pos.y + (net.y - pos.y) * alpha;
			obj->setComponent("position", pos);
			// Debug log for big jumps
			if (fabs(net.x - pos.x) > 50.0f || fabs(net.y - pos.y) > 50.0f)
			{
				std::cout << "[Interp] NPC " << obj->getComponent<int>("npc_id") << " large delta to net (" << net.x - pos.x << "," << net.y - pos.y << ")" << std::endl;
			}
		}

		// --- Collision System (Simplified) ---
		// A real collision system would be more complex.
		// This is just to demonstrate platform interaction.
		// ... (Collision logic would go here, looping through objects) ...

		// --- Render System ---
		setupScreen(renderer);

		for (auto &obj : clientObjectList)
		{
			// --- Step 1: Assign textures to new objects that don't have one ---
			// This part only runs once per new object.
			if (!obj->hasComponent("texture"))
			{
				if (obj->hasComponent("is_player"))
				{
					obj->setComponent("texture", playerTexture);
				}
				else if (obj->hasComponent("is_npc"))
				{
					// You can get more specific here later, e.g., "npc_type" == "police"
					obj->setComponent("texture", policeTexture);
				}
				else if (obj->hasComponent("is_platform"))
				{
					obj->setComponent("texture", platformTexture);
				}
				// If it's none of the above, it remains texture-less and won't be rendered.
			}

			// --- Step 2: Render all objects that are renderable ---
			// Now that we've assigned textures, this is the only logic we need.
			// It's much simpler and has no repeated code.
			if (obj->hasComponent("texture") && obj->hasComponent("position") && obj->hasComponent("dimensions"))
			{
				SDL_Texture *tex = obj->getComponent<SDL_Texture *>("texture");
				Vector pos = obj->getComponent<Vector>("position");
				Vector dim = obj->getComponent<Vector>("dimensions");

				SDL_FRect destRect;
				destRect.x = pos.x - dim.x / 2.0f;
				destRect.y = pos.y - dim.y / 2.0f;
				destRect.w = dim.x;
				destRect.h = dim.y;

				SDL_RenderTexture(renderer, tex, NULL, &destRect);
			}
		}
		refreshScreen(renderer);
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	std::cout << "Main loop ended. Joining network thread..." << std::endl;
	// The `running` flag being false will signal the network thread to stop its loop.
	// .join() will then wait for it to finish cleanly.
	networkThread.join();
	networkManager.cleanUp();

	// Clean up textures
	SDL_DestroyTexture(playerTexture);
	SDL_DestroyTexture(platformTexture);
	SDL_DestroyTexture(policeTexture);

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
