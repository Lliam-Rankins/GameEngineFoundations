/*
	This is a component file that outlines and implements rendering.
*/
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/collisions.h"
#include "../engine/headers/network.h"
#include "../engine/headers/protocol.h"
#include <map>

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

	// Don't show the player initially
	bool showPlayer = false;

	SDL_Texture *platformTexture = IMG_LoadTexture(renderer, "media/warped city files/warped city files/Assets/ENVIRONMENT/props/control-box-3.png");
	if (!platformTexture)
		textureError();

	SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "media/warped city files/warped city files/Assets/SPRITES/player/idle/idle-1.png");
	if (!playerTexture)
		textureError();

	SDL_Texture *policeTexture = IMG_LoadTexture(renderer, "media/warped city files/warped city files/Assets/SPRITES/vehicles/v-police.png");
	if (!policeTexture)
		textureError();

	NetworkManager networkManager;

	// Request port 5555 and subscribe port 5556
	const int REQUEST_PORT = 5555;
	const int SUBSCRIBE_PORT = 5556;
	networkManager.startClient("localhost", REQUEST_PORT, SUBSCRIBE_PORT);

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

	Entity platform(platPos, platDim, platformTexture, false, vel);

	// Create a collider for the platform
	Collider platformCollider(
		platPos.x - platDim.x / 2.0f,
		platPos.y - platDim.y / 2.0f,
		platPos.x + platDim.x / 2.0f,
		platPos.y + platDim.y / 2.0f);
	platform.setCollider(&platformCollider);

	// NETWORKING: Create ONE entity for the player this client controls.
	Vector playerPos = {WINDOW_WIDTH / 2, platPos.y - 100};
	Vector playerDim = {71, 67};
	Entity localPlayer(playerPos, playerDim, playerTexture, true, {0, 0});

	Collider playerCollider(
		playerPos.x - playerDim.x / 2.0f,
		playerPos.y - playerDim.y / 2.0f,
		playerPos.x + playerDim.x / 2.0f,
		playerPos.y + playerDim.y / 2.0f);
	localPlayer.setCollider(&playerCollider);

	// NETWORKING: Create a map to hold all the OTHER players.
	// The key is their unique client ID, and the value is their Entity object.
	std::map<int, Entity> remotePlayers;

	Vector policePos = {platPos.x - 100, platPos.y + 50};
	Vector policeDim = {163, 60};
	vel = {1, 0};

	Entity police(policePos, policeDim, policeTexture, false, vel);

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

	// Variables to track the previous state of toggle keys to prevent flickering.
	bool eKeyWasPressedLastFrame = false;
	bool graveKeyWasPressedLastFrame = false;

	// The main game loop
	while (running)
	{

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
			}
		}

		bool eKeyIsPressedNow = isKeyPressed(SDL_SCANCODE_E);
		bool graveKeyIsPressedNow = isKeyPressed(SDL_SCANCODE_GRAVE);

		// Toggle player visibility only on the frame the 'E' key is first pressed.
		if (eKeyIsPressedNow && !eKeyWasPressedLastFrame)
		{
			showPlayer = !showPlayer;
			localPlayer.position = {WINDOW_WIDTH / 2.0f, platPos.y - 300.0f};
			localPlayer.velocity = {0, 0};
			SyncColliderToEntity(localPlayer);
		}

		// Toggle scaling mode only on the frame the '`' key is first pressed.
		if (graveKeyIsPressedNow && !graveKeyWasPressedLastFrame)
		{
			constantSizeScale = !constantSizeScale;
		}

		// Update the tracking variables for the next frame.
		eKeyWasPressedLastFrame = eKeyIsPressedNow;
		graveKeyWasPressedLastFrame = graveKeyIsPressedNow;

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

		// Testing to allow the keypress of "ESC" to exit the window.
		if (isKeyPressed(SDL_SCANCODE_ESCAPE))
		{
			running = false;
		}
		if (isKeyPressed(SDL_SCANCODE_A))
		{
			localPlayer.velocity.x = -5.0f;
		}
		if (isKeyPressed(SDL_SCANCODE_D))
		{
			localPlayer.velocity.x = 5.0f;
		}

		if (isKeyPressed(SDL_SCANCODE_SPACE))
		{
			localPlayer.velocity.y = -30;
		}

		localPlayer.updatePosition();

		// --- 2. NETWORKING: SEND STATE ---
		// Package the localPlayer's current state into a PlayerState struct.
		PlayerState myState;
		myState.clientId = myClientId;
		myState.x = localPlayer.position.x;
		myState.y = localPlayer.position.y;
		networkManager.sendPlayerState(myState);

		// --- 3. NETWORKING: RECEIVE & UPDATE ---
		networkManager.update(); // This receives the latest broadcast from the server.
		auto latestGameState = networkManager.getLatestGameState();

		if (latestGameState.has_value())
		{
			GameState &gs = latestGameState.value();

			// This is the REPLICATION step.
			// We loop through the players in the GameState from the server.
			for (int i = 0; i < gs.num_clients; ++i)
			{
				PlayerState &serverPlayer = gs.players[i];

				// NETWORKING: First-time connection logic to find out our ID
				if (myClientId == -1)
				{
					// Let's assume the server adds players in order and the last
					// one in the list is the one we just requested. This is a
					// simple but brittle way to do it. A real handshake is better.
					myClientId = serverPlayer.clientId;
				}

				// If the player from the server is NOT us, update them.
				if (serverPlayer.clientId != myClientId)
				{
					if (remotePlayers.find(serverPlayer.clientId) == remotePlayers.end())
					{
						// If not, construct a new Entity for them directly inside the map.
						// The arguments inside emplace are passed directly to the Entity's constructor.
						remotePlayers.emplace(serverPlayer.clientId, Entity({serverPlayer.x, serverPlayer.y}, {71, 67}, playerTexture, false, {0, 0}));
					}
					else
					{
						// If we have seen them, just update their position.
						remotePlayers.at(serverPlayer.clientId).position = {serverPlayer.x, serverPlayer.y};
					}
				}
			}
		}

		// Rendering
		setupScreen(renderer);

		// Render the platform
		renderEntity(renderer, platform);

		// Create a leftmost bound for police car position
		int leftBound = policePos.x - 100;
		int rightBound = policePos.x + 100;

		if (police.position.x > rightBound)
			police.velocity = {-1, 0};
		if (police.position.x < leftBound)
			police.velocity = {1, 0};

		police.updatePosition();

		// Render the police car
		renderEntity(renderer, police);

		if (localPlayer.physicsApplied)
		{
			localPlayer.velocity.y += WorldPhysics::getGravity();
		}

		localPlayer.position.x += localPlayer.velocity.x;
		localPlayer.position.y += localPlayer.velocity.y;
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

        // Render the local player if toggled on
        if (showPlayer) {
            renderEntity(renderer, localPlayer);
        }

        // Render all the remote players
        for (auto const& [id, remote_player] : remotePlayers) {
            renderEntity(renderer, remote_player);
        }

		refreshScreen(renderer);
	}

	networkManager.cleanUp();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
