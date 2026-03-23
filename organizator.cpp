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

        void HandleQueue(std::string calledArgument){
            //organize queue
        }
};