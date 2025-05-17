// Demonstrates how to parse user defined data types

#include <iostream>

#include "Argon/MultiOption.hpp"
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

    auto studentErrorFn = [](const std::string& flag, const std::string& invalidArg) -> std::string {
        return std::format("Invalid value for flag '{}': expected either '1' or '2', got '{}'", flag, invalidArg);
    };

    std::string school;
    std::vector<Student> students;

    auto parser = Argon::Option(&school)["--school"]
                | Argon::MultiOption(&students, studentConversionFn, studentErrorFn)["--students"];

    const std::string input = "--students 1 2 --school University";

    parser.parseString(input);
    std::cout << "school: " << school << '\n';
    std::cout << "students[0]: " << students[0].name << ", " << students[0].age << "\n";
    std::cout << "students[1]: " << students[1].name << ", " << students[1].age << "\n";
}