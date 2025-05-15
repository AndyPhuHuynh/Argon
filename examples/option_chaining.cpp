// Demonstrates how to combine options together into one parser

#include <iostream>

#include "Argon/Option.hpp"
#include "Argon/Parser.hpp"

int main() {
    std::string name = "John";
    int age = 0;
    float salary = 0.0f;

    auto parser = Argon::Option(&name)["-n"]["--name"]
                | Argon::Option(&age)["-a"]["--age"]
                | Argon::Option(&salary)["-s"]["--salary"];
    const std::string input = "--name Bob -a 20 --salary 15.5";

    parser.parseString(input);
    std::cout << "Name: " << name << "\n";
    std::cout << "Age: " << age << "\n";
    std::cout << "Salary: " << salary << "\n";
}