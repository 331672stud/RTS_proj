export module core.event;

import core.time;


/**
 * @brief Typy wydarzeń
 * 
 */
export enum class EventType {
    GraphUpdate,       // Aktualizacja grafu
    OffRouteDetected   // gdy pozycja po za trasą
};

/**
 * @brief struct wydarzenia
 * 
 */
export struct Event {
    EventType type;
    Tick timestamp;
    void* data; 
};