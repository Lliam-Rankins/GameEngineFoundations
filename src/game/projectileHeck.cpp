
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
Vector player_Position = {800, 800};
Vector player_dimensions;
SDL_Texture* player_texture;
Vector player_Velocity = {0, 0};
int player_tp_distance = 200;
float playerSpeed = 300.0;

// Door
Vector door_position = {700, 350};
SDL_Texture* door_texture;
Vector door_dimensions;

// Orb
SDL_Texture* orb_texture;
Vector orb_dimensions;

// Spawn Zones
Vector spawnZone1_Position = {100, 100};
Vector spawnZone2_Position = {700, 100};



bool isPaused = false;






//////////////////////////////////////////////////
//
// Helper Functions
//
//////////////////////////////////////////////////

// Input Enum
enum Direction {
    UP,
    LEFT,
	DOWN,
	RIGHT,
	LEFT_TP
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
		SDL_Texture* tex = object->getComponent<SDL_Texture *>("texture");
		SDL_RenderTexture(renderer, tex, NULL, &rectangle);
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

void spawnBullet(GameObjectPool* objectPool, std::vector<GameObject *> *objectList, std::mutex *objectMutex) {
	{
		// Lock
		std::lock_guard<std::mutex> lock(*objectMutex);

		Vector vel = Vector {-(float)rand() / RAND_MAX, 2 * ((float)rand() / RAND_MAX) - 1};
		vel.x = vel.x * 300;
		vel.y = vel.y * 300;
		
		GameObject * gameObject = objectPool->spawn(Vector{door_position.x + door_dimensions.x / 2, door_position.y + door_dimensions.y / 2}, vel);
		gameObject->setComponent("texture", orb_texture);
		gameObject->setComponent("dimensions", Vector{orb_dimensions});

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
	player_texture = IMG_LoadTexture(renderer, "../media/darkworld_enemy_skullduggery_idle.png");
	texCheck(player_texture);

    door_texture = IMG_LoadTexture(renderer, "../media/darkworld_spawn_doorwaytolimbo.png");
	texCheck(door_texture);

	orb_texture = IMG_LoadTexture(renderer, "../media/darkworld_spawn_swirlingorb.png");
	texCheck(orb_texture);



	// Create Dimensions
	player_dimensions = {player_texture->w, player_texture->h};
    door_dimensions = {door_texture->w, door_texture->h};
	orb_dimensions = {orb_texture->w, orb_texture->h};


    // Adjust Starting Pos
    door_position = Vector{windowSize.x - door_dimensions.x, (windowSize.y / 2) - (door_dimensions.y / 2)};
    player_Position = Vector{0, (windowSize.y / 2) - (player_dimensions.y / 2)};
    spawnZone1_Position = player_Position;

	// Instantiate ObjectList
	std::vector<GameObject *> objects;
    std::mutex objectMutex;

	// Player
	GameObject player = GameObject();
	player.setComponent("position", player_Position);
	player.setComponent("starting_pos", player_Position);
    player.setComponent("id", 0);
	player.setComponent("velocity", Vector{0, 0});
	player.setComponent("dimensions", player_dimensions);
	player.setComponent("texture", player_texture);
	player.setComponent("physics", true);

	// Platforms
	GameObject bullet_spawner = GameObject();
	bullet_spawner.setComponent("position", door_position);
	bullet_spawner.setComponent("id", -1);
	bullet_spawner.setComponent("dimensions", door_dimensions);
	bullet_spawner.setComponent("texture", door_texture);

	// Spawn Zones
	GameObject player_spawner = GameObject();
	player_spawner.setComponent("position", spawnZone1_Position);
	player_spawner.setComponent("id", 1);

	objects.push_back(&player);
	objects.push_back(&bullet_spawner);
	objects.push_back(&player_spawner);

	// Time Line Setup
	Timeline timeline;


	////////////////////
	//`Pool Setup
	////////////////////
	GameObjectPool *objectPool = new GameObjectPool(sizeof(GameObject), 100, &player, door_position);

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

		switch (input.action) {
			case UP:
				player.setComponent("velocity", Vector{playerVel.x, -playerSpeed * timeline.getDeltaTime()});
				break; 

			case DOWN:
				player.setComponent("velocity", Vector{playerVel.x, playerSpeed * timeline.getDeltaTime()});
				break;

			case LEFT:
				player.setComponent("velocity", Vector{-playerSpeed * timeline.getDeltaTime(), playerVel.y});
				break;

			case RIGHT:
				player.setComponent("velocity", Vector{playerSpeed * timeline.getDeltaTime(), playerVel.y});
				break;
				
			case LEFT_TP:
				player.setComponent("position", vectorAdd(playerPos, Vector{-player_tp_distance, 0}));
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
		if (objectPool->getActiveCount() < objectPool->getCapacity() && timeline.getElapsedTicks() - time_since_spawn > 5) {

			spawnBullet(objectPool, &objects, &objectMutex);
			time_since_spawn = timeline.getElapsedTicks();
		}
		
		objectPool->update(d_time);


		//////////////////////////////////////////////////
		//
		// Player Movement
		//
		//////////////////////////////////////////////////
		// Player Movement
		// Vector vel = player.getComponent<Vector>("velocity");
		// if (isKeyPressed(SDL_SCANCODE_W)) {	// Jump
        //     // Move Up
        //     vel = vectorAdd(vel, Vector{0, -playerSpeed * d_time});
		// }
		// if (isKeyPressed(SDL_SCANCODE_A)) {										// Left
		// 	// Move Left
        //     vel = vectorAdd(vel, Vector{-playerSpeed * d_time, 0});
		// }
		// if (isKeyPressed(SDL_SCANCODE_S)) {										// Down
		// 	// Move Down
        //     vel = vectorAdd(vel, Vector{0, playerSpeed * d_time});
		// }
		// if (isKeyPressed(SDL_SCANCODE_D)) {										// Right
		// 	// Move Right
        //     vel = vectorAdd(vel, Vector{playerSpeed * d_time, 0});
		// }

        // player.setComponent("velocity", vel);


		//////////////////////////////////////////////////
		//
		// Collisions
		//
		//////////////////////////////////////////////////
		if (overlappingColliders1(player, bullet_spawner)) {
			running = false;
		}
		
				

		// Move player and then reset their velocity
		updatePosition(player, isPaused);
		player.setComponent("velocity", Vector{0, 0});




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
