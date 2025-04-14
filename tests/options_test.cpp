#include "testing.hpp"

#include "Argon/Argon.hpp"
#include "Argon/Option.hpp"

using namespace Argon;

void Test1() {
    unsigned int width = 0;
    float height;
    double depth;
    int test;
    
    Option opt2 = Option(&height)["-h"]["--height"];
    Option opt3 = Option(&depth)["-d"]["--depth"];

    Parser parser = Option(&width)["-w"]["--width"] | opt2 | opt3 | Option(&test)["-t"]["--test"];
    std::string str = "--width -100 --height 50.1 --depth 69.123456 -t 152";
    parser.parseString(str);

    bool success =  (height >= 50 && height <= 50.1) &&
                    (depth >= 69.1 && depth <= 69.2) &&
                    test == 152;
    check(success, "Option Test 1");
}

void StructTest() {
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
    Parser studentParser = studentOption;

    std::string studentString = "--student 3";
    studentParser.parseString(studentString);

    bool success = testStudent.name == "Sally" && testStudent.age == 3;
    check(success, "Option custom struct");
}

void runOptionsTests() {
    Test1();
    StructTest();
}