export module core.task;

import core.time;

export enum class TaskPriority {
    High,
    Medium,
    Low
};

export template<typename Context>
struct Task {
    void (*func)(Context&);
    TaskPriority priority;
    Tick period;
    Tick next_run{0};
};