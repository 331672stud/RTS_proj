module;

#include <cstddef>
#include <chrono>

export module system.config;

import core.task;

export constexpr int TICK_MS = 100;   // 10 ms tick = 100Hz

export constexpr size_t MAX_TASKS = 16;
export constexpr size_t EVENT_QUEUE_SIZE = 128;

export constexpr TaskPriority PRIORITY_HIGH = TaskPriority::High;
export constexpr TaskPriority PRIORITY_MED  = TaskPriority::Medium;
export constexpr TaskPriority PRIORITY_LOW  = TaskPriority::Low;