#include <stdio.h>
#include <string>
#include <vector>

namespace org

{
    class Organizator
    {
    public:
        Organizator();
        ~Organizator();

        void addTask(const std::string& task);
        void removeTask(const std::string& task);
        void displayTasks() const;

    private:
        std::vector<std::string> tasks;
    };
}