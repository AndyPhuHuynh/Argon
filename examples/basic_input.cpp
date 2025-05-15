// Demonstrates basic usage Argon argument parser

#include <iostream>

#include "Argon/Option.hpp"
#include "Argon/Parser.hpp"

int main() {
    std::string name = "John";
    auto parser = Argon::Parser(Argon::Option(&name)["-n"]["--name"]);

    const std::string input = "--name Bob";
    parser.parseString(input); // Name is "Bob" here
    std::cout << "Name: " << name << "\n";

    const std::string input2 = "-n Josh";
    parser.parseString(input2); // Name is "Josh" here
    std::cout << "Name: " << name << "\n";
}