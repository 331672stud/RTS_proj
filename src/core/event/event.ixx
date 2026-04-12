export module core.event;

import core.time;


/**
 * @brief Typy wydarzeń
 * 
 */
export enum class EventType {
    SensorRead, //procesowanie otrzymanych danych
    PositionUpdate, //na aktualizacje pozycji na mapie
    PositionWarn, //Ostrzeżenie o opuszczeniu trasy
    TrafficUpdate, //update dotyczycący trasy
    ReplanRoute //rekalkulacja trasy
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