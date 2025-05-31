#include "Argon/Parser.hpp"
#include "Argon/Option.hpp"

#include "catch2/catch_approx.hpp"
#include "catch2/catch_test_macros.hpp"

using namespace Argon;

TEST_CASE("Basic option", "[options]") {
    unsigned int width = 2;
    float height = 2;
    double depth = 2;
    int test = 2;

    Option opt2 = Option(&height)["-h"]["--height"];
    Option opt3 = Option(&depth)["-d"]["--depth"];

    Parser parser = Option(&width)["-w"]["--width"] | opt2 | opt3 | Option(&test)["-t"]["--test"];

    SECTION("Input provided") {
        const std::string input = "--width 100 --height 50.1 --depth 69.123456 -t 152";
        parser.parseString(input);

        CHECK(!parser.hasErrors());
        CHECK(width   == 100);
        CHECK(height  == Catch::Approx(50.1)      .epsilon(1e-6));
        CHECK(depth   == Catch::Approx(69.123456) .epsilon(1e-6));
        CHECK(test    == 152);
    }

    SECTION("No input provided") {
        const std::string input;
        parser.parseString(input);

        CHECK(!parser.hasErrors());
        CHECK(width   == 2);
        CHECK(height  == Catch::Approx(2).epsilon(1e-6));
        CHECK(depth   == Catch::Approx(2).epsilon(1e-6));
        CHECK(test    == 2);
    }
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

    CHECK(!parser.hasErrors());
    CHECK(name == "John");
    CHECK(age == 20);
    CHECK(major == "CS");
}

TEST_CASE("Option all built-in numeric types", "[options]") {
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

    SECTION("Booleans") {
        const std::string input = "-fb   false          -tb  true ";
        parser.parseString(input);
        CHECK(fb  == false); CHECK(tb == true);
    }

    SECTION("Base 10") {
        const std::string input = "-sc  -10             -uc  10                 -c a "
                                  "-ss  -300            -us  300 "
                                  "-si  -123456         -ui  123456 "
                                  "-sl  -123456         -ul  123456 "
                                  "-sll -1234567891011  -ull 1234567891011 "
                                  "-f    0.123456       -d   0.123456           -ld 0.123456 ";
        parser.parseString(input);

        CHECK(!parser.hasErrors());
        CHECK(sc  == -10);              CHECK(uc  == 10);           CHECK(c == 'a');
        CHECK(ss  == -300);             CHECK(us  == 300);
        CHECK(si  == -123456);          CHECK(ui  == 123456);
        CHECK(sl  == -123456);          CHECK(ul  == 123456);
        CHECK(sll == -1234567891011);   CHECK(ull == 1234567891011);
        CHECK(f   ==  Catch::Approx(0.123456) .epsilon(1e-6));
        CHECK(d   ==  Catch::Approx(0.123456) .epsilon(1e-6));
        CHECK(ld  ==  Catch::Approx(0.123456) .epsilon(1e-6));
    }

    SECTION("Hexadecimal") {
        const std::string input = "-sc  -0x1            -uc  0x1 "
                                  "-ss  -0x123          -us  0x123 "
                                  "-si  -0x12345        -ui  0x12345 "
                                  "-sl  -0x12345        -ul  0x12345 "
                                  "-sll -0x123456789    -ull 0x123456789 "
                                  "-f   -0x1.5p1        -d   0x1.5p1        -ld 0x1.5p1 ";
        parser.parseString(input);
        parser.printErrors(PrintMode::Tree);
        CHECK(!parser.hasErrors());
        CHECK(sc  == -0x1);             CHECK(uc  == 0x1);
        CHECK(ss  == -0x123);           CHECK(us  == 0x123);
        CHECK(si  == -0x12345);         CHECK(ui  == 0x12345);
        CHECK(sl  == -0x12345);         CHECK(ul  == 0x12345);
        CHECK(sll == -0x123456789);     CHECK(ull == 0x123456789);
        CHECK(f   ==  Catch::Approx(-0x1.5p1) .epsilon(1e-6));
        CHECK(d   ==  Catch::Approx( 0x1.5p1) .epsilon(1e-6));
        CHECK(ld  ==  Catch::Approx( 0x1.5p1) .epsilon(1e-6));
    }

    SECTION("Binary") {
        const std::string input = "-sc  -0b1        -uc  0b1 "
                                  "-ss  -0b11       -us  0b11 "
                                  "-si  -0b111      -ui  0b111 "
                                  "-sl  -0b1111     -ul  0b1111 "
                                  "-sll -0b11111    -ull 0b11111 ";
        parser.parseString(input);
        parser.printErrors(PrintMode::Tree);
        CHECK(!parser.hasErrors());
        CHECK(sc  == -0b1);         CHECK(uc  == 0b1);
        CHECK(ss  == -0b11);        CHECK(us  == 0b11);
        CHECK(si  == -0b111);       CHECK(ui  == 0b111);
        CHECK(sl  == -0b1111);      CHECK(ul  == 0b1111);
        CHECK(sll == -0b11111);     CHECK(ull == 0b11111);
    }
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

    CHECK(!parser.hasErrors());
    CHECK(josh  == Student{"Josh", 1});
    CHECK(john  == Student{"John", 2});
    CHECK(sally == Student{"Sally", 3});
}

TEST_CASE("Basic option group", "[option-group]") {
    std::string name = "default";
    int age = -1;

    std::string major = "default";
    std::string minor = "default";

    auto parser = Option(&name)["--name"]
                | Option(&age)["--age"]
                | (
                    OptionGroup()["--degrees"]
                    + Option(&major)["--major"]
                    + Option(&minor)["--minor"]
                );

    SECTION("Input provided") {
        const std::string input = "--name John --age 20 --degrees [--major CS --minor Music]";
        parser.parseString(input);

        CHECK(!parser.hasErrors());
        CHECK(name == "John");
        CHECK(age == 20);
        CHECK(major == "CS");
        CHECK(minor == "Music");
    }

    SECTION("No input provided") {
        const std::string input;
        parser.parseString(input);

        CHECK(!parser.hasErrors());
        CHECK(name == "default");
        CHECK(age == -1);
        CHECK(major == "default");
        CHECK(minor == "default");
    }
}

TEST_CASE("Nested option groups", "[option-group]") {
    std::string name;
    int age;

    std::string major;
    std::string minor;

    std::string main;
    std::string side;

    auto parser = Option(&name)["--name"]
                | Option(&age)["--age"]
                | (
                    OptionGroup()["--degrees"]
                    + Option(&major)["--major"]
                    + Option(&minor)["--minor"]
                    + (
                        OptionGroup()["--instruments"]
                        + Option(&main)["--main"]
                        + Option(&side)["--side"]
                    )
                );

    const std::string input = "--name John --age 20 --degrees [--major CS --instruments [--main piano --side drums] --minor Music]";
    parser.parseString(input);

    CHECK(!parser.hasErrors());
    CHECK(name  == "John");
    CHECK(age   == 20);
    CHECK(major == "CS");
    CHECK(minor == "Music");
    CHECK(main  == "piano");
    CHECK(side  == "drums");
}

TEST_CASE("Multioption test 1", "[options][multi]") {
    std::array<int, 3> intArr{};
    std::vector<double> doubleVec;

    Parser parser = MultiOption(&intArr)["-i"]["--ints"]
                  | MultiOption(&doubleVec)["-d"]["--doubles"];

    const std::string input = "--ints 1 2 3 --doubles 4.0 5.5 6.7";
    parser.parseString(input);

    CHECK(!parser.hasErrors());
    CHECK(intArr[0] == 1);
    CHECK(intArr[1] == 2);
    CHECK(intArr[2] == 3);
    REQUIRE(doubleVec.size() == 3);
    CHECK(doubleVec[0] == 4.0);
    CHECK(doubleVec[1] == 5.5);
    CHECK(doubleVec[2] == 6.7);
}

TEST_CASE("Multioption inside group", "[options][multi][option-group]") {
    std::array<int, 3> intArr{};
    std::vector<double> doubleVec;

    Parser parser = MultiOption(&intArr)["-i"]["--ints"]
                    | (
                        OptionGroup()["--group"]
                        + MultiOption(&doubleVec)["-d"]["--doubles"]
                    );

    const std::string input = "--ints 1 2 3 --group [--doubles 4.0 5.5 6.7]";
    parser.parseString(input);

    CHECK(!parser.hasErrors());
    CHECK(intArr[0] == 1);
    CHECK(intArr[1] == 2);
    CHECK(intArr[2] == 3);
    REQUIRE(doubleVec.size() == 3);
    CHECK(doubleVec[0] == 4.0);
    CHECK(doubleVec[1] == 5.5);
    CHECK(doubleVec[2] == 6.7);
}

TEST_CASE("Options getValue lvalue", "[options][getValue]") {
    auto nameOption = Option<std::string>()["--name"];
    auto ageOption  = Option<int>()["--age"];
    auto gpaOption  = Option<float>()["--gpa"];

    auto parser = nameOption | ageOption | gpaOption;

    const std::string input = "--name John --age 0x14 --gpa 5.5";
    parser.parseString(input);

    const auto& name    = nameOption.getValue();
    const auto& age     = ageOption.getValue();
    const auto& gpa     = gpaOption.getValue();

    CHECK(name          == "John");
    CHECK(age           == 20);
    CHECK(gpa           == Catch::Approx(5.5).epsilon(1e-6));
}

TEST_CASE("Parser getValue basic", "[options][getValue]") {
    auto parser = Option<std::string>()["--name"]
                | Option<int>()["--age"]
                | Option<float>()["--gpa"];

    const std::string input = "--name John --age 0x14 --gpa 5.5";
    parser.parseString(input);

    const auto& name    = parser.getValue<std::string>  ("--name");
    const auto& age     = parser.getValue<int>          ("--age");
    const auto& gpa     = parser.getValue<float>        ("--gpa");

    CHECK(name          == "John");
    CHECK(age           == 20);
    CHECK(gpa           == Catch::Approx(5.5).epsilon(1e-6));
}

TEST_CASE("Parser getValue nested", "[options][getValue][option-group]") {
    auto parser = Option<std::string>()["--one"]
                | (
                    OptionGroup()["--g1"]
                    + Option<std::string>()["--two"]
                    + (
                        OptionGroup()["--g2"]
                        + Option<std::string>()["--three"]
                        + (
                            OptionGroup()["--g3"]
                            + Option<std::string>()["--four"]
                        )
                    )
                );

    const std::string input = "--one 1 --g1 [--two 2 --g2 [--three 3 --g3 [--four 4]]]";
    parser.parseString(input);

    const auto& one      = parser.getValue<std::string>("--one");
    const auto& two      = parser.getValue<std::string>("--g1", "--two");
    const auto& three    = parser.getValue<std::string>("--g1", "--g2", "--three");
    const auto& four     = parser.getValue<std::string>("--g1", "--g2", "--g3", "--four");

    CHECK(!parser.hasErrors());

    CHECK(one   == "1");
    CHECK(two   == "2");
    CHECK(three == "3");
    CHECK(four  == "4");
}

TEST_CASE("Parser getValue multioption", "[options][multi][getValue]") {
    auto parser = MultiOption<std::array<int, 3>>()["--ints"]
                | MultiOption<std::vector<float>>()["--floats"];

    const std::string input = "--ints 1 2 3 --floats 1.5 2.5 3.5";
    parser.parseString(input);

    const auto& ints = parser.getMultiValue<std::array<int, 3>>("--ints");
    const auto& floats = parser.getMultiValue<std::vector<float>>("--floats");

    CHECK(!parser.hasErrors());

    CHECK(ints[0] == 1);
    CHECK(ints[1] == 2);
    CHECK(ints[2] == 3);

    REQUIRE(floats.size() == 3);
    CHECK(floats[0] == Catch::Approx(1.5).epsilon(1e-6));
    CHECK(floats[1] == Catch::Approx(2.5).epsilon(1e-6));
    CHECK(floats[2] == Catch::Approx(3.5).epsilon(1e-6));

}

TEST_CASE("Parser getValue multioption nested", "[options][multi][getValue][option-group]") {
    auto parser = MultiOption<std::array<int, 4>>()["--one"]
                | (
                    OptionGroup()["--g1"]
                    + MultiOption<std::array<float, 3>>()["--two"]
                    + (
                        OptionGroup()["--g2"]
                        + MultiOption<std::vector<float>>()["--three"]
                        + (
                            OptionGroup()["--g3"]
                            + MultiOption<std::vector<float>>()["--four"]
                        )
                    )
                );

    const std::string input = "--one 1 10 100 1000 --g1 [--two 2.0 2.2 2.3 --g2 [--three 1.5 2.5 --g3 [--four 4.5 5.5 6.5 7.5 8.5]]]";
    parser.parseString(input);


    const auto& one     = parser.getMultiValue<std::array<int, 4>>      ("--one");
    const auto& two     = parser.getMultiValue<std::array<float ,3>>    ("--g1", "--two");
    const auto& three   = parser.getMultiValue<std::vector<float>>      ("--g1", "--g2", "--three");
    const auto& four    = parser.getMultiValue<std::vector<float>>      ("--g1", "--g2", "--g3", "--four");

    CHECK(one[0] == 1);
    CHECK(one[1] == 10);
    CHECK(one[2] == 100);
    CHECK(one[3] == 1000);

    CHECK(two[0] == Catch::Approx(2.0).epsilon(1e-6));
    CHECK(two[1] == Catch::Approx(2.2).epsilon(1e-6));
    CHECK(two[2] == Catch::Approx(2.3).epsilon(1e-6));

    REQUIRE(three.size() == 2);
    CHECK(three[0] == Catch::Approx(1.5).epsilon(1e-6));
    CHECK(three[1] == Catch::Approx(2.5).epsilon(1e-6));

    REQUIRE(four.size() == 5);
    CHECK(four[0] == Catch::Approx(4.5).epsilon(1e-6));
    CHECK(four[1] == Catch::Approx(5.5).epsilon(1e-6));
    CHECK(four[2] == Catch::Approx(6.5).epsilon(1e-6));
    CHECK(four[3] == Catch::Approx(7.5).epsilon(1e-6));
    CHECK(four[4] == Catch::Approx(8.5).epsilon(1e-6));
}