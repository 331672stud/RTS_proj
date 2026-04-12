module;

#include <optional>
#include <array>

export module core.queue;

import core.event;

/**
 * @brief Kolejka dla wydarzeń
 * 
 */
export template<size_t Capacity>
class EventQueue {
    public:
        bool push(const Event& e){
            if(size == Capacity){
                return false;  
            }

            buffer[tail] = e;
            tail = (tail + 1) % Capacity;
            size++;
        }   
        std::optional<Event> pop(){
            if (size == 0) {
                return std::nullopt;
            }

            Event e = buffer[head];
            head = (head + 1) % Capacity;
            size--;
        }
    private:
        std::array<Event, Capacity> buffer;
        size_t head = 0;
        size_t tail = 0;  
        size_t size = 0;
};