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

// We now pass pointers to the object list and mutex so this thread can find the player
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

int main(int argc, char *argv[])
{
	initializeSDL();
	createWindowAndRenderer(&window, &renderer);

	SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/player/idle/idle-1.png");
	SDL_Texture *platformTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/ENVIRONMENT/props/control-box-3.png");
	SDL_Texture *policeTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/vehicles/v-police.png");
	SDL_Texture *platform2Texture = IMG_LoadTexture(renderer, "media/warped city files/Assets/ENVIRONMENT/props/control-box-1.png");
	SDL_Texture *hotelSignTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/ENVIRONMENT/props/hotel-sign.png");

	if (!playerTexture || !platformTexture || !policeTexture || !platform2Texture)
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

	networkManager.startClient("localhost", myRequestPort, SUBSCRIBE_PORT, clientObjectList, clientObjectListMutex);
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

	}
	GameObject *localPlayer = nullptr;
	Timeline mainTimeline;
	bool running = true;
	SDL_Event event;

	// We now pass the object list and mutex to the network thread
	std::thread networkThread(client_network_thread, &running, &networkManager, &clientObjectList, &clientObjectListMutex, myClientId);

	while (running)
	{
		mainTimeline.update();
		float dt = mainTimeline.getDeltaTime();
		if (dt <= 0)
			continue;

		// frame start

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
				running = false;
		}
		if (isKeyPressed(SDL_SCANCODE_ESCAPE))
			running = false;

		std::lock_guard<std::mutex> lock(clientObjectListMutex);

		// --- Link to Local Player ---
		if (!localPlayer)
		{
			localPlayer = findGameObjectByClientId(myClientId, clientObjectList);
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
				bool vertIntersect = (pbottom >= ptop && pvel.y >= 0);
				if (horizOverlap && vertIntersect)
				{
					// Snap player to platform top
					ppos.y = ptop - pdim.y / 2.0f;
					pvel.y = 0.0f;
					playerObj->setComponent("position", ppos);
					playerObj->setComponent("velocity", pvel);
				}
				else
				{
					// Only log when player is far below the platform to avoid spam
					if (ppos.y > 600)
					{
						// std::cout << "[Collision] No snap: player y=" << ppos.y << " bottom=" << pbottom << " platform top=" << ptop << " horiz=" << horizOverlap << " vel.y=" << pvel.y << std::endl;
					}
				}
			}
		}

		// --- Render System ---
		setupScreen(renderer);
		for (auto &obj : clientObjectList)
		{
			if (!obj->hasComponent("texture"))
			{
				if (obj->hasComponent("is_player"))
					obj->setComponent("texture", playerTexture);
				else if (obj->hasComponent("is_npc"))
					obj->setComponent("texture", policeTexture);
				else if (obj->hasComponent("is_platform"))
					obj->setComponent("texture", platformTexture);
				else if (obj->hasComponent("is_hotel"))
					obj->setComponent("texture", hotelSignTexture);
			}
			if (obj->hasComponent("texture") && obj->hasComponent("position") && obj->hasComponent("dimensions"))
			{
				SDL_Texture *tex = obj->getComponent<SDL_Texture *>("texture");
				Vector pos = obj->getComponent<Vector>("position");
				Vector dim = obj->getComponent<Vector>("dimensions");
				SDL_FRect destRect = {pos.x - dim.x / 2.0f, pos.y - dim.y / 2.0f, dim.x, dim.y};
				SDL_RenderTexture(renderer, tex, NULL, &destRect);
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
