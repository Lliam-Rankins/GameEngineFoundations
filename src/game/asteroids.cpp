
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/collisions.h"
#include "../engine/headers/input.h"
#include "../engine/headers/network.h"
#include "../engine/headers/protocol.h"
#include "../engine/headers/entities.h"
#include "../engine/headers/input.h"
#include "../engine/headers/timeline.h"
#include "../engine/headers/GameObject.h"
#include "../engine/headers/gameUtils.h"
#include "../engine/headers/EventManager.h"
#include "../engine/headers/recording.h"
#include "../engine/headers/GameObjectPool.h"
#include "../engine/headers/ship_bullet_pool.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <utility>


// Initialize the window and renderer using SDL method
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// Window Variables
Vector windowSize = {800, 800};


///////////////////////////
//	Defaults
///////////////////////////

// Player
Vector player_Position = {400, 400};
Vector player_dimensions;
SDL_Texture* player_texture;
Vector player_Velocity = {0, 0};
float playerAcceleration = .5;
float player_turnSpeed = 5;

// Orb
SDL_Texture* orb_texture;
Vector orb_dimensions;

// Asteroids
SDL_Texture* asteroid_texture_big;
SDL_Texture* asteroid_texture_med;
SDL_Texture* asteroid_texture_small;
int asteroid_speed = 20;

// Spawn Zones
Vector spawnZone1_Position = {100, 100};



bool isPaused = false;






//////////////////////////////////////////////////
//
// Helper Functions
//
//////////////////////////////////////////////////

// Input Enum
enum Direction {
    FORWARD,
    TURN_LEFT,
	BACK,
	TURN_RIGHT,
	SHOOT
};

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

bool collidable(GameObject *a) {
	if (a->hasComponent("position") && a->hasComponent("dimensions")) return true;
	return false;
}



void renderObj(GameObject *object, Vector offset) {
	// Only Render Objects with Positions and Dimensions, and a color or texture
	if (!object->hasComponent("position") || !object->hasComponent("dimensions")) {
		return;
	}

	Vector pos = object->getComponent<Vector>("position");
	Vector dim = object->getComponent<Vector>("dimensions");

	// Create Rectangle to render Obj
    SDL_FRect rectangle = { pos.x - offset.x, pos.y - offset.y, dim.x, dim.y};

	// Object has Texture
	if (object->hasComponent("texture")) {
		// AI USE DISCLOSURE, Gave ChatGPT a partially working render texture set of code, and asked
		// it to add rotational locking knowing that some objects have a orientation component
		SDL_Texture* tex = object->getComponent<SDL_Texture *>("texture");

		float orientDeg = 0.0f;
		SDL_FPoint pivot = {0, 0};

		// If rotating, compute pivot and angle
		if (object->hasComponent("orientation")) {
			float radians = object->getComponent<float>("orientation");
			orientDeg = radians * (180.0f / M_PI);
			pivot = { dim.x / 2.0f, dim.y / 2.0f };  // rotate around center
		}

		SDL_RenderTextureRotated(
			renderer,
			tex,
			NULL,            // src = whole texture (IMPORTANT)
			&rectangle,      // dst in world coordinates
			orientDeg,
			&pivot,          // center of rotation inside the texture
			SDL_FLIP_NONE
		);

		return;
		// AI USE END
	}
	// Object has Color
	else if (object->hasComponent("color")) {
		SDL_Color color = object->getComponent<SDL_Color>("color");
		SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
		SDL_RenderRect(renderer, &rectangle);
	}
	// Object has no color or texture
	else {
		SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
		SDL_RenderFillRect(renderer, &rectangle);
	}
}


void printList(std::vector<GameObject *> *objectList) {
	for (const auto& obj : *objectList) {
		if (obj->hasComponent("client_id")) {
			std::cout << "Client Id:" << obj->getComponent<int>("client_id") << std::endl;
		}
	}
}

Vector orientToVector(float orientation) {
	return Vector{cos(orientation), sin(orientation)};
}

void spawnBullet(Ship_Bullet_Pool* bulletPool, std::vector<GameObject *> *objectList, std::mutex *objectMutex, GameObject* player) {
	{
		// Lock
		std::lock_guard<std::mutex> lock(*objectMutex);

		GameObject* gameObject = bulletPool->spawn();
		gameObject->setComponent("texture", orb_texture);
		gameObject->setComponent("dimensions", orb_dimensions);
		
		objectList->push_back(gameObject);
	}
}

void spawnAsteroid(GameObjectPool* asteroidPool, std::vector<GameObject *> *objectList, std::mutex *objectMutex) {
	{
		// Lock
		std::lock_guard<std::mutex> lock(*objectMutex);

		// TODO, add logic to spawn asteroid on edge of screen going
		GameObject* gameObject = asteroidPool->spawn(Vector{400, 400}, Vector{0, 0});
		gameObject->setComponent("texture", asteroid_texture_small);
		gameObject->setComponent("dimensions", Vector {asteroid_texture_small->w, asteroid_texture_small->h});
		gameObject->setComponent("isAsteroid", true);
		gameObject->setComponent("position", Vector{rand() % (int)windowSize.x, rand() % (int)windowSize.y});
		gameObject->setComponent("velocity", Vector{rand() % asteroid_speed, rand() % asteroid_speed});

		objectList->push_back(gameObject);
	}
}



//////////////////////////////////////////////////
//
// Multithreading
//
/////////////////\/////////////////////////////////

// Rendering Thread
void rendering(std::vector<GameObject *> *objectList, std::mutex *objectMutex) {
	// Loop Forever
	while (true) {
        // Setup the Screen
        setupScreen(renderer);

        {
            // Lock, rendering remote objects
            std::lock_guard<std::mutex> lock(*objectMutex);

            // Render Local Objects
            for (const auto &object : *objectList) {
                renderObj(object, Vector{0, 0});
            }
		}

		// Wait
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
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
	player_texture = IMG_LoadTexture(renderer, "../media/space_ship.png");
	texCheck(player_texture);

	orb_texture = IMG_LoadTexture(renderer, "../media/darkworld_spawn_swirlingorb.png");
	texCheck(orb_texture);

	asteroid_texture_big = IMG_LoadTexture(renderer, "../media/Card_256.jpg");
	asteroid_texture_med = IMG_LoadTexture(renderer, "../media/Card_128.jpg");
	asteroid_texture_small = IMG_LoadTexture(renderer, "../media/Card_64.jpg");
	texCheck(asteroid_texture_big);
	texCheck(asteroid_texture_med);
	texCheck(asteroid_texture_small);



	// Create Dimensions
	player_dimensions = {player_texture->w, player_texture->h};
	orb_dimensions = {orb_texture->w, orb_texture->h};


    // Adjust Starting Pos
    player_Position = Vector{(windowSize.x / 2) - (player_dimensions.x / 2), (windowSize.y / 2) - (player_dimensions.y / 2)};
    spawnZone1_Position = player_Position;

	// Instantiate ObjectList
	std::vector<GameObject *> objects;
    std::mutex objectMutex;

	// Player
	GameObject player = GameObject();
	player.setComponent("position", player_Position);
	player.setComponent("starting_pos", player_Position);
	player.setComponent("orientation", 0.0f);
    player.setComponent("id", 0);
	player.setComponent("velocity", Vector{0, 0});
	player.setComponent("dimensions", player_dimensions);
	player.setComponent("texture", player_texture);
	player.setComponent("physics", true);

	// Spawn Zones
	GameObject player_spawner = GameObject();
	player_spawner.setComponent("position", spawnZone1_Position);
	player_spawner.setComponent("id", 1);

	objects.push_back(&player);
	objects.push_back(&player_spawner);

	// Time Line Setup
	Timeline timeline;


	////////////////////
	//`Pool Setup
	////////////////////
	Ship_Bullet_Pool *player_bullet_pool = new Ship_Bullet_Pool(sizeof(GameObject), 100, &player, &objects, &objectMutex);
	GameObjectPool *asteroid_pool = new GameObjectPool(sizeof(GameObject), 5, &player, Vector{100, 100});

	////////////////////
	//`Event Setup
	////////////////////
	std::vector<std::shared_ptr<Event>> eventList;
    std::mutex eventMutex;

	EventManager eventManager;

	// Input Event
	eventManager.RegisterListener(InputEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
		const auto &input = static_cast<const InputEvent &>(e);

		Vector playerVel = player.getComponent<Vector>("velocity");
		Vector playerPos = player.getComponent<Vector>("position");
		float playerRad = player.getComponent<float>("orientation");

		Vector playerOrient{ cos(playerRad - M_PI/2), sin(playerRad - M_PI/2) };

		// TODO: Account for orientation overflow
		switch (input.action) {
			case FORWARD:
				player.setComponent("velocity", vectorAdd(playerVel, Vector{playerOrient.x * playerAcceleration, playerOrient.y * playerAcceleration}));
				break; 

			case BACK:
				player.setComponent("velocity", vectorSub(playerVel, Vector{playerOrient.x * playerAcceleration, playerOrient.y * playerAcceleration}));
				break;

			case TURN_LEFT:
				player.setComponent("orientation", playerRad - player_turnSpeed * timeline.getDeltaTime());
				break;

			case TURN_RIGHT:
				player.setComponent("orientation", playerRad + player_turnSpeed * timeline.getDeltaTime());
				break;
				
			case SHOOT:
				spawnBullet(player_bullet_pool, &objects, &objectMutex, &player);
				break;
	}
	});


	///////////////////////
	//	Input
	///////////////////////
	InputManager input(eventManager, 0);
	
	// Binding Input


	input.bindKey(SDL_SCANCODE_W, 0, InputManager::InputType::Simple);
	input.bindKey(SDL_SCANCODE_A, 1, InputManager::InputType::SingleComplex);
	input.bindKey(SDL_SCANCODE_S, 2, InputManager::InputType::Simple);
	input.bindKey(SDL_SCANCODE_D, 3, InputManager::InputType::Simple);

	input.bindChord({SDL_SCANCODE_A, SDL_SCANCODE_LSHIFT}, 4);

	std::cout << "Setup" << std::endl;

	//////////////////////////////////////////////////
	//
	// Multi Threading
	//
	//////////////////////////////////////////////////

	// Start Rendering Thread
	std::thread renderThread(&rendering, &objects, &objectMutex);

	std::cout << "Started multi threading" << std::endl;



	//////////////////////////////////////////////////
	//
	// Main Gameplay Loop
	//
	//////////////////////////////////////////////////

	// Main game loop condition variable
	bool running = true;
	long time_since_spawn = 0;

	// SDL_Event to capture event of window being closed
	SDL_Event event;

	// The main game loop
	while (running) {
		// Time Line Update
		timeline.update();
		float d_time = timeline.getDeltaTime();

		// Check inputs
		input.update();
		eventManager.ProcessEvents(std::chrono::steady_clock::now().time_since_epoch().count());

		// Poll for events
		while (SDL_PollEvent(&event)) {

			// If event is Window Resize
			if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                //Get Window Size
                int w, h;
                SDL_GetWindowSize(window, &w, &h);
                // Resize as if the screen was still the same
                SDL_SetRenderLogicalPresentation(renderer, w, h, SDL_LOGICAL_PRESENTATION_STRETCH);
			}

			// Read input from input manager
			// If the event is close the window
			if (event.type == SDL_EVENT_QUIT)
				running = false;

			// // Otherwise look for a key press
			// else if (event.type == SDL_EVENT_KEY_DOWN) {
			// 	// Testing to allow the keypress of "ESC" to exit the window.
			// 	if (i) {								// Quit
			// 		running = false;
			// 	}
			// }
		}

		//////////////////////////////////////////////////
		//
		// Gameplay Updates
		//
		//////////////////////////////////////////////////		
		if (asteroid_pool->getActiveCount() < asteroid_pool->getCapacity() && timeline.getElapsedTicks() - time_since_spawn > 20) {

			spawnAsteroid(asteroid_pool, &objects, &objectMutex);
			time_since_spawn = timeline.getElapsedTicks();
		}
		
		asteroid_pool->update(d_time);
		player_bullet_pool->update(d_time);


		//////////////////////////////////////////////////
		//
		// Player Movement
		//
		//////////////////////////////////////////////////
		// Restrain Player to window
		{
			std::lock_guard<std::mutex> lock(objectMutex);

			Vector pos = player.getComponent<Vector>("position");

			if (pos.x > windowSize.x) pos.x = 0;
			if (pos.y > windowSize.y) pos.y = 0;

			if (pos.x < 0) pos.x = windowSize.x;
			if (pos.y < 0) pos.y = windowSize.y;

			player.setComponent("position", pos);
		}


		//////////////////////////////////////////////////
		//
		// Collisions
		//
		//////////////////////////////////////////////////
		
		// Check if any asteroid is interacting with any bullet
		for (auto object1 : objects) {
			if (object1->hasComponent("isAsteroid")) {

				{
					std::lock_guard<std::mutex> lock(objectMutex);
					
					Vector pos = object1->getComponent<Vector>("position");

					if (pos.x > windowSize.x) pos.x = 0;
					if (pos.y > windowSize.y) pos.y = 0;

					if (pos.x < 0) pos.x = windowSize.x;
					if (pos.y < 0) pos.y = windowSize.y;

					object1->setComponent("position", pos);
				}

				// Object 1 is bullet
				for (auto object2 : objects) {
					if (object2->hasComponent("isBullet")) {
						// Object 2 is asteroid, check if overlapping
						if (overlappingColliders1(*object1, *object2)) {
							// change position and velocity randomly
							{
								std::lock_guard<std::mutex> lock(objectMutex);

								object1->setComponent("position", Vector{rand() % (int)windowSize.x, rand() % (int)windowSize.y});
								object1->setComponent("velocity", Vector{rand() % asteroid_speed, rand() % asteroid_speed});

								
							}
							
						}
					}
				}
			}
		}
		
				

		// Move player and then reset their velocity
		{
			std::lock_guard<std::mutex> lock(objectMutex);

			Vector pos = player.getComponent<Vector>("position");
			Vector vel = player.getComponent<Vector>("velocity");

			player.setComponent("position", Vector{pos.x + (vel.x * timeline.getDeltaTime()), pos.y + (vel.y * timeline.getDeltaTime())});
		}




		//////////////////////////////////////////////////
		//
		// Rendering
		//
		//////////////////////////////////////////////////
		refreshScreen(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
