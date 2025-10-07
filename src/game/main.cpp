/*
	This is a file that runs the main loop.
	Some of the content in this file was generated with Gemini 2.5 Pro.
	This citation is to abide by the syllabus requirement that "appropriate citations"
	must be given when referring to external sources. More information is available upon request.
*/
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/collisions.h"
#include "../engine/headers/input.h"
#include "../engine/headers/network.h"
#include "../engine/headers/protocol.h"
#include "../engine/headers/entities.h"
#include "../engine/headers/input.h"
#include "../engine/headers/timeline.h"
#include <map>
#include <memory>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

SDL_Renderer *renderer = nullptr;
SDL_Window *window = nullptr;

/**
 * Produces a texture error.
 */
int textureError()
{
	SDL_Log("Could not load image: %s", SDL_GetError());
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 1;
}

/**
 * Synchronizes the collider's position to match the entity's position.
 */
void SyncColliderToEntity(Entity &entity)
{
	if (entity.collider)
	{
		entity.collider->topLeft.x = entity.position.x - entity.dimensions.x / 2.0f;
		entity.collider->topLeft.y = entity.position.y - entity.dimensions.y / 2.0f;
		entity.collider->bottomRight.x = entity.position.x + entity.dimensions.x / 2.0f;
		entity.collider->bottomRight.y = entity.position.y + entity.dimensions.y / 2.0f;
	}
}

// The map of remote players and its mutex
std::map<int, std::unique_ptr<Entity>> remotePlayers;
std::mutex remotePlayersMutex;

/**
 * Thread function to run networking.
 * @param running Pointer to the running flag.
 * @param netManager Pointer to the NetworkManager instance.
 * @param localPlayer Pointer to the local player entity.
 * @param myClientId Pointer to the client's ID.
 */
void network_thread_loop(bool *running, Entity *police, NetworkManager *netManager, Entity *localPlayer, int *myClientId)
{
	while (*running)
	{
		PlayerState myState;
		myState.clientId = *myClientId;
		Vector currentPos = localPlayer->getPosition();
		myState.x = currentPos.x;
		myState.y = currentPos.y;
		netManager->sendPlayerState(myState);

		netManager->update();
		auto latestGameState = netManager->getLatestGameState();

		if (latestGameState.has_value())
		{
			GameState &gs = latestGameState.value();

			// Lock the mutex before accessing the shared remotePlayers map
			std::lock_guard<std::mutex> lock(remotePlayersMutex);

			for (int i = 0; i < gs.num_clients; ++i)
			{
				PlayerState &serverPlayer = gs.players[i];
				if (serverPlayer.clientId != *myClientId)
				{
					if (remotePlayers.find(serverPlayer.clientId) == remotePlayers.end())
					{
						// Create new remote player
						Vector playerDim = {71, 67};
						remotePlayers[serverPlayer.clientId] = std::make_unique<Entity>(Vector{serverPlayer.x, serverPlayer.y}, playerDim, Vector{0, 0}, false);
					}
					else
					{
						// Update existing remote player's position
						remotePlayers.at(serverPlayer.clientId)->setPosition({serverPlayer.x, serverPlayer.y});
					}
				}
			}
			if (gs.npcs[0].objectId == 0) // Check if the server sent data for this NPC
			{
				// Use the thread-safe setter to update the police car's position
				police->setPosition({gs.npcs[0].x, gs.npcs[0].y});
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(33));
	}
}

int main(int argc, char *argv[])
{
	// Initialize the SDL library
	initializeSDL();

	// Initialize the window and renderer using SDL method
	createWindowAndRenderer(&window, &renderer);

	SDL_Texture *platformTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/ENVIRONMENT/props/control-box-3.png");
	if (!platformTexture)
		textureError();

	SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/player/idle/idle-1.png");
	if (!playerTexture)
		textureError();

	SDL_Texture *policeTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/vehicles/v-police.png");
	if (!policeTexture)
		textureError();

	// Create a timeline and a network manager
	Timeline mainTimeline;
	NetworkManager networkManager;

	// Request port 5555 and subscribe port 5556
	const int HANDSHAKE_PORT = 5557;
	const int SUBSCRIBE_PORT = 5556;
	const int BASE_REPLY_PORT = 6000;

	int myClientId = networkManager.connectAndHandshake("localhost", HANDSHAKE_PORT);

	// If the client ID is -1, there was an error
	if (myClientId == -1)
	{
		// Handle error: couldn't connect to server
		std::cerr << "Failed to connect and get a client ID from the server." << std::endl;
		return 1;
	}

	std::cout << "Successfully connected to server. My client ID is: " << myClientId << std::endl;

	int myRequestPort = BASE_REPLY_PORT + myClientId;
	std::cout << "This client will send updates to port: " << myRequestPort << std::endl;

	// Start the main client sockets, connecting to our unique request port
	networkManager.startClient("localhost", myRequestPort, SUBSCRIBE_PORT);

	// Set everything for the platform
	Vector platPos = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
	Vector platDim = {62, 30};
	Vector vel = {0, 0};

	Entity platform(platPos, platDim, vel, false);

	// Create a collider for the platform
	Collider platformCollider(
		platPos.x - platDim.x / 2.0f,
		platPos.y - platDim.y / 2.0f,
		platPos.x + platDim.x / 2.0f,
		platPos.y + platDim.y / 2.0f);
	platform.setCollider(&platformCollider);

	// Create a local player for this particular client.
	Vector playerDim = {71, 67};
	Vector playerPos = {
		WINDOW_WIDTH / 2.0f,
		platPos.y - (platDim.y / 2.0f) - (playerDim.y / 2.0f)};
	Entity localPlayer(playerPos, playerDim, {0, 0}, true);

	Collider playerCollider(
		playerPos.x - playerDim.x / 2.0f,
		playerPos.y - playerDim.y / 2.0f,
		playerPos.x + playerDim.x / 2.0f,
		playerPos.y + playerDim.y / 2.0f);
	localPlayer.setCollider(&playerCollider);
	SyncColliderToEntity(localPlayer);

	// Do the same thing for each of their renderers
	std::map<int, RenderComponent> playerRenderers;

	// Create the RenderComponent for our local player
	playerRenderers.emplace(myClientId, RenderComponent(playerTexture));

	// Create render components for the platform and police car
	RenderComponent platformRenderer(platformTexture);
	RenderComponent policeRenderer(policeTexture);

	Vector policePos = {platPos.x - 100, platPos.y + 50};
	Vector policeDim = {163, 60};
	vel = {150.0f, 0};

	Entity police(policePos, policeDim, vel, false);

	Collider policeCollider(
		policePos.x - policeDim.x / 2.0f,
		policePos.y - policeDim.y / 2.0f,
		policePos.x + policeDim.x / 2.0f,
		policePos.y + policeDim.y / 2.0f);

	police.setCollider(&policeCollider);

	// Main game loop condition variable
	bool running = true;

	std::thread networkThread(network_thread_loop, &running, &police, &networkManager, &localPlayer, &myClientId);

	// SDL_Event to capture event of window being closed
	SDL_Event event;

	// Scaling Type bool
	bool constantSizeScale = true;

	// Variables to track the previous state of toggle keys to prevent flickering. These are for scaling, making player spawn, and time management
	bool graveKeyWasPressedLastFrame = false;
	bool pKeyWasPressedLastFrame = false;
	bool oKeyWasPressedLastFrame = false;
	bool minusKeyWasPressedLastFrame = false;
	bool equalsKeyWasPressedLastFrame = false;
	bool zeroKeywasPressedLastFrame = false;

	// The main game loop
	while (running)
	{

		// Update the timeline and get the current delta time.
		mainTimeline.update();
		float dt = mainTimeline.getDeltaTime();
		const float MAX_DELTA_TIME = 1.0f / 20.0f;
		if (dt > MAX_DELTA_TIME)
		{
			dt = MAX_DELTA_TIME;
		}
		if (dt < 0)
		{
			// A negative dt is a clock error
			// We can print a warning to the console to know when it happens.
			if (dt < 0)
			{
				std::cerr << "WARNING: Negative delta time detected (" << dt << "s). Skipping frame to maintain stability." << std::endl;
			}
			continue; // Immediately start the next loop iteration
		}
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}
		}

		bool graveKeyIsPressedNow = isKeyPressed(SDL_SCANCODE_GRAVE);
		bool oKeyIsPressedNow = isKeyPressed(SDL_SCANCODE_O);
		bool pKeyIsPressedNow = isKeyPressed(SDL_SCANCODE_P);
		bool minusKeyIsPressedNow = isKeyPressed(SDL_SCANCODE_MINUS);
		bool equalsKeyIsPressedNow = isKeyPressed(SDL_SCANCODE_EQUALS);
		bool zeroKeyIsPressedNow = isKeyPressed(SDL_SCANCODE_0);

		// Toggle scaling mode only on the frame the '`' key is first pressed.
		if (graveKeyIsPressedNow && !graveKeyWasPressedLastFrame)
		{
			constantSizeScale = !constantSizeScale;
		}
		if (pKeyIsPressedNow && !pKeyWasPressedLastFrame)
		{
			mainTimeline.pauseTime();
			std::cout << "Timeline Paused." << std::endl;
		}
		if (oKeyIsPressedNow && !oKeyWasPressedLastFrame)
		{
			mainTimeline.unpauseTime();
			std::cout << "Timeline Unpaused." << std::endl;
		}
		if (minusKeyIsPressedNow && !minusKeyWasPressedLastFrame)
		{
			mainTimeline.setTimeScale(0.5);
			std::cout << "Time scale set to 0.5x" << std::endl;
		}
		if (equalsKeyIsPressedNow && !equalsKeyWasPressedLastFrame)
		{
			mainTimeline.setTimeScale(2.0);
			std::cout << "Time scale set to 2.0x" << std::endl;
		}

		if (zeroKeyIsPressedNow && !zeroKeywasPressedLastFrame)
		{
			mainTimeline.setTimeScale(1.0);
			std::cout << "Time scale set to 1.0x" << std::endl;
		}

		// Update last frame key states
		graveKeyWasPressedLastFrame = graveKeyIsPressedNow;
		pKeyWasPressedLastFrame = pKeyIsPressedNow;
		oKeyWasPressedLastFrame = oKeyIsPressedNow;
		minusKeyWasPressedLastFrame = minusKeyIsPressedNow;
		equalsKeyWasPressedLastFrame = equalsKeyIsPressedNow;
		zeroKeywasPressedLastFrame = zeroKeyIsPressedNow;

		// Constant Scaling
		if (constantSizeScale)
		{
			// Resize as if the screen was still the same
			SDL_SetRenderLogicalPresentation(renderer, 1920, 1080, SDL_LOGICAL_PRESENTATION_STRETCH);
		}

		// Proportional Scaling
		else
		{
			// Get Window Size
			int w, h;
			SDL_GetWindowSize(window, &w, &h);
			// Resize as if the screen was still the same
			SDL_SetRenderLogicalPresentation(renderer, w, h, SDL_LOGICAL_PRESENTATION_STRETCH);
		}

		if (isKeyPressed(SDL_SCANCODE_ESCAPE))
		{
			running = false;
		}

		// Reset the player's velocity
		localPlayer.setVelocity({0, localPlayer.getVelocity().y});

		if (isKeyPressed(SDL_SCANCODE_A))
		{
			// Set the velocity using the setter
			Vector currentVel = localPlayer.getVelocity();
			localPlayer.setVelocity({-300.0f, currentVel.y});
		}
		if (isKeyPressed(SDL_SCANCODE_D))
		{
			Vector currentVel = localPlayer.getVelocity();
			localPlayer.setVelocity({300.0f, currentVel.y});
		}
		// Jump can be set directly since it modifies the whole vector
		if (isKeyPressed(SDL_SCANCODE_SPACE))
		{
			localPlayer.setVelocity({localPlayer.getVelocity().x, -300.0f});
		}

		// Rendering
		setupScreen(renderer);

		localPlayer.setVelocity({localPlayer.getVelocity().x, localPlayer.getVelocity().y + WorldPhysics::getGravity() * dt});

		Vector nextPosition = localPlayer.getPosition();
		nextPosition.x += localPlayer.getVelocity().x * dt;
		nextPosition.y += localPlayer.getVelocity().y * dt;

		Collider testCollider(
			nextPosition.x - localPlayer.dimensions.x / 2.0f,
			nextPosition.y - localPlayer.dimensions.y / 2.0f,
			nextPosition.x + localPlayer.dimensions.x / 2.0f,
			nextPosition.y + localPlayer.dimensions.y / 2.0f);

		// Check for collision with the platform
		if (overlappingColliders(testCollider, *platform.collider))
		{

			if (localPlayer.getVelocity().y > 0)
			{
				nextPosition.y = platform.collider->topLeft.y - localPlayer.dimensions.y / 2.0f;
				localPlayer.setVelocity({localPlayer.getVelocity().x, 0});
			}
		}

		localPlayer.setPosition(nextPosition);

		platformRenderer.render(renderer, platform.position, platform.dimensions);
		policeRenderer.render(renderer, police.getPosition(), police.getDimensions());

		// Render the local player
		if (playerRenderers.count(myClientId))
		{
			playerRenderers.at(myClientId).render(renderer, localPlayer.getPosition(), localPlayer.getDimensions());
		}

		{
			std::lock_guard<std::mutex> lock(remotePlayersMutex);
			// Render remote players
			for (auto const &[id, remote_player_ptr] : remotePlayers)
			{
				if (playerRenderers.find(id) == playerRenderers.end())
				{
					playerRenderers.emplace(id, RenderComponent(playerTexture));
				}
				// Use the pointer to get the entity's data
				playerRenderers.at(id).render(renderer, remote_player_ptr->getPosition(), remote_player_ptr->getDimensions());
			}
		}

		refreshScreen(renderer);
	}

	networkThread.join();
	SDL_DestroyTexture(playerTexture);
	SDL_DestroyTexture(platformTexture);
	SDL_DestroyTexture(policeTexture);

	networkManager.cleanUp();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
