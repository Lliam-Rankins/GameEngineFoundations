/*
    This is a file that runs the main loop for peer to peer.
    Some of the content in this file was generated with Gemini 2.5 Pro.
    This citation is to abide by the syllabus requirement that "appropriate citations"
    must be given when referring to external sources.
*/
#include "../engine/headers/render.h"
#include "../engine/headers/physics.h"
#include "../engine/headers/collisions.h"
#include "../engine/headers/P2PClientManager.h"
#include "../engine/headers/protocol.h"
#include "../engine/headers/entities.h"
#include "../engine/headers/input.h"
#include "../engine/headers/timeline.h"
#include <map>
#include <memory>
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

void SyncColliderToEntityAt(Entity &entity, Vector pos)
{
    if (entity.collider)
    {
        entity.collider->topLeft.x = pos.x - entity.dimensions.x / 2.0f;
        entity.collider->topLeft.y = pos.y - entity.dimensions.y / 2.0f;
        entity.collider->bottomRight.x = pos.x + entity.dimensions.x / 2.0f;
        entity.collider->bottomRight.y = pos.y + entity.dimensions.y / 2.0f;
    }
}

int main(int argc, char *argv[])
{
    // 1. Read P2P Port from Command Line
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <my_p2p_port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 6000" << std::endl;
        return 1;
    }
    int my_p2p_port = std::stoi(argv[1]);

    // --- Standard Game and SDL Setup ---
    initializeSDL();
    createWindowAndRenderer(&window, &renderer);
    SDL_Texture *platformTexture = IMG_LoadTexture(renderer, "media/warped city files/warped city files/Assets/ENVIRONMENT/props/control-box-3.png");
    SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "media/warped city files/warped city files/Assets/SPRITES/player/idle/idle-1.png");
    SDL_Texture *policeTexture = IMG_LoadTexture(renderer, "media/warped city files/warped city files/Assets/SPRITES/vehicles/v-police.png");
    if (!platformTexture || !playerTexture || !policeTexture)
        textureError();

    Timeline mainTimeline;
    P2PClientManager p2pManager;

    // 2. Join the P2P Network via the Authoritative Server (Matchmaker)
    if (!p2pManager.join_network("localhost", 5555, my_p2p_port))
    {
        std::cerr << "Failed to connect to the matchmaker server." << std::endl;
        return 1;
    }
    int myClientId = p2pManager.get_my_id();

    // --- Entity and Renderer Setup ---
    Vector platPos = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
    Vector platDim = {62, 30};
    Entity platform(platPos, platDim, {0, 0}, false);
    Collider platformCollider(platPos.x - platDim.x / 2.0f, platPos.y - platDim.y / 2.0f, platPos.x + platDim.x / 2.0f, platPos.y + platDim.y / 2.0f);
    platform.setCollider(&platformCollider);

    Vector playerDim = {71, 67};
    Vector playerPos = {WINDOW_WIDTH / 2.0f, platPos.y - (platDim.y / 2.0f) - (playerDim.y / 2.0f)};
    Entity localPlayer(playerPos, playerDim, {0, 0}, false);
    Collider playerCollider(playerPos.x - playerDim.x / 2.0f, playerPos.y - playerDim.y / 2.0f, playerPos.x + playerDim.x / 2.0f, playerPos.y + playerDim.y / 2.0f);
    localPlayer.setCollider(&playerCollider);
    SyncColliderToEntity(localPlayer);

    // The police car is now just a visual placeholder. The server will tell us where it is.
    Entity police({0, 0}, {163, 60}, {0, 0}, false);
    Collider policeCollider;
    police.setCollider(&policeCollider);
    SyncColliderToEntity(police); // Sync it to the initial (0,0) position

    std::map<int, std::unique_ptr<Entity>> remotePlayers;
    std::map<int, RenderComponent> playerRenderers;
    playerRenderers.emplace(myClientId, RenderComponent(playerTexture));

    RenderComponent platformRenderer(platformTexture);
    RenderComponent policeRenderer(policeTexture);

    // --- Main Game Loop ---
    bool running = true;
    bool graveKeyWasPressedLastFrame = false;
    bool spaceKeyWasPressedLastFrame = false;
    // Scaling Type bool
    bool constantSizeScale = true;

    while (running)
    {
        mainTimeline.update();
        float dt = mainTimeline.getDeltaTime();
        const float MAX_DELTA_TIME = 1.0f / 30.0f; // Cap at 30 FPS
        if (dt > MAX_DELTA_TIME)
        {
            dt = MAX_DELTA_TIME;
        }
		if (dt <= 0)
		{
			// A negative dt is a clock error, and a zero dt means the game is paused.
			// In either case, we should skip all logic and rendering for this frame.
			// We can print a warning to the console to know when it happens.
			if (dt < 0)
			{
				std::cerr << "WARNING: Negative delta time detected (" << dt << "s). Skipping frame to maintain stability." << std::endl;
			}
			continue; // Immediately start the next loop iteration
		}
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
        }

        bool graveKeyIsPressedNow = isKeyPressed(SDL_SCANCODE_GRAVE);
        bool spaceKeyIsPressedNow = isKeyPressed(SDL_SCANCODE_SPACE);

        if (graveKeyIsPressedNow && !graveKeyWasPressedLastFrame)
        {
            constantSizeScale = !constantSizeScale;
        }

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
            localPlayer.velocity.x = -300.0f;
        }
        if (isKeyPressed(SDL_SCANCODE_D))
        {
            localPlayer.velocity.x = 300.0f;
        }

        // Jump is an impulse (instantaneous change), so it doesn't use dt.
        if (spaceKeyIsPressedNow && !spaceKeyWasPressedLastFrame && localPlayer.velocity.y == 0)
        {
            localPlayer.velocity.y = -900.0f; // A proper jump impulse
        }

        if (localPlayer.position.y < 0)
        {
            localPlayer.position.y = 0;
            localPlayer.velocity.y = 0;
        }

        graveKeyWasPressedLastFrame = graveKeyIsPressedNow;
        spaceKeyWasPressedLastFrame = spaceKeyIsPressedNow;

        localPlayer.velocity.y += WorldPhysics::getGravity() * dt;

        // Calculate potential next position for this axis
        Vector nextPosition = localPlayer.position;
        nextPosition.x += localPlayer.velocity.x * dt;

        // Update the collider to this potential next position to check for collisions
        SyncColliderToEntityAt(localPlayer, nextPosition);

        // Check for collision at the potential position
        if (overlappingColliders(*localPlayer.collider, *platform.collider))
        {
            // Collision detected, resolve it.
            if (localPlayer.velocity.x > 0) // Moving right
            {
                // Snap the 'nextPosition' to the left side of the platform
                nextPosition.x = platform.collider->topLeft.x - localPlayer.dimensions.x / 2.0f;
            }
            else if (localPlayer.velocity.x < 0) // Moving left
            {
                // Snap the 'nextPosition' to the right side of the platform
                nextPosition.x = platform.collider->bottomRight.x + localPlayer.dimensions.x / 2.0f;
            }
            localPlayer.velocity.x = 0; // Stop horizontal movement
        }

        // Now calculate the Y-axis movement from our (potentially corrected) X position
        nextPosition.y += localPlayer.velocity.y * dt;

        // Update the collider again to the final potential position
        SyncColliderToEntityAt(localPlayer, nextPosition);

        bool grounded = false;
        if (overlappingColliders(*localPlayer.collider, *platform.collider))
        {
            if (localPlayer.velocity.y > 0) // Moving down
            {
                // Snap the 'nextPosition' to be on top of the platform
                nextPosition.y = platform.collider->topLeft.y - localPlayer.dimensions.y / 2.0f;
                localPlayer.velocity.y = 0;
                grounded = true;
            }
            else if (localPlayer.velocity.y < 0) // Moving up
            {
                // Snap the 'nextPosition' to be at the bottom of the platform
                nextPosition.y = platform.collider->bottomRight.y + localPlayer.dimensions.y / 2.0f;
                localPlayer.velocity.y = 0;
            }
        }

        // Only after all physics and collision checks are done, we commit the final, safe position.
        localPlayer.position = nextPosition;
        SyncColliderToEntity(localPlayer);

        // Handle special collisions, like with the police car, after resolving physics
        if (overlappingColliders(*localPlayer.collider, *police.collider))
        {
            // For the police car, we can just reset the player's position as a penalty
            localPlayer.position = playerPos; // Reset to initial spawn position
            localPlayer.velocity = {0, 0};
            SyncColliderToEntity(localPlayer);
        }

        PlayerState myState;
        myState.clientId = myClientId;
        myState.x = localPlayer.position.x;
        myState.y = localPlayer.position.y;
        p2pManager.broadcast_state(myState);

        AllUpdates updates = p2pManager.poll_updates();

        // Process updates from other players
        for (const auto &peer_state : updates.player_states)
        {
            if (peer_state.clientId == myClientId)
            {
                continue;
            }

            if (remotePlayers.find(peer_state.clientId) == remotePlayers.end())
            {
                remotePlayers[peer_state.clientId] = std::make_unique<Entity>(
                    Vector{peer_state.x, peer_state.y}, playerDim, Vector{0, 0}, false);
                playerRenderers.emplace(peer_state.clientId, RenderComponent(playerTexture));
            }
            else
            {
                remotePlayers.at(peer_state.clientId)->position = {peer_state.x, peer_state.y};
            }
        }

        // Process updates for server-controlled objects
        for (const auto &npc_state : updates.npc_states)
        {
            if (npc_state.objectId == 0)
            { // Assuming 0 is the police car
                police.position = {npc_state.x, npc_state.y};
                SyncColliderToEntity(police);
            }
        }

        setupScreen(renderer);
        platformRenderer.render(renderer, platform.position, platform.dimensions);
        policeRenderer.render(renderer, police.position, police.dimensions);

        if (playerRenderers.count(myClientId))
        {
            playerRenderers.at(myClientId).render(renderer, localPlayer.position, localPlayer.dimensions);
        }
        for (auto const &[id, remote_player_ptr] : remotePlayers)
        {
            if (playerRenderers.count(id))
            {
                playerRenderers.at(id).render(renderer, remote_player_ptr->position, remote_player_ptr->dimensions);
            }
        }
        refreshScreen(renderer);
    }

    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(platformTexture);
    SDL_DestroyTexture(policeTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    p2pManager.clean_up();

    SDL_Quit();
    return 0;
}