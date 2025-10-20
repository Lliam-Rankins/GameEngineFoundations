/*
	This is a file that runs the main loop.
	Some of the content in this file was generated with Gemini 2.5 Pro.
	This citation is to abide by the syllabus requirement that "appropriate citations"
	must be given when referring to external sources. More information is available upon request.
*/
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/collisions.h"

// Initialize the window and renderer using SDL method
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

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

// Starting Positions
Vector playerPos = {100, 100};
Vector platformPos_1 = {100, 400};
Vector movingPlatPos_1 = {500, 600};

// Defaults
Vector defaultVel = {0, 0};


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

int gravity = 2;


//////////////////////////////////////////////////
//
// Helper Functions
//
//////////////////////////////////////////////////

	SDL_Texture *platformTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/ENVIRONMENT/props/control-box-3.png");
	if (!platformTexture)
		textureError();

	SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/player/idle/idle-1.png");
	if (!playerTexture)
		textureError();

	SDL_Texture *policeTexture = IMG_LoadTexture(renderer, "media/warped city files/Assets/SPRITES/vehicles/v-police.png");
	if (!policeTexture)
		textureError();

//////////////////////////////////////////////////
//
// Main Function
//
//////////////////////////////////////////////////
int main(int argc, char* argv[])
{


	// Request port 5555 and subscribe port 5556
	const int HANDSHAKE_PORT = 5557;
	const int SUBSCRIBE_PORT = 5556;
	const int BASE_REPLY_PORT = 6000;

	int myClientId = networkManager.connectAndHandshake("localhost", HANDSHAKE_PORT);


	//Load Textures
	SDL_Texture* playerTex = IMG_LoadTexture(renderer, "../media/darkworld_character_morwen_idle.png");
	texCheck(playerTex);

	SDL_Texture* brickTex = IMG_LoadTexture(renderer, "media/brick.png");
	texCheck(brickTex);

	int myRequestPort = BASE_REPLY_PORT + myClientId;
	std::cout << "This client will send updates to port: " << myRequestPort << std::endl;

	// Start the main client sockets, connecting to our unique request port
	networkManager.startClient("localhost", myRequestPort, SUBSCRIBE_PORT);

	// Set everything for the platform
	Vector platPos = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
	Vector platDim = {62, 30};
	Vector vel = {0, 0};

	// Entity creation
	Entity player(playerPos, {playerTex->w, playerTex->h}, playerTex, true, defaultVel);

	Entity platform_1(platformPos_1, {brickTex->w, brickTex->h}, brickTex, false, defaultVel);
	Entity movingPlat_1(movingPlatPos_1, {brickTex->w, brickTex->h}, brickTex, false, {movingPlatSpeed, 0}); //Moves to the Right


	//Creating and Setting Colliders
	Collider playerCol(0, 0, 0, 0);
	player.setCollider(&playerCol);

	Collider platformCol_1(0, 0, 0, 0);
	platform_1.setCollider(&platformCol_1);

	Collider movingPlatCol_1(0, 0, 0, 0);
	movingPlat_1.setCollider(&movingPlatCol_1);


	// Setting Gravity
	WorldPhysics::setGravity(gravity);



	//////////////////////////////////////////////////
	//
	// Main Gameplay Loop
	//
	//////////////////////////////////////////////////

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

			// Otherwise look for a key press
			else if (event.type == SDL_EVENT_KEY_DOWN) {
				// Testing to allow the keypress of "ESC" to exit the window.
				if (isKeyPressed(SDL_SCANCODE_ESCAPE)) {								// Quit
					running = false;
				}

				// Change Scaling Mode
				if (isKeyPressed(SDL_SCANCODE_GRAVE)) {									// Change Scaling Mode
					constantSizeScale = !constantSizeScale;
				}
			}
		}

		//////////////////////////////////////////////////
		//
		// Gameplay Updates
		//
		//////////////////////////////////////////////////

		// Update moving platforms position
		if (movingPlat_1.position.x > movingPlatPos_1.x + 100) movingPlat_1.velocity.x = -movingPlatSpeed;
		if (movingPlat_1.position.x < movingPlatPos_1.x - 100) movingPlat_1.velocity.x = movingPlatSpeed;
		movingPlat_1.updatePosition();

		platform_1.updatePosition();


		//////////////////////////////////////////////////
		//
		// Player Movement
		//
		//////////////////////////////////////////////////
		// Player Movement
		if (isKeyPressed(SDL_SCANCODE_W) || isKeyPressed(SDL_SCANCODE_SPACE)) {	// Jump
				player.velocity.y = -playerJumpSpeed;
				player.updatePosition();
		}
		if (isKeyPressed(SDL_SCANCODE_A)) {										// Left
			player.velocity.x = -playerSpeed;
			player.updatePosition();
		}
		if (isKeyPressed(SDL_SCANCODE_S)) {										// Down
			player.velocity.y = playerSpeed;
			player.updatePosition();
		}
		if (isKeyPressed(SDL_SCANCODE_D)) {										// Right
			player.velocity.x = playerSpeed;
			player.updatePosition();
		}

		
		
		// Check if player is coliding with anything
		if (!overlappingColliders(*player.collider, *platform_1.collider) && !overlappingColliders(*player.collider, *movingPlat_1.collider)) {
			player.velocity.y += WorldPhysics::getGravity();
		}

		player.updatePosition();
		player.velocity = {0, 0};

		//////////////////////////////////////////////////
    //
		// Rendering
		//
		//////////////////////////////////////////////////

		// Setup the Screen
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
