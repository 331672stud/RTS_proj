export module app.context;

import core.event;

export struct TaskContext {
    // USER STATE (no logic here)

    // Event handlers (to be implemented elsewhere)
    void onPositionUpdate(void* data){}
    void onTrafficUpdate(void* data){}
    void onRoadBlocked(void* data){}

    void requestLocalReplan(){}
    void requestGlobalReplan(){}
};