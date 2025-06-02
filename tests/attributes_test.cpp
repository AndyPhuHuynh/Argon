#include "Argon/Attributes.hpp"

#include "catch2/catch_test_macros.hpp"

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