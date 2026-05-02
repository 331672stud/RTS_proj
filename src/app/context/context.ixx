export module app.context;

import core.event;

export struct TaskContext {    
    void onGraphUpdate(void* data);          // aktualizacja grafu (wypadki, itd)
    void onOffRouteDetected(void* data);     // wyznacza nową trasę
    
    void requestLocalReplan();
    void requestGlobalReplan();
    void requestOffRouteWarning();            // powiadamia użytkownika
    void requestPositionUpdate();
};