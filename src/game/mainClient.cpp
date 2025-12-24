#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/network.h"
#include "../engine/headers/protocol.h"
#include "../engine/headers/timeline.h"
#include "../engine/headers/entities.h"
#include "../engine/headers/input.h"
#include "../engine/headers/jobSystem.h"
#include "../engine/headers/GameObject.h"
#include "../engine/headers/gameUtils.h"
#include <thread>
#include <iostream>
#include "../engine/headers/EventManager.h"
#include "../engine/headers/input.h"
#include "../engine/headers/GameObjectPool.h"
#include "../engine/headers/CustomAllocator.h"


SDL_Renderer* renderer = nullptr;
SDL_Window* window = nullptr;


/**
 * @brief NEW: Configures all input bindings.
 */
void setupInputBindings(InputManager& input) {
    // BIND CHORDS
    // Ultrashot
    input.bindChord({SDL_SCANCODE_LSHIFT, SDL_SCANCODE_SPACE}, 0);

    // BIND KEYS
    // Move left
    input.bindKey(SDL_SCANCODE_A, 1, InputManager::InputType::Simple);
    // Move right
    input.bindKey(SDL_SCANCODE_D, 2, InputManager::InputType::Simple);
    // Shoot
    input.bindKey(SDL_SCANCODE_SPACE, 3, InputManager::InputType::SingleComplex);
}

/**
  This is the main loop for the whole game. The game is played as the following:
  Input/Movement- A moves left, D moves right, SPACE fires a bullet with a small cooldown, 
    SPACE + LSHIFT fires an Ultrashot/large bullet with a longer cooldown
  Objective- Shoot and kill all aliens to win, avoid getting hit by alien bullets or else
    you will "die" (have your position reset)
*/
int main(int, char**) {
    // Initialize SDL and rendering objects
    initializeSDL();
    createWindowAndRenderer(&window, &renderer);

    // Create an object list for use
    std::vector<GameObject *> clientObjectList;

    // Import various textures (all made by myself, hence no other citations)
    SDL_Texture* playerTexture = IMG_LoadTexture(renderer, "media/ship.png");
    SDL_Texture* alienTexture = IMG_LoadTexture(renderer, "media/alien.png");
    SDL_Texture* bulletTexture = IMG_LoadTexture(renderer, "media/bullet.png");

    // -----GAME OBJECT INITIALIZATIONS-----
    //
    //
    // Create GameObjects representing left and right screen boundaries
    GameObject leftBoundary;
    leftBoundary.setComponent("position", Vector{-100, 0});
    leftBoundary.setComponent("dimensions", Vector{100, 1080});

    GameObject rightBoundary;
    rightBoundary.setComponent("position", Vector{1920, 0});
    rightBoundary.setComponent("dimensions", Vector{100, 1080});

    // Create the player's GameObject
    GameObject *player = new GameObject();
    player->setComponent("id", -1);
    player->setComponent("position", Vector{WINDOW_WIDTH / 2, 860});
    player->setComponent("dimensions", Vector {64, 64});
    player->setComponent("texture", playerTexture);
    clientObjectList.push_back(player);

    // Create the 12 alien GameObjects
    //
    // Alien top row (row 1)
    GameObject *alien1 = new GameObject();
    alien1->setComponent("id", -2);
    alien1->setComponent("isAlien", true);
    alien1->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 1 + 100, 200});
    alien1->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 1 + 100, 200});
    alien1->setComponent("dimensions", Vector {64, 64});
    alien1->setComponent("velocity", (float) 10);
    alien1->setComponent("offset", (float) 0.0);
    alien1->setComponent("texture", alienTexture);
    alien1->setComponent("hit", false);
    clientObjectList.push_back(alien1);

    GameObject *alien2 = new GameObject();
    alien2->setComponent("id", -3);
    alien2->setComponent("isAlien", true);
    alien2->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 2 + 100, 200});
    alien2->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 2 + 100, 200});
    alien2->setComponent("dimensions", Vector {64, 64});
    alien2->setComponent("velocity", (float) 10);
    alien2->setComponent("offset", (float) 0.0);
    alien2->setComponent("texture", alienTexture);
    alien2->setComponent("hit", false);
    clientObjectList.push_back(alien2);

    GameObject *alien3 = new GameObject();
    alien3->setComponent("id", -4);
    alien3->setComponent("isAlien", true);
    alien3->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 3 + 100, 200});
    alien3->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 3 + 100, 200});
    alien3->setComponent("dimensions", Vector {64, 64});
    alien3->setComponent("velocity", (float) 10);
    alien3->setComponent("offset", (float) 0.0);
    alien3->setComponent("texture", alienTexture);
    alien3->setComponent("hit", false);
    clientObjectList.push_back(alien3);

    GameObject *alien4 = new GameObject();
    alien4->setComponent("id", -5);
    alien4->setComponent("isAlien", true);
    alien4->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 4 + 100, 200});
    alien4->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 4 + 100, 200});
    alien4->setComponent("dimensions", Vector {64, 64});
    alien4->setComponent("velocity", (float) 10);
    alien4->setComponent("offset", (float) 0.0);
    alien4->setComponent("texture", alienTexture);
    alien4->setComponent("hit", false);
    clientObjectList.push_back(alien4);

    GameObject *alien5 = new GameObject();
    alien5->setComponent("id", -6);
    alien5->setComponent("isAlien", true);
    alien5->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 5 + 100, 200});
    alien5->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 5 + 100, 200});
    alien5->setComponent("dimensions", Vector {64, 64});
    alien5->setComponent("velocity", (float) 10);
    alien5->setComponent("offset", (float) 0.0);
    alien5->setComponent("texture", alienTexture);
    alien5->setComponent("hit", false);
    clientObjectList.push_back(alien5);

    GameObject *alien6 = new GameObject();
    alien6->setComponent("id", -7);
    alien6->setComponent("isAlien", true);
    alien6->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 6 + 100, 200});
    alien6->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 6 + 100, 200});
    alien6->setComponent("dimensions", Vector {64, 64});
    alien6->setComponent("velocity", (float) 10);
    alien6->setComponent("offset", (float) 0.0);
    alien6->setComponent("texture", alienTexture);
    alien6->setComponent("hit", false);
    clientObjectList.push_back(alien6);

    // Alien bottom row (row 2)
    GameObject *alien7 = new GameObject();
    alien7->setComponent("id", -8);
    alien7->setComponent("isAlien", true);
    alien7->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 1, 400});
    alien7->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 1, 400});
    alien7->setComponent("dimensions", Vector {64, 64});
    alien7->setComponent("velocity", (float) 10);
    alien7->setComponent("offset", (float) 0.0);
    alien7->setComponent("texture", alienTexture);
    alien7->setComponent("hit", false);
    clientObjectList.push_back(alien7);

    GameObject *alien8 = new GameObject();
    alien8->setComponent("id", -9);
    alien8->setComponent("isAlien", true);
    alien8->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 2, 400});
    alien8->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 2, 400});
    alien8->setComponent("dimensions", Vector {64, 64});
    alien8->setComponent("velocity", (float) 10);
    alien8->setComponent("offset", (float) 0.0);
    alien8->setComponent("texture", alienTexture);
    alien8->setComponent("hit", false);
    clientObjectList.push_back(alien8);

    GameObject *alien9 = new GameObject();
    alien9->setComponent("id", -10);
    alien9->setComponent("isAlien", true);
    alien9->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 3, 400});
    alien9->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 3, 400});
    alien9->setComponent("dimensions", Vector {64, 64});
    alien9->setComponent("velocity", (float) 10);
    alien9->setComponent("offset", (float) 0.0);
    alien9->setComponent("texture", alienTexture);
    alien9->setComponent("hit", false);
    clientObjectList.push_back(alien9);

    GameObject *alien10 = new GameObject();
    alien10->setComponent("id", -11);
    alien10->setComponent("isAlien", true);
    alien10->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 4, 400});
    alien10->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 4, 400});
    alien10->setComponent("dimensions", Vector {64, 64});
    alien10->setComponent("velocity", (float) 10);
    alien10->setComponent("offset", (float) 0.0);
    alien10->setComponent("texture", alienTexture);
    alien10->setComponent("hit", false);
    clientObjectList.push_back(alien10);

    GameObject *alien11 = new GameObject();
    alien11->setComponent("id", -12);
    alien11->setComponent("isAlien", true);
    alien11->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 5, 400});
    alien11->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 5, 400});
    alien11->setComponent("dimensions", Vector {64, 64});
    alien11->setComponent("velocity", (float) 10);
    alien11->setComponent("offset", (float) 0.0);
    alien11->setComponent("texture", alienTexture);
    alien11->setComponent("hit", false);
    clientObjectList.push_back(alien11);

    GameObject *alien12 = new GameObject();
    alien12->setComponent("id", -13);
    alien12->setComponent("isAlien", true);
    alien12->setComponent("spawnLoc", Vector{(WINDOW_WIDTH / 7) * 6, 400});
    alien12->setComponent("position", Vector{(WINDOW_WIDTH / 7) * 6, 400});
    alien12->setComponent("dimensions", Vector {64, 64});
    alien12->setComponent("velocity", (float) 10);
    alien12->setComponent("offset", (float) 0.0);
    alien12->setComponent("texture", alienTexture);
    alien12->setComponent("hit", false);
    clientObjectList.push_back(alien12);

    // Array used to keep track of alive aliens
    int alienLives[12] = {0};

    // Create both event and input managers as well as the timeline for primary game functionalities
    EventManager eventManager;   
    InputManager input(eventManager);
    Timeline timeline(5);
    timeline.update();
    float dt = 0;

    // Initialize a GameObjectPull for bullet allocation and floats for time markers to prohibit or enable certain game loop actions
    GameObjectPool *objectPool = new GameObjectPool(sizeof(GameObject), 250);
    float timeSinceLastBullet = 0;
    float timeSinceLastUltra = 0;
    float timeSinceLastAlienShot = 0;

    // Register input event listener, setup input
    eventManager.RegisterListener(InputEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
        const auto &input = static_cast<const InputEvent  &>(e);

        if(input.action == 0 && timeSinceLastUltra > 1) {
            std::cout << "Chord Event: Ultrashot (LSHIFT + SPACE)\n";
            timeSinceLastUltra = 0;
            GameObject* bullet = objectPool->spawn();
            bullet->setComponent("texture", bulletTexture);
            bullet->setComponent("dimensions", Vector{64, 64});
            bullet->setComponent("position", Vector(player->getComponent<Vector>("position").x + player->getComponent<Vector>("dimensions").x / 2, player->getComponent<Vector>("position").y));
            bullet->setComponent("bullet", true);
            bullet->setComponent("active", true);
            bullet->setComponent("playerBullet", true);
            bullet->setComponent("velocity", Vector{0, -500});
            clientObjectList.push_back(bullet);
        } else if(input.action == 1 && !overlappingColliders1(*player, leftBoundary)) {
            player->setComponent("position", Vector{player->getComponent<Vector>("position").x - 1000 * dt, player->getComponent<Vector>("position").y});
        } else if(input.action == 2 && !overlappingColliders1(*player, rightBoundary)) {
            player->setComponent("position", Vector{player->getComponent<Vector>("position").x + 1000 * dt, player->getComponent<Vector>("position").y});
        } else if(input.action == 3 && timeSinceLastBullet > 0.3) {
            std::cout << "Single Key Event: Shoot laser\n";
            timeSinceLastBullet = 0;
            GameObject* bullet = objectPool->spawn();
            bullet->setComponent("texture", bulletTexture);
            bullet->setComponent("dimensions", Vector{16, 16});
            bullet->setComponent("position", Vector(player->getComponent<Vector>("position").x + player->getComponent<Vector>("dimensions").x / 2, player->getComponent<Vector>("position").y));
            bullet->setComponent("bullet", true);
            bullet->setComponent("active", true);
            bullet->setComponent("playerBullet", true);
            bullet->setComponent("velocity", Vector{0, -750});
            clientObjectList.push_back(bullet);
        } 
    });
    setupInputBindings(input); 

    // Register death/spawn events
    eventManager.RegisterListener(DeathEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
        const auto &death = static_cast<const DeathEvent &>(e);
        Vector spawnPos = {WINDOW_WIDTH / 2, 860};
        
        auto spawnEvent = std::make_shared<SpawnEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), death.entityID, spawnPos.x, spawnPos.y);
        eventManager.QueueEvent(spawnEvent);
    });

    eventManager.RegisterListener(SpawnEvent::STATIC_EVENT_TYPE_ID, [&](const Event &e) {
        const auto &spawn = static_cast<const SpawnEvent &>(e);
        player->setComponent("position", Vector{spawn.x, spawn.y});
    });
    

    bool running = true;
    SDL_Event e;
    // main loop
    while (running) {

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;
        }
        
        // Update timeline, update vars, process events, setup screen 
        timeline.update();
        dt = timeline.getDeltaTime();
        if (dt > 0.05f) dt = 0.05f;
        timeSinceLastBullet += dt;
        timeSinceLastUltra += dt;
        timeSinceLastAlienShot += dt;
        input.update();
        eventManager.ProcessEvents(std::chrono::steady_clock::now().time_since_epoch().count());
        setupScreen(renderer);

        // Despawn any out-of-bounds bullets
        for(int i = 0; i < clientObjectList.size(); i++) {
            GameObject* obj = clientObjectList[i];

            if(obj->hasComponent("bullet") && obj->getComponent<bool>("active") == true) {
                Vector bulletPos = obj->getComponent<Vector>("position");

                if(bulletPos.y < 0 || bulletPos.y > WINDOW_HEIGHT) {
                    clientObjectList.erase(clientObjectList.begin() + i);
                    i--;
                    objectPool->despawn(obj);
                    continue;
                }
            }
        }
        
        bool shot = false;
        // For each object in the list
        for (size_t i = 0; i < clientObjectList.size(); i++) {
            GameObject* obj = clientObjectList[i];

            if(obj->hasComponent("hit") && obj->getComponent<bool>("hit") == true) continue;
            // Update aliens
            if(obj->hasComponent("isAlien") && obj->getComponent<bool>("isAlien") == true) {
                obj->setComponent("offset", obj->getComponent<float>("offset") + obj->getComponent<float>("velocity") * dt);
                if(obj->getComponent<float>("offset") > 100 || obj->getComponent<float>("offset") < -100) {
                    if(obj->getComponent<float>("offset") > 0) {
                        obj->setComponent("offset", (float) 100);
                    } else {
                        obj->setComponent("offset", (float) -100);
                    }
                    obj->setComponent("velocity", obj->getComponent<float>("velocity") * -1);
                    obj->setComponent("position", Vector{obj->getComponent<Vector>("position").x, obj->getComponent<Vector>("position").y + 50});
                }
                obj->setComponent("position", Vector{obj->getComponent<Vector>("spawnLoc").x + obj->getComponent<float>("offset"), obj->getComponent<Vector>("position").y});
            
                if(timeSinceLastAlienShot > 5) {
                    shot = true;
                    GameObject* bullet = objectPool->spawn();
                    bullet->setComponent("texture", bulletTexture);
                    bullet->setComponent("dimensions", Vector{32, 32});
                    bullet->setComponent("position", Vector(obj->getComponent<Vector>("position").x + obj->getComponent<Vector>("dimensions").x / 2, obj->getComponent<Vector>("position").y));
                    bullet->setComponent("alienBullet", true);
                    bullet->setComponent("active", true);
                    bullet->setComponent("alien", true);
                    bullet->setComponent("velocity", Vector{0, 750});
                    clientObjectList.push_back(bullet);
                }
            
            
            }
            // Update alien bullets
            if(obj->hasComponent("alienBullet")) {
                Vector pos = obj->getComponent<Vector>("position");
                Vector vel = obj->getComponent<Vector>("velocity");

                pos.x += vel.x * dt;
                pos.y += vel.y * dt;

                obj->setComponent("position", pos);

                // Kill player if hit by alien bullet
                if(overlappingColliders1(*obj, *player)) {
                    std::cout << "Man down!\n";


                    obj->setComponent("active", false);

                    auto deathEvent = std::make_shared<DeathEvent>(std::chrono::steady_clock::now().time_since_epoch().count(), -1);
                    eventManager.QueueEvent(deathEvent);
                }
            }
            // Update player bullets
            if(obj->hasComponent("playerBullet")) {
                Vector pos = obj->getComponent<Vector>("position");
                Vector vel = obj->getComponent<Vector>("velocity");

                pos.x += vel.x * dt;
                pos.y += vel.y * dt;

                obj->setComponent("position", pos);

                for (auto& alien : clientObjectList) {
                    if (!alien->hasComponent("isAlien") || alien->getComponent<bool>("hit")) continue;
                    if(overlappingColliders1(*obj, *alien) && !obj->hasComponent("alien")) {
                        obj->setComponent("active", false);
                        alien->setComponent("hit", true);

                        break;
                    }
                }

            }
            
            // Start assigning info to render the texture
            if(obj->hasComponent("texture") && obj->hasComponent("position") && obj->hasComponent("dimensions")) {
                SDL_Texture *thisTexture = obj->getComponent<SDL_Texture *>("texture");
                Vector pos = obj->getComponent<Vector>("position");
                Vector dim = obj->getComponent<Vector>("dimensions");
                SDL_FRect destRect = {pos.x, pos.y, dim.x, dim.y};
                SDL_RenderTexture(renderer, thisTexture, NULL, &destRect);
            }
        } 


        // Reset alien bullet timer
        if(shot) {
            timeSinceLastAlienShot = 0;
        }
        // Re-render screen
        refreshScreen(renderer);

        // Check to see if all aliens dead
        int alienCt = 0;
        for (auto& alien : clientObjectList) {
            if(alien->hasComponent("hit") && alien->getComponent<bool>("hit") == true) alienCt++;
        }
        // If so, exit and pronounce player as winner!
        if(alienCt == 12) {
            std::cout << "You Win!\n";
            break;
        }
    }
    
    // Clean up!
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

