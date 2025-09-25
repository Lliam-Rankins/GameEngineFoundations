/*
	This is a file that runs the main loop.
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
#include <SDL3_image/SDL_image.h>
#include <iostream>


SDL_Renderer *renderer = nullptr;
SDL_Window *window = nullptr;

int textureError()
{
	SDL_Log("Could not load image: %s", SDL_GetError());
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 1;
}

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

int main(int argc, char *argv[])
{
	// Initialize the SDL library
	initializeSDL();

	// Initialize the window and renderer using SDL method
	createWindowAndRenderer(&window, &renderer);

	SDL_Texture *platformTexture = IMG_LoadTexture(renderer, "media/warped city files/warped city files/Assets/ENVIRONMENT/props/control-box-3.png");
	if (!platformTexture)
		textureError();

	SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "media/warped city files/warped city files/Assets/SPRITES/player/idle/idle-1.png");
	if (!playerTexture)
		textureError();

	SDL_Texture *policeTexture = IMG_LoadTexture(renderer, "media/warped city files/warped city files/Assets/SPRITES/vehicles/v-police.png");
	if (!policeTexture)
		textureError();

	// Create a timeline and a network manager
	Timeline mainTimeline;
	NetworkManager networkManager;

	// Request port 5555 and subscribe port 5556
	const int REQUEST_PORT = 5555;
	const int SUBSCRIBE_PORT = 5556;
	networkManager.startClient("localhost", REQUEST_PORT, SUBSCRIBE_PORT);

	// Get back a client ID.
	int myClientId = networkManager.connectAndHandshake();

	if (myClientId == -1)
	{
		// Handle error: couldn't connect to server
		std::cerr << "Failed to connect and get a client ID from the server." << std::endl;
		return 1;
	}

	std::cout << "Successfully connected to server. My client ID is: " << myClientId << std::endl;

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

	// Create a map to hold all the other remote players with key as their ID and entity as the value
	std::map<int, Entity> remotePlayers;

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

	// The main game loop
	while (running)
	{

		// Update the timeline and get the current delta time.
		mainTimeline.update();
		float dt = mainTimeline.getDeltaTime();

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

		// Toggle scaling mode only on the frame the '`' key is first pressed.
		if (graveKeyIsPressedNow && !graveKeyWasPressedLastFrame)
		{
			constantSizeScale = !constantSizeScale;
		}
		// === TIMELINE INTEGRATION 4: KEYBOARD CONTROLS FOR TIME ===
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
			mainTimeline.setTimeScale(0.5); // Slow motion
			std::cout << "Time scale set to 0.5x" << std::endl;
		}
		if (equalsKeyIsPressedNow && !equalsKeyWasPressedLastFrame)
		{
			mainTimeline.setTimeScale(2.0); // Fast-forward
			std::cout << "Time scale set to 2.0x" << std::endl;
		}

		// You might want a key to reset time scale to 1.0 as well.
		// TODO: this *******************************************

		// Update last frame key states
		graveKeyWasPressedLastFrame = graveKeyIsPressedNow;
		pKeyWasPressedLastFrame = pKeyIsPressedNow;
		oKeyWasPressedLastFrame = oKeyIsPressedNow;
		minusKeyWasPressedLastFrame = minusKeyIsPressedNow;
		equalsKeyWasPressedLastFrame = equalsKeyIsPressedNow;

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
		localPlayer.velocity.x = 0;

		if (isKeyPressed(SDL_SCANCODE_A))
		{
			localPlayer.velocity.x = -5.0f; // Velocity in pixels per second
		}
		if (isKeyPressed(SDL_SCANCODE_D))
		{
			localPlayer.velocity.x = 5.0f; // Velocity in pixels per second
		}

		// Jump is an impulse (instantaneous change), so it doesn't use dt.
		if (isKeyPressed(SDL_SCANCODE_SPACE))
		{
			localPlayer.velocity.y = -5.0f; // A stronger jump impulse
		}

		localPlayer.updatePosition();

		// Package the localPlayer's current state into a PlayerState struct and send it
		PlayerState myState;
		myState.clientId = myClientId;
		myState.x = localPlayer.position.x;
		myState.y = localPlayer.position.y;
		networkManager.sendPlayerState(myState);

		// Update the network manager to get the latest gamestate/broadcast
		networkManager.update();
		auto latestGameState = networkManager.getLatestGameState();

		if (latestGameState.has_value())
		{
			GameState &gs = latestGameState.value();

			// Loop through the players in the GameState from the server.
			for (int i = 0; i < gs.num_clients; ++i)
			{
				PlayerState &serverPlayer = gs.players[i];

				// If the ID is -1 (just created), we need to get a new client ID.
				if (myClientId == -1)
				{
					// Client ID is equal to the server player's client ID
					myClientId = serverPlayer.clientId;
				}

				// If the player from the server is NOT us, update them.
				if (serverPlayer.clientId != myClientId)
				{
					if (remotePlayers.find(serverPlayer.clientId) == remotePlayers.end())
					{
						// If not, create a new data-only Entity for them.
						remotePlayers.emplace(serverPlayer.clientId, Entity({serverPlayer.x, serverPlayer.y}, {71, 67}));
						std::cout << "New player joined with ID: " << serverPlayer.clientId << std::endl;
					}
					else
					{
						// If they already exist, just update their position data.
						remotePlayers.at(serverPlayer.clientId).position = {serverPlayer.x, serverPlayer.y};
					}

					// Check if we have a visual RenderComponent for this remote player yet.
					if (playerRenderers.find(serverPlayer.clientId) == playerRenderers.end())
					{
						// If not, create a new RenderComponent for them using the shared player texture.
						playerRenderers.emplace(serverPlayer.clientId, RenderComponent(playerTexture));
					}
				}
			}
		}

		// Rendering
		setupScreen(renderer);

		// Create a leftmost bound for police car position
		int leftBound = policePos.x - 100;
		int rightBound = policePos.x + 100;

		if (police.position.x > rightBound)
			police.velocity = {-150.0f, 0};
		if (police.position.x < leftBound)
			police.velocity = {150.0f, 0};

		// === TIMELINE INTEGRATION 6: TIME-CORRECTED MOVEMENT ===
		// Update police position based on velocity over time.
		police.position.x += police.velocity.x * dt;

		// Apply gravity to the player (acceleration over time).
		if (localPlayer.physicsApplied)
		{
			// Gravity is now an acceleration (pixels per second, per second)
			localPlayer.velocity.y += WorldPhysics::getGravity() * dt;
		}

		// Update player position based on velocity over time.
		localPlayer.position.x += localPlayer.velocity.x * dt;
		localPlayer.position.y += localPlayer.velocity.y * dt;
		SyncColliderToEntity(localPlayer);

		// Collision check
		if (overlappingColliders(*localPlayer.collider, *platform.collider))
		{
			if (localPlayer.velocity.y > 0)
			{
				localPlayer.position.y = platform.position.y - platform.dimensions.y / 2.0f - localPlayer.dimensions.y / 2.0f;
				localPlayer.collider->topLeft.y = localPlayer.position.y - localPlayer.dimensions.y / 2.0f;
				localPlayer.collider->bottomRight.y = localPlayer.position.y + localPlayer.dimensions.y / 2.0f;
				localPlayer.velocity.y = 0;
			}
		}

		if (overlappingColliders(*localPlayer.collider, *police.collider))
		{
			localPlayer.position.x = playerPos.x;
			localPlayer.position.y = playerPos.y;
		}

		platformRenderer.render(renderer, platform.position, platform.dimensions);
		policeRenderer.render(renderer, police.position, police.dimensions);

		// Render the local player
		if (playerRenderers.count(myClientId))
		{
			playerRenderers.at(myClientId).render(renderer, localPlayer.position, localPlayer.dimensions);
		}

		// Render remote players
		for (auto const &[id, remote_player] : remotePlayers)
		{
			// Check if we have a renderer for this remote player yet
			if (playerRenderers.find(id) == playerRenderers.end())
			{
				// If not, create one!
				playerRenderers.emplace(id, RenderComponent(playerTexture));
			}
			playerRenderers.at(id).render(renderer, remote_player.position, remote_player.dimensions);
		}

		refreshScreen(renderer);
	}

	SDL_DestroyTexture(playerTexture);
	SDL_DestroyTexture(platformTexture);
	SDL_DestroyTexture(policeTexture);

	networkManager.cleanUp();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
