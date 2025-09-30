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

// Window Variables
Vector windowSize = {1440, 1080};

// Starting Positions
Vector playerPos = {100, 100};
Vector platformPos_1 = {100, 400};
Vector movingPlatPos_1 = {500, 600};

// Defaults
Vector defaultVel = {0, 0};


// Game Vars
float playerSpeed = 1.0;
float playerJumpSpeed = 2.5;
float movingPlatSpeed = .5;

int gravity = 2;


//////////////////////////////////////////////////
//
// Helper Functions
//
//////////////////////////////////////////////////

// Texture valid check
void texCheck(SDL_Texture* tex) {
	if (!tex) {
		SDL_Log("Could not load image: %s", SDL_GetError());
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
		exit(1);
	}
}

// Render entity
void renderEntity(const Entity& e) {
    SDL_FRect rectangle = { e.position.x - e.dimensions.x, e.position.y - e.dimensions.y, e.dimensions.x, e.dimensions.y};
    SDL_RenderTexture(renderer, e.texture, NULL, &rectangle);
}


//////////////////////////////////////////////////
//
// Main Function
//
//////////////////////////////////////////////////
int main(int argc, char* argv[])
{

	//////////////////////////////////////////////////
	//
	// Setup
	//
	//////////////////////////////////////////////////

	// Initialize the SDL library
	initializeSDL();
	createWindowAndRenderer(&window, &renderer);
	SDL_SetWindowSize(window, windowSize.x, windowSize.y);


	//Load Textures
	SDL_Texture* playerTex = IMG_LoadTexture(renderer, "../media/darkworld_character_morwen_idle.png");
	texCheck(playerTex);

	SDL_Texture* brickTex = IMG_LoadTexture(renderer, "media/brick.png");
	texCheck(brickTex);


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

	std::thread networkThread(network_thread_loop, &running, &networkManager, &localPlayer, &myClientId);

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

		renderEntity(player);
		renderEntity(platform_1);
		renderEntity(movingPlat_1);

		if (police.position.x > rightBound)
			police.setVelocity({-150.0f, 0});
		if (police.position.x < leftBound)
			police.setVelocity({150.0f, 0});

		police.setPosition({police.getPosition().x + police.getVelocity().x * dt, police.getPosition().y});
		SyncColliderToEntity(police);

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
		policeRenderer.render(renderer, police.position, police.dimensions);

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
