// Demonstrates basic usage Argon argument parser

#include <iostream>

#include "../include/Argon/Options/Option.hpp"
#include "Argon/Parser.hpp"

int main() {
    std::string name = "John";
    auto parser = Argon::Parser(Argon::Option(&name)["-n"]["--name"]);

    const std::string input = "--name Bob";
    parser.parse(input); // Name is "Bob" here
    std::cout << "Name: " << name << "\n";

    const std::string input2 = "-n Josh";
    parser.parse(input2); // Name is "Josh" here
    std::cout << "Name: " << name << "\n";
}