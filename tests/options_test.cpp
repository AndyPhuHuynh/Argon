#include "Argon/Parser.hpp"
#include "Argon/Option.hpp"

#include "catch2/catch_approx.hpp"
#include "catch2/catch_test_macros.hpp"

using namespace Argon;

TEST_CASE("Basic option test case 1", "[options]") {
    unsigned int width;
    float height;
    double depth;
    int test;

    Option opt2 = Option(&height)["-h"]["--height"];
    Option opt3 = Option(&depth)["-d"]["--depth"];

    Parser parser = Option(&width)["-w"]["--width"] | opt2 | opt3 | Option(&test)["-t"]["--test"];
    const std::string input = "--width 100 --height 50.1 --depth 69.123456 -t 152";
    parser.parseString(input);

    CHECK(!parser.hasErrors());
    CHECK(width   == 100);
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

TEST_CASE("Basic option group" "[option-group]") {
    std::string name;
    int age;

    std::string major;
    std::string minor;

    auto parser = Option(&name)["--name"]
                | Option(&age)["--age"]
                | (
                    OptionGroup()["--degrees"]
                    + Option(&major)["--major"]
                    + Option(&minor)["--minor"]
                );

    const std::string input = "--name John --age 20 --degrees [--major CS --minor Music]";
    parser.parseString(input);

    CHECK(!parser.hasErrors());
    CHECK(name == "John");
    CHECK(age == 20);
    CHECK(major == "CS");
    CHECK(minor == "Music");
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
    std::string name;
    int age;
    float gpa;

    auto nameOption = Option(&name)["--name"];
    auto ageOption  = Option(&age)["--age"];
    auto gpaOption  = Option(&gpa)["--gpa"];

    auto parser = nameOption | ageOption | gpaOption;

    const std::string input = "--name John --age 0x14 --gpa 5.5";
    parser.parseString(input);

    const auto& nameOpt = nameOption.getValue();
    const auto& ageOpt  = ageOption.getValue();
    const auto& gpaOpt  = gpaOption.getValue();

    CHECK(nameOpt   .has_value());
    CHECK(ageOpt    .has_value());
    CHECK(gpaOpt    .has_value());

    CHECK(nameOpt.value()   == "John");
    CHECK(ageOpt.value()    == 20);
    CHECK(gpaOpt.value()    == Catch::Approx(5.5).epsilon(1e-6));
}

TEST_CASE("Parser getValue basic", "[options][getValue]") {
    std::string name;
    int age;
    float gpa;

    auto parser = Option(&name)["--name"]
                | Option(&age)["--age"]
                | Option(&gpa)["--gpa"];

    const std::string input = "--name John --age 0x14 --gpa 5.5";
    parser.parseString(input);

    auto nameOpt    = parser.getValue<std::string>  ("--name");
    auto ageOpt     = parser.getValue<int>          ("--age");
    auto gpaOpt     = parser.getValue<float>        ("--gpa");

    CHECK(nameOpt   .has_value());
    CHECK(ageOpt    .has_value());
    CHECK(gpaOpt    .has_value());

    CHECK(nameOpt.value()   == "John");
    CHECK(ageOpt.value()    == 20);
    CHECK(gpaOpt.value()    == Catch::Approx(5.5).epsilon(1e-6));
}

TEST_CASE("Parser getValue nested", "[options][getValue][option-group]") {
    std::string one;
    std::string two;
    std::string three;
    std::string four;

    auto parser = Option(&one)["--one"]
                | (
                    OptionGroup()["--g1"]
                    + Option(&two)["--two"]
                    + (
                        OptionGroup()["--g2"]
                        + Option(&three)["--three"]
                        + (
                            OptionGroup()["--g3"]
                            + Option(&four)["--four"]
                        )
                    )
                );

    const std::string input = "--one 1 --g1 [--two 2 --g2 [--three 3 --g3 [--four 4]]]";
    parser.parseString(input);


    const auto oneOpt   = parser.getValue<std::string>("--one");
    const auto twoOpt   = parser.getValue<std::string>("--g1", "--two");
    const auto threeOpt = parser.getValue<std::string>("--g1", "--g2", "--three");
    const auto fourOpt  = parser.getValue<std::string>("--g1", "--g2", "--g3", "--four");

    CHECK(!parser.hasErrors());

    REQUIRE(oneOpt      .has_value());
    REQUIRE(twoOpt      .has_value());
    REQUIRE(threeOpt    .has_value());
    REQUIRE(fourOpt     .has_value());

    CHECK(oneOpt.value()    == "1");
    CHECK(twoOpt.value()    == "2");
    CHECK(threeOpt.value()  == "3");
    CHECK(fourOpt.value()   == "4");
}

TEST_CASE("Parser getValue multioption", "[options][multi][getValue]") {
    std::array<int, 3> ints{};
    std::vector<float> floats{};

    auto parser = MultiOption(&ints)["--ints"]
                | MultiOption(&floats)["--floats"];

    const std::string input = "--ints 1 2 3 --floats 1.5 2.5 3.5";
    parser.parseString(input);

    const auto intsOpt = parser.getMultiValue<std::array<int, 3>>("--ints");
    const auto floatsOpt = parser.getMultiValue<std::vector<float>>("--floats");

    CHECK(!parser.hasErrors());

    REQUIRE(intsOpt     .has_value());
    REQUIRE(floatsOpt   .has_value());

    CHECK(ints[0] == 1);
    CHECK(ints[1] == 2);
    CHECK(ints[2] == 3);

    REQUIRE(floats.size() == 3);
    CHECK(floats[0] == Catch::Approx(1.5).epsilon(1e-6));
    CHECK(floats[1] == Catch::Approx(2.5).epsilon(1e-6));
    CHECK(floats[2] == Catch::Approx(3.5).epsilon(1e-6));

}

TEST_CASE("Parser getValue multioption nested", "[options][multi][getValue][option-group]") {
    using namespace Argon;

    std::array<int,   4> one{};
    std::array<float, 3> two{};
    std::vector<float> three{};
    std::vector<float>  four{};

    auto parser = MultiOption(&one)["--one"]
                | (
                    OptionGroup()["--g1"]
                    + MultiOption(&two)["--two"]
                    + (
                        OptionGroup()["--g2"]
                        + MultiOption(&three)["--three"]
                        + (
                            OptionGroup()["--g3"]
                            + MultiOption(&four)["--four"]
                        )
                    )
                );

    const std::string input = "--one 1 10 100 1000 --g1 [--two 2.0 2.2 2.3 --g2 [--three 1.5 2.5 --g3 [--four 4.5 5.5 6.5 7.5 8.5]]]";
    parser.parseString(input);


    const auto oneOpt   = parser.getMultiValue<std::array<int, 4>>      ("--one");
    const auto twoOpt   = parser.getMultiValue<std::array<float ,3>>    ("--g1", "--two");
    const auto threeOpt = parser.getMultiValue<std::vector<float>>      ("--g1", "--g2", "--three");
    const auto fourOpt  = parser.getMultiValue<std::vector<float>>      ("--g1", "--g2", "--g3", "--four");

    CHECK(!parser.hasErrors());

    REQUIRE(oneOpt    .has_value());
    REQUIRE(twoOpt    .has_value());
    REQUIRE(threeOpt  .has_value());
    REQUIRE(fourOpt   .has_value());

    CHECK(oneOpt.value()[0] == 1);
    CHECK(oneOpt.value()[1] == 10);
    CHECK(oneOpt.value()[2] == 100);
    CHECK(oneOpt.value()[3] == 1000);

    CHECK(twoOpt.value()[0] == Catch::Approx(2.0).epsilon(1e-6));
    CHECK(twoOpt.value()[1] == Catch::Approx(2.2).epsilon(1e-6));
    CHECK(twoOpt.value()[2] == Catch::Approx(2.3).epsilon(1e-6));

    REQUIRE(threeOpt.value().size() == 2);
    CHECK(threeOpt.value()[0] == Catch::Approx(1.5).epsilon(1e-6));
    CHECK(threeOpt.value()[1] == Catch::Approx(2.5).epsilon(1e-6));

    REQUIRE(fourOpt.value().size() == 5);
    CHECK(fourOpt.value()[0] == Catch::Approx(4.5).epsilon(1e-6));
    CHECK(fourOpt.value()[1] == Catch::Approx(5.5).epsilon(1e-6));
    CHECK(fourOpt.value()[2] == Catch::Approx(6.5).epsilon(1e-6));
    CHECK(fourOpt.value()[3] == Catch::Approx(7.5).epsilon(1e-6));
    CHECK(fourOpt.value()[4] == Catch::Approx(8.5).epsilon(1e-6));
}