#include <Argon.hpp>

#include "Option.hpp"

int main() {
    using namespace Argon;

    int width;
    float height;
    double depth;
    int test;
    
    // Option<int> opt1 = Option<int>()["-w"]["--width"];
    Option opt2 = Option(&height)["-h"]["--height"];
    Option opt3 = Option(&depth)["-d"]["--depth"];
    
    std::string test1 = "test1";
    std::string test2 = "-w";
    std::string test3 = "-h";
    std::string test4 = "--width --height";

    Parser parser = Option(&width)["-w"]["--width"] | opt2 | opt3 | Option(&test)["-t"]["--test"];
    std::string test5 = "--width 100 --height 50.1 --depth 69.123456 -t 152";
    parser.parseString(test5);

    std::cout << "Width: " << width << "\n";
    std::cout << "Height: " << height << "\n";
    std::cout << "Depth: " << depth << "\n";
    std::cout << "Test: " << test << "\n";
}
