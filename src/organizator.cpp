#include <stdio.h>
#include <string>
#include <vector>

class Organizator{
    private:
        struct Queue{
            std::vector<int> data;
            Queue* next;
            int priority;
        };
    public:
        void handleQueue(std::string calledArgument){
            //organize queue
        }

        void sendNext(){
            //send next in queue
        }

        void hookEvent(std::string eventName){
            //hook event
        }

        void unhookEvent(std::string eventName){
            //unhook event
        }

        void sendStartData(){
            //send start data
        }
};