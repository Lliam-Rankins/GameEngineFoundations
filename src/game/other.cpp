
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
Vector windowSize = {720, 540};


///////////////////////////
//	Defaults
///////////////////////////

// Player
Vector player_Position = {100, 100};
Vector player_Dimensions;
SDL_Texture* player_Texture;
Vector player_Velocity = {0, 0};
SDL_Texture* playerTex;

// Platforms
Vector platform1_Position = {100, 400};
Vector platform2_Position = {400, 400};
Vector platform3_Position = {700, 400};

SDL_Texture* orb_texture;
Vector orb_dimensions;

// Spawn Zones
Vector spawnZone1_Position = {100, 100};
Vector spawnZone2_Position = {700, 100};

// DeathZones
Vector deathZone1_Position = {100, 900};
Vector deathZone1_Dimensions = {700, 100};

// Game Vars
float playerSpeed = 300.0;
float playerJumpSpeed = 300.0;

int gravity = 200;

bool isPaused = false;






//////////////////////////////////////////////////
//
// Helper Functions
//
//////////////////////////////////////////////////

// Input Enum
enum Direction {
    UP,
    DOWN,
	LEFT,
	RIGHT,
	FAR_LEFT
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



//////////////////////////////////////////////////
//
// Multithreading
//
/////////////////\/////////////////////////////////

// Rendering Thread
void rendering(std::vector<GameObject *> *objectList, std::mutex *objectMutex, GameObject *player, std::vector<GameObject *> *localObjectList) {
	// Loop Forever
	while (true) {
        // Setup the Screen
        setupScreen(renderer);

        {
            // Lock, rendering remote objects
            std::lock_guard<std::mutex> lock(*objectMutex);

            // Get Player Position for offsetting others
            Vector offset = player->getComponent<Vector>("position");


            // Render Local Objects
            for (const auto &object : *localObjectList) {
                renderObj(object, offset);
            }

            // Render Each Object
            for (const auto &object : *objectList) {
                if (object->hasComponent("client_id") && object->getComponent<int>("client_id") == player->getComponent<int>("client_id")) continue;
                renderObj(object, offset);
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
	player_Texture = IMG_LoadTexture(renderer, "../media/darkworld_character_morwen_idle.png");
	texCheck(player_Texture);

	orb_texture = IMG_LoadTexture(renderer, "../media/darkworld_platform_brick_idle.png");
	texCheck(orb_texture);



	// Create Dimensions
	player_Dimensions = {player_Texture->w, player_Texture->h};
	orb_dimensions = {orb_texture->w, orb_texture->h};


	// Instantiate ObjectList
	std::vector<std::vector<GameObject *> *> masterObjectList;
    std::vector<GameObject *> objectList;
	std::vector<GameObject *> objects;
    std::mutex objectMutex;

	masterObjectList.push_back(&objectList);
	masterObjectList.push_back(&objects);

	// Player
	GameObject player;
	player.setComponent("position", player_Position);
    player.setComponent("id", 0);
	player.setComponent("velocity", Vector{0, 0});
	player.setComponent("dimensions", player_Dimensions);
	player.setComponent("texture", player_Texture);
	player.setComponent("physics", true);

	// Platforms
	GameObject bullet_spawner = GameObject();
	bullet_spawner.setComponent("position", platform1_Position);
	bullet_spawner.setComponent("id", -1);
	bullet_spawner.setComponent("dimensions", orb_dimensions);
	bullet_spawner.setComponent("texture", orb_texture);

	// Spawn Zones
	GameObject player_spawner = GameObject();
	player_spawner.setComponent("position", spawnZone1_Position);
	player_spawner.setComponent("id", 1);

	objects.push_back(&player);
	objects.push_back(&bullet_spawner);
	objects.push_back(&player_spawner);
;

	// Setting Gravity
	WorldPhysics::setGravity(gravity);

	// Time Line Setup
	Timeline timeline;


	////////////////////
	//`Pool Setup
	////////////////////
	GameObjectPool *objectPool = new GameObjectPool(sizeof(GameObject), 5);
	GameObject *go = objectPool->spawn();
	go->setComponent("is_npc", true);
	go->setComponent("position", Vector{700, 600});

	objects.push_back(go);

	std::cout << "Setup" << std::endl;

	//////////////////////////////////////////////////
	//
	// Multi Threading
	//
	//////////////////////////////////////////////////

	// Start Rendering Thread
	std::thread renderThread(&rendering, &objectList, &objectMutex, &player, &objects);

	std::cout << "Started multi threading" << std::endl;



	//////////////////////////////////////////////////
	//
	// Main Gameplay Loop
	//
	//////////////////////////////////////////////////

	// Main game loop condition variable
	bool running = true;

	// SDL_Event to capture event of window being closed
	SDL_Event event;

	// Scaling Type bool
	bool constantSizeScale = true;

	bool justPressed1 = false;
	bool justPressed3 = false;

	// The main game loop
	while (running) {
		// Time Line Update
		timeline.update();
		float d_time = timeline.getDeltaTime();

		// Poll for events
		while (SDL_PollEvent(&event)) {

			// If event is Window Resize
			if (event.type == SDL_EVENT_WINDOW_RESIZED) {
				// Constant Scaling
				if (constantSizeScale) {
					// Resize as if the screen was still the same
					SDL_SetRenderLogicalPresentation(renderer, 1080, 1080, SDL_LOGICAL_PRESENTATION_STRETCH);
				}
				// Proportional Scaling
				else {
					//Get Window Size
					int w, h;
					SDL_GetWindowSize(window, &w, &h);
					// Resize as if the screen was still the same
					SDL_SetRenderLogicalPresentation(renderer, w, h, SDL_LOGICAL_PRESENTATION_STRETCH);
				}	
			}

			// Read input from input manager
			// If the event is close the window
			if (event.type == SDL_EVENT_QUIT)
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





		//////////////////////////////////////////////////
		//
		// Player Movement
		//
		//////////////////////////////////////////////////
		// Player Movement
		Vector vel = player.getComponent<Vector>("velocity");
		if (isKeyPressed(SDL_SCANCODE_W) || isKeyPressed(SDL_SCANCODE_SPACE)) {	// Jump
			// Move Up
		}
		if (isKeyPressed(SDL_SCANCODE_A)) {										// Left
			// Move Left
		}
		if (isKeyPressed(SDL_SCANCODE_S)) {										// Down
			// Move Down
		}
		if (isKeyPressed(SDL_SCANCODE_D)) {										// Right
			// Move Right
		}

		// Add Gravity
		Vector playerSpeed = player.getComponent<Vector>("velocity");
		player.setComponent("velocity", Vector {playerSpeed.x, playerSpeed.y + WorldPhysics::getGravity() * d_time});

		//////////////////////////////////////////////////
		//
		// Collisions
		//
		//////////////////////////////////////////////////
		
		
				

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
