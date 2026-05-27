module;

#include <chrono>
#include <cstdint>

export module core.time;

export using Tick = uint64_t;

export inline Tick now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}