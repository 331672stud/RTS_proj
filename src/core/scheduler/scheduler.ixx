module;

#include <array>
#include <cstddef>

export module core.scheduler;

import core.task;
import core.queue;
import core.event;
import core.time;

export template<typename Context, size_t MaxTasks, size_t QueueSize>
class Scheduler {
public:
    Scheduler(Context& ctx) : context(ctx) {}

    void addTask(const Task<Context>& task){}

    void tick() {
        current_tick++;
        processEvents();
        runTasks();
    }

    EventQueue<QueueSize>& getEventQueue() { return queue; }

private:
    void processEvents(){
        while(auto e = queue.pop()){
            dispatchEvent(context, *e);
        }
    }
    void runTasks(){
        for(size_t i = 0; i < task_count; ++i){
            auto& t = tasks[i];
            if(current_tick >= t.next_run){
                t.func(context);
                t.next_run = current_tick + t.period;
            }
        }
    }    

private:
    std::array<Task<Context>, MaxTasks> tasks{};
    size_t task_count{0};
    Tick current_tick{0};

    EventQueue<QueueSize> queue;
    Context& context;
};