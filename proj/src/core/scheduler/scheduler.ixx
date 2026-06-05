module;

#include <array>
#include <stdexcept>

export module core.scheduler;

import app.tasks;
import core.task;
import core.queue;
import core.event;
import core.time;
import app.navigationController;
import system.config;

export template<typename Context, size_t MaxTasks, size_t QueueSize>
class Scheduler {
public:
    Scheduler(Context& ctx) : context(ctx) {
        context.queue=&queue;
    }

    // Add a periodic or one‑shot task (period == 0 means one‑shot)
    void addTask(const Task<Context>& task) {
        if (task_count >= MaxTasks) {
            throw std::runtime_error("Maximum number of tasks reached");
        }
        tasks[task_count] = task;
        tasks[task_count].next_run = current_tick + (task.period > 0 ? task.period : 0);
        ++task_count;
    }

    // Convenience method for one‑shot tasks
    void addOneShotTask(void (*func)(Context&)) {
        Task<Context> t{func, PRIORITY_HIGH, 0, 0};
        addTask(t);
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
            switch (e->type) {
                case EventType::GraphUpdate:
                    onGraphUpdate(context, queue, e->data);
                    break;
                case EventType::OffRouteDetected:
                    onOffRouteDetected(context, queue, e->data);
                    break;
                case EventType::RouteNodesUpdate:
                    onRouteNodesUpdate(context, queue, e->data);
                    break;
                case EventType::VehicleUpdate:
                    onVehicleUpdate(context, queue, e->data);
                    break;
                case EventType::LocalReplanRequest:
                    addOneShotTask(taskLocalReplan);
                    break;
                case EventType::GlobalReplanRequest:
                    addOneShotTask(taskGlobalReplan);
                    break;
            }
        }
    }

    void runTasks() {
        for (size_t i = 0; i < task_count; ) {
            auto& t = tasks[i];
            if (current_tick >= t.next_run) {
                t.func(context);
                if (t.period == 0) {
                    // One‑shot task – remove by swapping with the last task
                    tasks[i] = tasks[task_count - 1];
                    --task_count;
                    // Do not increment i; re‑examine the swapped task
                } else {
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