#pragma once
#include <functional>
#include <vector>
#include "sharedData.h"

using Job = std::function<void()>;
using JobQueue = std::vector<Job>;

void worker(SharedData& data, const JobQueue& jobs);
