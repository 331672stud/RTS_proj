module;

#include <array>
#include <stdexcept>

export module core.scheduler;

import app.dispatcher;
import core.task;
import core.queue;
import core.event;
import core.time;

export template<typename Context, size_t MaxTasks, size_t QueueSize>
class Scheduler {
public:
    Scheduler(Context& ctx) : context(ctx) {}

    void addTask(const Task<Context>& task) {
        if (task_count >= MaxTasks) {
            throw std::runtime_error("Maksymalna liczba tasków");
        }
        tasks[task_count] = task;
        // Initialize next_run: run as soon as possible (current tick) or after period?
        // For periodic tasks, we set next_run = current_tick + period
        // For one‑shot (period == 0), we set next_run = current_tick (run immediately on next tick)
        tasks[task_count].next_run = current_tick + (task.period > 0 ? task.period : 0);
        ++task_count;
    }

    void tick() {
        ++current_tick;
        processEvents();
        runTasks();
    }

    EventQueue<QueueSize>& getEventQueue() { return queue; }

private:
    void processEvents() {
        while (auto e = queue.pop()) {
            dispatchEvent(context, *e);
            //delete e;   // assume Event was allocated with new; adjust if needed
        }
    }

    void runTasks() {
        for (size_t i = 0; i < task_count; ) {
            auto& t = tasks[i];
            if (current_tick >= t.next_run) {
                t.func(context);            // execute the task
                if (t.period == 0) {
                    // One‑shot task: remove it by swapping with the last task
                    tasks[i] = tasks[task_count - 1];
                    --task_count;
                    // Do not increment i; re‑check the swapped task
                } else {
                    // Periodic task: schedule next run
                    t.next_run = current_tick + t.period;
                    ++i;
                }
            } else {
                ++i;
            }
        }
    }

    std::array<Task<Context>, MaxTasks> tasks{};
    size_t task_count{0};
    Tick current_tick{0};

    EventQueue<QueueSize> queue;
    Context& context;
};