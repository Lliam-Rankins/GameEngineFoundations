/*
 	Some of the content in this file was generated with Gemini 2.5 Pro.
    This citation is to abide by the syllabus requirement that "appropriate citations"
    must be given when referring to external sources. More information is available upon request.
 */
#pragma once

#include "GameObject.h"
#include <vector>
#include <string>



/**
 * @brief Searches a list of GameObjects for one with a matching client_id.
 * * NOTE: This function does NOT lock the mutex.
 * The caller is responsible for locking and unlocking the list's mutex
 * before calling this.
 *
 * @param id The client ID to search for.
 * @param objectList The list of GameObjects to search.
 * @return A pointer to the found GameObject, or nullptr if not found.
 */
inline GameObject *findGameObjectByClientId(int id, std::vector<GameObject *> &objectList)
{
    for (auto &obj : objectList)
    {
        // Check if the object has a "client_id" component
        if (obj->hasComponent("client_id"))
        {
            // Check if the component's value matches the id
            if (obj->getComponent<int>("client_id") == id)
            {
                return obj;
            }
        }
    }
    return nullptr; // Not found
}

// You can also add your client-side helper here
inline GameObject *findLocalObject(int id, std::vector<GameObject *> &objectList)
{
    // This might search for "client_id" OR "npc_id"
    for (auto &obj : objectList)
    {
        if (obj->hasComponent("client_id") && obj->getComponent<int>("client_id") == id)
        {
            return obj;
        }
        if (obj->hasComponent("npc_id") && obj->getComponent<int>("npc_id") == id)
        {
            return obj;
        }
    }
    return nullptr;
}

// Find an object by its NPC id only (does not match client_id). Use this when
// processing NPCState entries to avoid colliding with client IDs.
inline GameObject *findGameObjectByNpcId(int id, std::vector<GameObject *> &objectList)
{
    for (auto &obj : objectList)
    {
        if (obj->hasComponent("npc_id") && obj->getComponent<int>("npc_id") == id)
            return obj;
    }
    return nullptr;
}