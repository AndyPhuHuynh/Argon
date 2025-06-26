// Demonstrates how to parse user defined data types

#include <iostream>

#include "../include/Argon/Options/Option.hpp"
#include "Argon/Parser.hpp"

int main() {
    struct Student {
        std::string name = "Default";
        int age = 0;
    };

    auto studentConversionFn = [](const std::string_view str, Student& out) -> bool {
        if (str == "1") {
            out = { .name = "Josh", .age = 1 };
            return true;
        } else if (str == "2") {
            out = { .name = "Sally", .age = 2 };
            return true;
        }
        return false;
    };

    auto studentErrorFn = [](const std::string_view flag, const std::string_view invalidArg) -> std::string {
        return std::format("Invalid value for flag '{}': expected either '1' or '2', got '{}'", flag, invalidArg);
    };

    Student student1, student2;
    auto parser = Argon::Option(&student1)["--first"]
                    .withConversionFn(studentConversionFn).withErrorMsgFn(studentErrorFn)
                | Argon::Option(&student2)["--second"]
                    .withConversionFn(studentConversionFn).withErrorMsgFn(studentErrorFn);

    const std::string input = "--first 1 --second 3";
    parser.parse(input);

    std::cout << "Student1: " << student1.name << ", " << student1.age << "\n";
    std::cout << "Student2: " << student2.name << ", " << student2.age << "\n";
}