module;

#include <cstddef>

export module system.config;

import core.task;

export constexpr size_t MAX_TASKS = 16;
export constexpr size_t EVENT_QUEUE_SIZE = 128;

export constexpr TaskPriority PRIORITY_HIGH = TaskPriority::High;
export constexpr TaskPriority PRIORITY_MED  = TaskPriority::Medium;
export constexpr TaskPriority PRIORITY_LOW  = TaskPriority::Low;