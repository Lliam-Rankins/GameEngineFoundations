/*
	This is a component file that outlines and implements rendering.
*/
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/collisions.h"

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

	Vector playerPos = {WINDOW_WIDTH / 2, platPos.y - 100};
	Vector playerDim = {71, 67};
	vel = {0, 0};

	Entity player(playerPos, playerDim, playerTexture, true, vel);

	Collider playerCollider(
		playerPos.x - playerDim.x / 2.0f,
		playerPos.y - playerDim.y / 2.0f,
		playerPos.x + playerDim.x / 2.0f,
		playerPos.y + playerDim.y / 2.0f);

	player.setCollider(&playerCollider);

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
			player.position = {WINDOW_WIDTH / 2.0f, platPos.y - 300.0f};
			player.velocity = {0, 0};
			SyncColliderToEntity(player);
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
			player.velocity = {-1, 0};
			player.updatePosition();
		}
		if (isKeyPressed(SDL_SCANCODE_D))
		{
			player.velocity = {1, 0};
			player.updatePosition();
		}

		if (isKeyPressed(SDL_SCANCODE_SPACE))
		{
			player.velocity = {0, -30};
			player.updatePosition();
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

		if (player.physicsApplied)
		{
			player.velocity.y += WorldPhysics::getGravity();
		}

		player.position.y += player.velocity.y;

		// Update collider to match new position
		player.collider->topLeft.y = player.position.y - player.dimensions.y / 2.0f;
		player.collider->bottomRight.y = player.position.y + player.dimensions.y / 2.0f;

		// Collision check
		if (overlappingColliders(*player.collider, *platform.collider))
		{
			if (player.velocity.y > 0)
			{
				player.position.y = platform.position.y - platform.dimensions.y / 2.0f - player.dimensions.y / 2.0f;
				player.collider->topLeft.y = player.position.y - player.dimensions.y / 2.0f;
				player.collider->bottomRight.y = player.position.y + player.dimensions.y / 2.0f;
				player.velocity.y = 0;
			}
		}

		if (overlappingColliders(*player.collider, *police.collider))
		{
			player.position.x = playerPos.x;
			player.position.y = playerPos.y;
		}

		if( showPlayer )
			renderEntity(renderer, player);

		refreshScreen(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
