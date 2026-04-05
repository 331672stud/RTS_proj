#include <stdio.h>
#include <osmium/handler.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/io/reader.hpp>
#include <osmium/visitor.hpp>
#include <unordered_map>
#include <vector>

struct Point{
    double lat;
    double lon;
};

struct Edge{
    long target;
    double weight;
};

class MyHandler : public osmium::handler::Handler {
    public:
        void node(const osmium::Node& node) {
        //handle node
        }
};

void findBestPath(){
    osmium::io::File infile("map.pbf");
    osmium::io::Reader reader(infile);

    MyHandler handler;
    osmium::apply(reader, handler);

    std::unordered_map<long, std::vector<Edge>> graph;

    reader.close();
}