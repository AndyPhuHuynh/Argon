#include "catch2/catch_test_macros.hpp"

#include "Argon/Parser.hpp"
using namespace Argon;

TEST_CASE("Attributes test 1", "[attributes]") {
    Constraints c;

    c.require("hello");
    c.require("world");
    c.require("hello");
    c.require("hello");

    c.mutuallyExclusive("hello", {"world", "what!"});
    c.mutuallyExclusive("hello", {"world", "add2"});
    c.mutuallyExclusive("world", {"world", "add3"});
}

TEST_CASE() {
    auto parser = Option<int>()["-x"]
                | Option<int>()["-y"]
                | Option<int>()["-z"];

    parser.constraints()
        .require("-x")
        .dependsOn("-x", {"-y", "-z"});

    // c.mutuallyExclusive("-x", {"-y", "-z"});
    // parser.parseString("-x 10 -y 20 -z 30");
    // parser.parse("-x 10");

    // auto v = parser.getValue<float>(FlagPath{"--what", "--the", "--heck"});
    // auto v = parser.getValue<float>(FlagPath{"--what"});

    const char *argv[] = {"argontest.exe", "-x", "10", "-y", "20", "-z", "30"};
    parser.parse(7, argv);

    if (parser.hasErrors()) {
        parser.printErrors(PrintMode::Flat);
    }
}