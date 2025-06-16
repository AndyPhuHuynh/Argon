#include "Argon/Error.hpp"
#include "Argon/Option.hpp"
#include "Argon/Parser.hpp"
#include "catch2/catch_test_macros.hpp"

static void MessageTest1() {
    using namespace Argon;
    ErrorGroup root;

    root.addErrorMessage("one", 1);
    root.addErrorMessage("three", 3);
    root.addErrorMessage("two", 2);

    const auto errors = root.getErrors();
    // check(success, "Error Messages Test 1");
}

static void GroupTest1() {
    using namespace Argon;
    ErrorGroup root;

    root.addErrorMessage("one", 1);
    root.addErrorMessage("25", 25);
    root.addErrorMessage("21", 21);
    root.addErrorMessage("35", 35);
}

static void SyntaxError1() {
    using namespace Argon;
    int test = 0;
    auto parser = static_cast<Parser>(Option(&test)["--test"]);

    const std::string input = "[[[[[[[[[ --test ] 2";
    parser.parse(input);

    std::cout << "Test: " << test << "\n";
}

void runErrorTests() {
    MessageTest1();
    GroupTest1();
    SyntaxError1();
}

TEST_CASE("Errors test 1", "[errors]") {
    using namespace Argon;
    auto parser = Option<int>()["--integer"]
                | Option<bool>()["--bool"]
                | (
                    OptionGroup()["--g"]
                    + Option<int>()["--integer"]
                    + Option<bool>()["--bool"]
                    + (
                    OptionGroup()["--g2"]
                    + Option<int>()["--nestedint"]
                    + Option<bool>()["--nestedbool"]
                )
                );
    const std::string input = "--integer asdf --g [--integer what --g2 [--nestedbool bool --nestedint int] --bool dddd]  --bool 2";
    parser.parse(input);
    if (parser.hasErrors()) {
        // parser.printErrors();
    }
}