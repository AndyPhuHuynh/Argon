#include "catch2/catch_test_macros.hpp"

#include "Argon/Parser.hpp"
using namespace Argon;

TEST_CASE("Attributes test 1", "[attributes]") {
    Constraints c;

    c.require(FlagPath("hello"));
    c.require(FlagPath("world"));
    c.require(FlagPath("hello"));
    c.require(FlagPath("hello"));

    c.mutuallyExclusive(FlagPath("hello"), {FlagPath("world"), FlagPath("what!")});
    c.mutuallyExclusive(FlagPath("hello"), {FlagPath("world"), FlagPath("add2")});
    c.mutuallyExclusive(FlagPath("world"), {FlagPath("world"), FlagPath("add3")});
}

TEST_CASE() {
    auto parser = Option<int>()["-x"]["--x2"]
                | Option<int>()["-y"]
                | Option<int>()["-z"]
                | MultiOption<std::array<int, 3>>()["-w"]
                | (
                    OptionGroup()["-g"]["--group"]
                    + Option<int>()["-a"]
                    + Option<int>()["-b"]
                    + Option<int>()["-c"]
                );

    parser.constraints()
        .require({"-x"})
        // .mutuallyExclusive({"-x"}, {{"-y"}, {"-g", "-a"}})
        .dependsOn({"-x"}, {{"-y"}, {"--group", "-a"}})
        // .dependsOn({"-g", "-a"}, {{"-x"}, {"-y"}})
    ;
    // c.mutuallyExclusive("-x", {"-y", "-z"});
    parser.parse("-x 10 -z 30 -w 1 2 3");
    // parser.parse("-x 10");

    // auto v = parser.getValue<float>(FlagPath{"--what", "--the", "--heck"});
    // auto v = parser.getValue<float>(FlagPath{"--what"});

    // const char *argv[] = {"argontest.exe", "-x", "10", "-y", "20", "-z", "30"};
    // parser.parse(7, argv);

    // const std::string input = "-x 10 -y 20 -g [-a 40 -b 50]";
    // parser.parse(input);

    // if (parser.hasErrors()) {
    //     parser.printErrors(PrintMode::Flat);
    // }
}

TEST_CASE("Duplicate flags") {
    auto parser = Option<int>()["-x"]["--canonical"]
                | (
                    OptionGroup()["--group"]["-g"]
                    + Option<float>()["-x"]
                    + Option<float>()["-x"]
                    + (
                        OptionGroup()["-g2"]
                        + Option<std::string>()["-y"]
                        + Option<bool>()["-y"]
                    )
                )
                | Option<int>()["-x"]
                | Option<int>()["-y"]["--group"]
                | Option<int>()["-g2"]["--group2"];
    parser.parse("-c asdf  -g [-x asdf]");
    // if (parser.hasErrors()) {
    //     parser.printErrors(PrintMode::Tree);
    // }
}

TEST_CASE() {
    auto parser = Option<int>()["-x"]
                | Option<int>()["-y"]
                | (
                    OptionGroup()["-g"]["--group"]
                );

    parser.parse("-x 10 -g [] = -y 20");
    // if (parser.hasErrors()) {
    //     parser.printErrors(PrintMode::Tree);
    // }
    //
    // std::cout << "Y: " << parser.getValue<int>("-y") << "\n";
}

TEST_CASE("Add default dashes") {
    int x, y;
    auto parser = Option(&x)["main"]["m"]
                | Option(&y)["--main2"]["-m2"];

    parser.parse("--main 1 -m 2 --main2 3 -m2 4");

    CHECK(!parser.hasErrors());
    parser.printErrors(PrintMode::Flat);
    CHECK(x == 2);
    CHECK(y == 4);
}

TEST_CASE("Help message") {
    const auto parser = Option<int>()["--xcoord"]["x"].description("X coordinate")
                      | Option<int>()["--ycoord"]["y"].description("Y coordinate")
                      | Option<int>()["--zcoord"]["z"].description("Z coordinate");
    const auto msg = parser.getHelpMessage();
    std::cout << msg;
}