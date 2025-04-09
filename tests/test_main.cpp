#include "Testing.hpp"

#include <iostream>

#include <Argon/Argon.hpp>
#include "Argon/Option.hpp"

int main() {
    using namespace Argon;
    
    unsigned int width;
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
    std::string test5 = "--width -100 --height 50.1 --depth 69.123456 -t 152";
    parser.parseString(test5);

    std::cout << "Width: " << width << "\n";
    std::cout << "Height: " << height << "\n";
    std::cout << "Depth: " << depth << "\n";
    std::cout << "Test: " << test << "\n";

    struct Student {
        std::string name;
        int age;
    };

    auto studentFromString = [](const std::string & str, Student& out) -> bool {
        if (str == "1") {
            out = {
                .name = "Josh",
                .age = 1
            };
            return true;
        } else if (str == "2") {
            out = {
                .name = "John",
                .age = 2
            };
            return true;
        } else if (str == "3") {
            out = {
                .name = "Sally",
                .age = 3
            };
            return true;
        }
        return false;
    };

    auto printStudent = [](const Student& student) {
        std::cout << "Name: " << student.name << "\n";
        std::cout << "Age: " << student.age << "\n";
    };

    auto studentError = [](const std::string& flag, const std::string& invalidArg) -> std::string {
        std::stringstream ss;
        ss << "Flag: " << flag << ": valid options are 1, 2, and 3";
        return ss.str();
    };
    
    Student testStudent{};
    Option studentOption = Option<Student>(&testStudent, studentFromString, studentError)["-s"]["--student"];
    Parser studentParser = studentOption | opt2;

    std::string studentString = "--student 100";
    studentParser.parseString(studentString);
    printStudent(testStudent);

    bool boolTest = true;
    std::cout << "boolTest: " << boolTest << "\n";
    Parser boolParser = Option(&boolTest)["-b"]["--bool"];
    boolParser.parseString("--bool trueasdfads");
    std::cout << "boolTest: " << boolTest << "\n";

    runScannerTests();
}
