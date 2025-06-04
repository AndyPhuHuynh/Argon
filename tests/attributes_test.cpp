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
    auto parser = Option<int>()["-x"]
                | Option<int>()["-y"]
                | Option<int>()["-z"];

    parser.constraints()
        .require({"-x"})
        .dependsOn({"-x"}, {{"-y"}, {"-z"}});

    // c.mutuallyExclusive("-x", {"-y", "-z"});
    parser.parse("-x 10 -z 30");
    // parser.parse("-x 10");

    // auto v = parser.getValue<float>(FlagPath{"--what", "--the", "--heck"});
    // auto v = parser.getValue<float>(FlagPath{"--what"});

    // const char *argv[] = {"argontest.exe", "-x", "10", "-y", "20", "-z", "30"};
    // parser.parse(7, argv);

    if (parser.hasErrors()) {
        parser.printErrors(PrintMode::Flat);
    }
}