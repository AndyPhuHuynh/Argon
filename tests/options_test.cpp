#include "Argon/Parser.hpp"
#include "Argon/Option.hpp"

#include "catch2/catch_approx.hpp"
#include "catch2/catch_test_macros.hpp"

using namespace Argon;

TEST_CASE("Basic option test case 1", "[options]") {
    unsigned int width = 0;
    float height;
    double depth;
    int test;

    Option opt2 = Option(&height)["-h"]["--height"];
    Option opt3 = Option(&depth)["-d"]["--depth"];

    Parser parser = Option(&width)["-w"]["--width"] | opt2 | opt3 | Option(&test)["-t"]["--test"];
    const std::string input = "--width -100 --height 50.1 --depth 69.123456 -t 152";
    parser.parseString(input);

    CHECK(width   == 0);
    CHECK(height  == Catch::Approx(50.1)      .epsilon(1e-6));
    CHECK(depth   == Catch::Approx(69.123456) .epsilon(1e-6));
    CHECK(test    == 152);
}

TEST_CASE("Basic option test 2", "[options]") {
    std::string name;
    int age;
    std::string major;

    auto parser = Option(&name)["--name"]
                | Option(&age)["--age"]
                | Option(&major)["--major"];

    const std::string input = "--name John --age 20 --major CS";
    parser.parseString(input);

    CHECK(name == "John");
    CHECK(age == 20);
    CHECK(major == "CS");
}

TEST_CASE("Option all built-in types", "[options]") {
    bool                fb  = true;
    bool                tb  = false;
    char                c   = 0;
    signed char         sc  = 0;
    unsigned char       uc  = 0;
    signed short        ss  = 0;
    unsigned short      us  = 0;
    signed int          si  = 0;
    unsigned int        ui  = 0;
    signed long         sl  = 0;
    unsigned long       ul  = 0;
    signed long long    sll = 0;
    unsigned long long  ull = 0;

    float               f   = 0;
    double              d   = 0;
    long double         ld  = 0;

    auto parser = Option(&fb)  ["-fb"]      | Option(&tb)  ["-tb"]
                | Option(&sc)  ["-sc"]      | Option(&uc)  ["-uc"]      | Option(&c)  ["-c"]
                | Option(&ss)  ["-ss"]      | Option(&us)  ["-us"]
                | Option(&si)  ["-si"]      | Option(&ui)  ["-ui"]
                | Option(&sl)  ["-sl"]      | Option(&ul)  ["-ul"]
                | Option(&sll) ["-sll"]     | Option(&ull) ["-ull"]
                | Option(&f)   ["-f"]       | Option(&d)   ["-d"]       | Option(&ld) ["-ld"];
    const std::string input = "-fb   false          -tb  false "
                              "-sc  -10             -uc  10                 -c a "
                              "-ss  -300            -us  300 "
                              "-si  -123456         -ui  123456 "
                              "-sl  -123456         -ul  123456 "
                              "-sll -1234567891011  -ull 1234567891011 "
                              "-f    0.123456       -d   0.123456           -ld 0.123456 ";
    parser.parseString(input);

    CHECK(fb  == false);            CHECK(tb == false);
    CHECK(sc  == -10);              CHECK(uc == 10);    CHECK(c == 'a');
    CHECK(ss  == -300);             CHECK(us == 300);
    CHECK(si  == -123456);          CHECK(ui == 123456);
    CHECK(sl  == -123456);          CHECK(ul == 123456);
    CHECK(sll == -1234567891011);   CHECK(ull == 1234567891011);
    CHECK(f   ==  Catch::Approx(0.123456) .epsilon(1e-6));
    CHECK(d   ==  Catch::Approx(0.123456) .epsilon(1e-6));
    CHECK(ld  ==  Catch::Approx(0.123456) .epsilon(1e-6));
}

struct Student {
    std::string name = "default";
    int age = -1;
};

auto operator==(const Student &a, const Student &b) -> bool {
    return a.name == b.name && a.age == b.age;
}

auto operator<<(std::ostream &os, const Student &s) -> std::ostream & {
    return os << "Student(name:\"" << s.name << "\", age:" << s.age << ")";
}

auto studentFromString = [](const std::string& str, Student& out) -> bool {
    if (str == "1") {
        out = { .name = "Josh", .age = 1 }; return true;
    }
    if (str == "2") {
        out = { .name = "John", .age = 2 }; return true;
    }
    if (str == "3") {
        out = { .name = "Sally", .age = 3 }; return true;
    }
    return false;
};

auto studentError = [](const std::string& flag, const std::string& invalidArg) -> std::string {
    return std::format("Invalid value for flag '{}': expected either '1' or '2', got '{}'", flag, invalidArg);
};

TEST_CASE("Option with user defined type", "[options]") {
    Student josh, john, sally;
    auto parser = Option<Student>(&josh,  studentFromString, studentError)["--josh"]
                | Option<Student>(&john,  studentFromString, studentError)["--john"]
                | Option<Student>(&sally, studentFromString, studentError)["--sally"];

    const std::string input = "--josh 1 --john 2 --sally 3";
    parser.parseString(input);

    CHECK(josh  == Student{"Josh", 1});
    CHECK(john  == Student{"John", 2});
    CHECK(sally == Student{"Sally", 3});
}

TEST_CASE("Multioption test 1", "[options][multi]") {
    std::array<int, 3> intArr{};
    std::vector<double> doubleVec;

    Parser parser = MultiOption(&intArr)["-i"]["--ints"]
                  | MultiOption(&doubleVec)["-d"]["--doubles"];

    const std::string input = "--ints 1 2 3 --doubles 4.0 5.5 6.7";
    parser.parseString(input);

    CHECK(intArr[0] == 1);
    CHECK(intArr[1] == 2);
    CHECK(intArr[2] == 3);
    REQUIRE(doubleVec.size() == 3);
    CHECK(doubleVec[0] == 4.0);
    CHECK(doubleVec[1] == 5.5);
    CHECK(doubleVec[2] == 6.7);
}