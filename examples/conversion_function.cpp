// Demonstrates how to parse user defined data types

#include <iostream>

#include "Argon/Option.hpp"
#include "Argon/Parser.hpp"

int main() {
    struct Student {
        std::string name = "Default";
        int age = 0;
    };

    auto studentConversionFn = [](const std::string& str, Student& out) -> bool {
        if (str == "1") {
            out = { .name = "Josh", .age = 1 };
            return true;
        } else if (str == "2") {
            out = { .name = "Sally", .age = 2 };
            return true;
        }
        return false;
    };

    Student student1, student2;
    auto parser = Argon::Option<Student>(&student1, studentConversionFn)["--first"]
                | Argon::Option<Student>(&student2, studentConversionFn)["--second"];

    const std::string input = "--first 1 --second 3";
    parser.parse(input);
    std::cout << "Student1: " << student1.name << ", " << student1.age << "\n";
    std::cout << "Student2: " << student2.name << ", " << student2.age << "\n";
}