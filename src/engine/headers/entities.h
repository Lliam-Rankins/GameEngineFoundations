#pragma once

#include <mutex>
#include <atomic>
#include "struct.h"
#include "collisions.h"
#include "mathEngine.h"
#include <SDL3_image/SDL_image.h>
#include <iostream>



void updatePosition(GameObject &obj, bool isPaused);

