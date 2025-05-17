#include "testing.hpp"

#include "Argon/Parser.hpp"
#include "Argon/Option.hpp"

using namespace Argon;

static void Test1() {
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

static void StructTest() {
    struct Student {
        std::string name;
        int age;
    };

    auto studentFromString = [](const std::string& str, Student& out) -> bool {
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

    auto studentError = [](const std::string& flag, const std::string& invalidArg) -> std::string {
        return std::format("Invalid value for flag '{}': expected either '1' or '2', got '{}'", flag, invalidArg);
    };
    
    Student testStudent{};
    Option studentOption = Option<Student>(&testStudent, studentFromString, studentError)["-s"]["--student"];
    Parser studentParser = static_cast<Parser>(studentOption);

    std::string studentString = "--student 3";
    studentParser.parseString(studentString);

    bool success = testStudent.name == "Sally" && testStudent.age == 3;
    check(success, "Option custom struct");
}

void runOptionsTests() {
    Test1();
    StructTest();
}