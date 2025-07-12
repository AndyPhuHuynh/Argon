#include "Argon/Error.hpp"
#include "Argon/Options/Option.hpp"
#include "Argon/Parser.hpp"

#include "ErrorTestFunctions.hpp"

using namespace Argon;

TEST_CASE("Error message basic sorting", "[errors]") {
    using namespace Argon;

    ErrorGroup root;
    root.addErrorMessage("zero",  0, ErrorType::None);
    root.addErrorMessage("one",   1, ErrorType::None);
    root.addErrorMessage("four",  4, ErrorType::None);
    root.addErrorMessage("two",   2, ErrorType::None);
    root.addErrorMessage("five",  5, ErrorType::None);
    root.addErrorMessage("three", 3, ErrorType::None);
    root.addErrorMessage("six",   6, ErrorType::None);
    root.addErrorMessage("nine",  9, ErrorType::None);
    root.addErrorMessage("eight", 8, ErrorType::None);
    root.addErrorMessage("seven", 7, ErrorType::None);

    const auto& errors = root.getErrors();

    REQUIRE(errors.size() == 10);
    for (size_t i = 0; i < errors.size(); ++i) {
        const auto& msg = RequireMsg(errors[i]);
        CheckMessage(msg,
            DigitToString(static_cast<int>(i)),
            static_cast<int>(i),
            ErrorType::None);
    }
}

TEST_CASE("Error group insertion", "[errors]") {
    using namespace Argon;

    ErrorGroup root;
    root.addErrorGroup("Group one", 10, 20);
    root.addErrorMessage("one", 1, ErrorType::Syntax_MissingFlagName);
    root.addErrorGroup("Group inside one", 15, 17);
    root.addErrorMessage("sixteen", 16, ErrorType::Syntax_MissingValue);
    root.addErrorGroup("Group two", 100, 200);
    root.addErrorMessage("150", 150, ErrorType::Analysis_UnknownFlag);

    const auto& errors = root.getErrors();
    REQUIRE(errors.size() == 3);

    // ErrorMsg one
    CheckMessage(RequireMsg(errors[0]), "one", 1, ErrorType::Syntax_MissingFlagName);

    // Group one
    const auto& groupOne = RequireGroup(errors[1]);
    CheckGroup(groupOne, "Group one", 10, 20, 1);
    const auto& groupOneErrors = groupOne.getErrors();

        // Group inside one
        const auto& groupInsideOne = RequireGroup(groupOneErrors[0]);
        CheckGroup(groupInsideOne, "Group inside one", 15, 17, 1);
        // Sixteen
        CheckMessage(RequireMsg(groupInsideOne.getErrors()[0]), "sixteen", 16, ErrorType::Syntax_MissingValue);

    // Group two
    const auto& groupTwo = RequireGroup(errors[2]);
    CheckGroup(groupTwo, "Group two", 100, 200, 1);
    CheckMessage(RequireMsg(groupTwo.getErrors()[0]), "150", 150, ErrorType::Analysis_UnknownFlag);
}

TEST_CASE("Error group correctly encompasses errors inside its range", "[errors]") {
    using namespace Argon;

    ErrorGroup root;

    root.addErrorMessage("1", 1, ErrorType::None);
    root.addErrorMessage("2", 2, ErrorType::None);
    root.addErrorGroup("Group one", 0, 9);
    root.addErrorMessage("11", 11, ErrorType::None);
    root.addErrorMessage("12", 12, ErrorType::None);
    root.addErrorGroup("Group two", 10, 20);
    root.addErrorGroup("Outer group", -1, 100);

    // Root
    const auto& errors = root.getErrors();
    CheckGroup(root, "", -1, -1, 1);

    // Outer group
    const auto& outerGroup = RequireGroup(errors[0]);
    CheckGroup(outerGroup, "Outer group", -1, 100, 2);
    const auto& outerGroupErrors = outerGroup.getErrors();

        // Group one
        const auto& groupOne = RequireGroup(outerGroupErrors[0]);
        CheckGroup(groupOne, "Group one", 0, 9, 2);
        CheckMessage(RequireMsg(groupOne.getErrors()[0]), "1", 1, ErrorType::None);
        CheckMessage(RequireMsg(groupOne.getErrors()[1]), "2", 2, ErrorType::None);

        // Group two
        const auto& groupTwo = RequireGroup(outerGroupErrors[1]);
        CheckGroup(groupTwo, "Group two", 10, 20, 2);
        CheckMessage(RequireMsg(groupTwo.getErrors()[0]), "11", 11, ErrorType::None);
        CheckMessage(RequireMsg(groupTwo.getErrors()[1]), "12", 12, ErrorType::None);
}

TEST_CASE("Option group syntax errors", "[option-group][syntax][errors]") {
    using namespace Argon;
    using namespace Catch::Matchers;

    std::string name;
    int age;
    std::string major;

    auto parser = Option(&name)["--name"]
                | (
                    OptionGroup()["--group"]
                    + Option(&age)["--age"]
                ) | (
                    OptionGroup()["--group2"]
                    + Option(&age)["--age"]
                );

    SECTION("Missing flag for group name") {
        parser.parse("--name [--age 10]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 1);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            7, ErrorType::Syntax_MissingValue);
    }

    SECTION("Unknown flag") {
        parser.parse("--name John --huh [--age 20]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 1);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            12, ErrorType::Syntax_UnknownFlag);
    }

    SECTION("Missing left bracket") {
        parser.parse("--name John --group --age 20]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 2);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            20, ErrorType::Syntax_MissingLeftBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]),
            28, ErrorType::Syntax_MissingLeftBracket);
    }

    SECTION("Missing right bracket") {
        parser.parse("--name John --group [--age 20 --major CS");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 2);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            30, ErrorType::Syntax_UnknownFlag);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]),
            40, ErrorType::Syntax_MissingRightBracket);
    }

    SECTION("Same level groups missing lbrack") {
        parser.parse("--name John --group --age 20] --group2 --age 21]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 4);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            20, ErrorType::Syntax_MissingLeftBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]),
            28, ErrorType::Syntax_MissingLeftBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]),
            39, ErrorType::Syntax_MissingLeftBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[3]),
            47, ErrorType::Syntax_MissingLeftBracket);
    }

    SECTION("Same level groups missing rbrack") {
        parser.parse("--name John --group [--age 20 --group2 [--age 21");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 3);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            30, ErrorType::Syntax_UnknownFlag);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]),
            39, ErrorType::Syntax_MissingRightBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]),
            48, ErrorType::Syntax_MissingRightBracket);
    }

    SECTION("Same level groups missing lbrack and then rbrack") {
        parser.parse("--name John --group --age 20] --group2 [--age 21");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 3);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            20, ErrorType::Syntax_MissingLeftBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]),
            28, ErrorType::Syntax_MissingLeftBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]),
            48, ErrorType::Syntax_MissingRightBracket);
    }

    SECTION("Same level groups missing rbrack and then lbrack") {
        parser.parse("--name John --group [--age 20 --group2 --age 21]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 1);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            30, ErrorType::Syntax_UnknownFlag);
    }

    CHECK(!parser.getAnalysisErrors().hasErrors());
}

TEST_CASE("Option group nested syntax errors", "[option-group][syntax][errors]") {
    using namespace Argon;
    using namespace Catch::Matchers;

    std::string name;
    int age;
    std::string major;

    auto parser = Option(&name)["--name"]
                | (
                    OptionGroup{}["--group"]
                    + Option(&age)["--age"]
                    + (
                        OptionGroup{}["--classes"]
                        + Option(&major)["--major"]
                    ) + (
                        OptionGroup{}["--classes2"]
                        + Option(&major)["--major"]
                    )
                );

    SECTION("Missing flag for outer group name") {
        parser.parse("--name [--age 10 --classes [--major Music]]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 1);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            7, ErrorType::Syntax_MissingValue);
    }

    SECTION("Missing flag for inner group name") {
        parser.parse("--name --group [--age [--major Music]]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 2);

        const auto& noValueForName   = RequireMsg(syntaxErrors.getErrors()[0]);
        const auto& noValueForAge    = RequireMsg(syntaxErrors.getErrors()[1]);

        CheckMessage(noValueForName, 7,  ErrorType::Syntax_MissingValue);
        CheckMessage(noValueForAge,  22, ErrorType::Syntax_MissingValue);
    }

    SECTION("Unknown flag") {
        parser.parse("--huh John --group [--huh 20]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 2);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            0,  ErrorType::Syntax_UnknownFlag);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]),
            20, ErrorType::Syntax_UnknownFlag);
    }

    SECTION("Missing left bracket") {
        parser.parse("--name John --group [--age 20 --classes --major Music]]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 2);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            40, ErrorType::Syntax_MissingLeftBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]),
            54, ErrorType::Syntax_MissingLeftBracket);
    }

    SECTION("Missing right bracket") {
        parser.parse("--name John --group [--age 20 --classes [--major Music");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 2);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            54, ErrorType::Syntax_MissingRightBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]),
            54, ErrorType::Syntax_MissingRightBracket);
    }

    SECTION("Same level groups missing lbrack") {
        parser.parse("--name John --group [--classes --major Music] --classes2 --major CS]]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 4);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            31, ErrorType::Syntax_MissingLeftBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]),
            46, ErrorType::Syntax_UnknownFlag);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]),
            67, ErrorType::Syntax_MissingLeftBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[3]),
            68, ErrorType::Syntax_MissingLeftBracket);
    }

    SECTION("Same level groups missing rbrack") {
        parser.parse("--name John --group [--classes [--major Music --classes2 [--major CS]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 3);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            46, ErrorType::Syntax_UnknownFlag);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]),
            69, ErrorType::Syntax_MissingRightBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]),
            69, ErrorType::Syntax_MissingRightBracket);
    }

    SECTION("Same level groups missing lbrack and then rbrack") {
        parser.parse("--name John --group [--classes --major Music] --classes2 [--major CS]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 2);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            31, ErrorType::Syntax_MissingLeftBracket);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]),
            46, ErrorType::Syntax_UnknownFlag);
    }

    SECTION("Same level groups missing rbrack and then lbrack") {
        parser.parse("--name John --group [--classes [--major Music --classes2 --major CS]]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = parser.getSyntaxErrors();
        CheckGroup(syntaxErrors, "Syntax Errors", -1, -1, 1);

        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]),
            46, ErrorType::Syntax_UnknownFlag);
    }

    CHECK(!parser.getAnalysisErrors().hasErrors());
}

TEST_CASE("Missing value for flags", "[option][syntax][errors]") {
    using namespace Argon;
    int i; float f; std::string s;
    auto parser = Option(&i)["--int"]
                | Option(&f)["--float"]
                | Option(&s)["--str"];

    SECTION("First value") {
        parser.parse("--int --float 1.0 --str hello");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 1);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 6, ErrorType::Syntax_MissingValue);
    }

    SECTION("Second value") {
        parser.parse("--int 1 --float --str hello");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 1);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 16, ErrorType::Syntax_MissingValue);
    }

    SECTION("Third value") {
        parser.parse("--int 1 --float 1.0 --str");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 1);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 25, ErrorType::Syntax_MissingValue);
    }

    SECTION("First and second value") {
        parser.parse("--int --float --str hello");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 2);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 6 , ErrorType::Syntax_MissingValue);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), 14, ErrorType::Syntax_MissingValue);
    }

    SECTION("First and third value") {
        parser.parse("--int --float 1.0 --str");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 2);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 6 , ErrorType::Syntax_MissingValue);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), 23, ErrorType::Syntax_MissingValue);
    }

    SECTION("Second and third value") {
        parser.parse("--int 1 --float --str");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 2);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 16, ErrorType::Syntax_MissingValue);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), 21, ErrorType::Syntax_MissingValue);
    }

    SECTION("All values") {
        parser.parse("--int --float --str");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 3);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 6 , ErrorType::Syntax_MissingValue);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), 14, ErrorType::Syntax_MissingValue);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]), 19, ErrorType::Syntax_MissingValue);
    }

    CHECK(!parser.getAnalysisErrors().hasErrors());
}

TEST_CASE("Extra values", "[option][analysis][errors]") {
    using namespace Argon;
    int i; float f; std::string s;
    auto parser = Option(&i)["--int"]
                | Option(&f)["--float"]
                | Option(&s)["--str"];

    SECTION("First flag") {
        parser.parse("--int 1 extra --float 1.0 --str hello");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 1);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]), 8, ErrorType::Analysis_UnexpectedToken);
    }

    SECTION("Second flag") {
        parser.parse("--int 1 --float 1.0 extra --str hello");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 1);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]), 20, ErrorType::Analysis_UnexpectedToken);
    }

    SECTION("Third flag") {
        parser.parse("--int 1 --float 1.0 --str hello extra");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 1);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]), 32, ErrorType::Analysis_UnexpectedToken);
    }

    SECTION("First and second flag") {
        parser.parse("--int 1 extra --float 1.0 extra2 --str hello");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 2);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]), 8 , ErrorType::Analysis_UnexpectedToken);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[1]), 26, ErrorType::Analysis_UnexpectedToken);
    }

    SECTION("First and third flag") {
        parser.parse("--int 1 extra --float 1.0 --str hello extra3");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 2);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]), 8 , ErrorType::Analysis_UnexpectedToken);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[1]), 38, ErrorType::Analysis_UnexpectedToken);
    }

    SECTION("Second and third flag") {
        parser.parse("--int 1 --float 1.0 extra2 --str hello extra3");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 2);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]), 20, ErrorType::Analysis_UnexpectedToken);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[1]), 39, ErrorType::Analysis_UnexpectedToken);
    }

    SECTION("All flags") {
        parser.parse("--int 1 extra --float 1.0 extra2 --str hello extra3");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 3);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]), 8 , ErrorType::Analysis_UnexpectedToken);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[1]), 26, ErrorType::Analysis_UnexpectedToken);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[2]), 45, ErrorType::Analysis_UnexpectedToken);
    }

    CHECK(!parser.getSyntaxErrors().hasErrors());
}

TEST_CASE("Unknown flags", "[option][syntax][errors]") {
    using namespace Argon;
    int i; float f; std::string s;
    auto parser = Option(&i)["--int"]
                | Option(&f)["--float"]
                | Option(&s)["--str"];

    SECTION("First flag") {
        parser.parse("-int 1 --float 1.0 --str hello");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 1);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 0 , ErrorType::Syntax_UnknownFlag);
    }

    SECTION("Second flag") {
        parser.parse("--int 1 -float 1.0 --str hello");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 1);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 8 , ErrorType::Syntax_UnknownFlag);
    }

    SECTION("Third flag") {
        parser.parse("--int 1 --float 1.0 -str hello");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 1);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 20, ErrorType::Syntax_UnknownFlag);
    }

    SECTION("First and second flag") {
        parser.parse("-int 1 -float 1.0 --str hello");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 1);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 0 , ErrorType::Syntax_UnknownFlag);
    }

    SECTION("First and third flag") {
        parser.parse("-int 1 --float 1.0 -str hello");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 2);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 0 , ErrorType::Syntax_UnknownFlag);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), 19, ErrorType::Syntax_UnknownFlag);
    }

    SECTION("Second and third flag") {
        parser.parse("--int 1 -float 1.0 -str hello");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 1);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 8 , ErrorType::Syntax_UnknownFlag);
    }

    SECTION("All flags") {
        parser.parse("-int 1 -float 1.0 -str hello");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 1);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), 0 , ErrorType::Syntax_UnknownFlag);
    }

    CHECK(!parser.getAnalysisErrors().hasErrors());
}

TEST_CASE("Integer analysis errors", "[analysis][errors]") {
    using namespace Argon;
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
                | Option(&sc)  ["-sc"].withCharMode(CharMode::ExpectInteger)
                | Option(&uc)  ["-uc"].withCharMode(CharMode::ExpectInteger)
                | Option(&c)   ["-c"] .withCharMode(CharMode::ExpectInteger)
                | Option(&ss)  ["-ss"]      | Option(&us)  ["-us"]
                | Option(&si)  ["-si"]      | Option(&ui)  ["-ui"]
                | Option(&sl)  ["-sl"]      | Option(&ul)  ["-ul"]
                | Option(&sll) ["-sll"]     | Option(&ull) ["-ull"]
                | Option(&f)   ["-f"]       | Option(&d)   ["-d"]       | Option(&ld) ["-ld"];

    SECTION("Strings instead of numbers") {
        parser.parse("-fb hello -tb world -c string -sc  asdf -uc asdf "
                 "-ss asdf  -us asdf  -si  asdf -ui  asdf "
                 "-sl asdf  -ul asdf  -sll asdf -ull asdf "
                 "-f  word  -d  word  -ld  long");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 16);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]),
            {"-fb", "boolean", "hello"},    4 , ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[1]),
            {"-tb", "boolean", "world"},    14, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[2]),
            {"-c", "integer", "string"},    23, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[3]),
            {"-sc", "integer", "asdf"},     35, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[4]),
            {"-uc", "integer", "asdf"},     44, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[5]),
            {"-ss", "integer", "asdf"},     53, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[6]),
            {"-us", "integer", "asdf"},     63, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[7]),
            {"-si", "integer", "asdf"},     74, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[8]),
            {"-ui", "integer", "asdf"},     84, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[9]),
            {"-sl", "integer", "asdf"},     93, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[10]),
            {"-ul", "integer", "asdf"},     103, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[11]),
            {"-sll", "integer", "asdf"},    114, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[12]),
            {"-ull", "integer", "asdf"},    124, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[13]),
            {"-f", "floating", "word"},     133, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[14]),
            {"-d", "floating", "word"},     143, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[15]),
            {"-ld", "floating", "long"},    154, ErrorType::Analysis_ConversionError);
    }

    SECTION("Integrals over max") {
        parser.parse("-c  256        -sc 128        -uc 256 "
                     "-ss 32768      -us 65536 "
                     "-si 2147483648 -ui 4294967296 "
                     "-sl 2147483648 -ul 4294967296 "
                     "-sll 9223372036854775808 "
                     "-ull 18446744073709551616 ");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 11);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]),
            {"-c", "integer", "256"},           4, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[1]),
            {"-sc", "integer", "128"},          19, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[2]),
            {"-uc", "integer", "256"},          34, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[3]),
            {"-ss", "integer", "32768"},        42, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[4]),
            {"-us", "integer", "65536"},        57, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[5]),
            {"-si", "integer", "2147483648"},   67, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[6]),
            {"-ui", "integer", "4294967296"} ,  82, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[7]),
            {"-sl", "integer", "2147483648"} ,  97, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[8]),
            {"-ul", "integer", "4294967296"} ,  112, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[9]),
            {"-sll", "integer", "9223372036854775808"} ,    128, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[10]),
            {"-ull", "integer", "18446744073709551616"} ,   153, ErrorType::Analysis_ConversionError);
    }
    SECTION("Integrals at max") {
        parser.parse("-c  127        -sc 127        -uc 255 "
            "-ss 32767      -us 65535 "
            "-si 32767      -ui 65535 "
            "-sl 2147483645 -ul 4294967295 "
            "-sll 9223372036854775807 "
            "-ull 18446744073709551615 ");
        CHECK(!parser.hasErrors());
        CHECK(c == 127);    CHECK(sc == 127); CHECK(uc == 255);
        CHECK(ss == 32767); CHECK(us == 65535);
        CHECK(si == 32767); CHECK(ui == 65535);
        CHECK(sl == 2147483645);                CHECK(ul == 4294967295);
        CHECK(sll == 9223372036854775807ll);    CHECK(ull == 18446744073709551615ull);
    }


    SECTION("Integrals below min") {
        parser.parse("-c   -129        -sc -129      -uc -1 "
                     "-ss  -32769      -us -1 "
                     "-si  -2147483649 -ui -1 "
                     "-sl  -2147483649 -ul -1 "
                     "-sll -9223372036854775809 "
                     "-ull -1 ");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 11);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]),
            {"-c", "integer", "-129"},          5 , ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[1]),
            {"-sc", "integer", "-129"},         21, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[2]),
            {"-uc", "integer", "-1"},           35, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[3]),
            {"-ss", "integer", "-32769"},       43, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[4]),
            {"-us", "integer", "-1"},           59, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[5]),
            {"-si", "integer", "-2147483649"},  67, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[6]),
            {"-ui", "integer", "-1"} ,          83, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[7]),
            {"-sl", "integer", "-2147483649"},  91, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[8]),
            {"-ul", "integer", "-1"} ,          107, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[9]),
            {"-sll", "integer", "-9223372036854775809"} ,   115, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[10]),
            {"-ull", "integer", "-1"} ,                     141, ErrorType::Analysis_ConversionError);
    }

    SECTION("Integrals at min") {
        parser.parse("-c   0            -sc -128        -uc 0 "
                     "-ss  -32768       -us 0 "
                     "-si  -32768       -ui 0 "
                     "-sl  -2147483648  -ul 0 "
                     "-sll -9223372036854775808 "
                     "-ull 0 ");
        CHECK(!parser.hasErrors());
        CHECK(c == 0); CHECK(sc == -128); CHECK(uc == 0);
        CHECK(ss == -32768); CHECK(us == 0);
        CHECK(si == -32768); CHECK(ui == 0);
        CHECK(sl == -2147483648); CHECK(ull == 0);
        CHECK(sll == std::numeric_limits<long long>::min()); CHECK(ull == 0);
    }
}

TEST_CASE("Parser setCharMode errors", "[parser][config][errors][char]") {
    using namespace Argon;
    char cOpt; signed char scOpt; unsigned char ucOpt;
    char cPos; signed char scPos; unsigned char ucPos;
    auto parser = Option(&cOpt)["-c"]
                | Option(&scOpt)["-sc"]
                | Option(&ucOpt)["-uc"]
                | Positional(&cPos)
                | Positional(&scPos)
                | Positional(&ucPos);
    SECTION("Ascii partially correct") {
        parser.getConfig().setDefaultCharMode(CharMode::ExpectAscii);
        parser.parse("a  -c  10 "
                     "20 -sc b "
                     "c  -uc 30");
        CHECK(parser.hasErrors());
        CHECK(cPos == 'a'); CHECK(scOpt == 'b'); CHECK(ucPos == 'c');
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 3);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]), {"ASCII", "10", "-c"},  7,  ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[1]), {"ASCII", "20"},        10, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[2]), {"ASCII", "30", "-uc"}, 26, ErrorType::Analysis_ConversionError);
    }
    SECTION("Integer partially correct") {
        parser.getConfig().setDefaultCharMode(CharMode::ExpectInteger);
        parser.parse("a -c  1 "
                     "2 -sc b "
                     "c -uc 3");
        CHECK(parser.hasErrors());
        CHECK(cOpt == 1); CHECK(scPos == 2); CHECK(ucOpt == 3);
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 3);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]), {"integer", "a"},           0,  ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[1]), {"integer", "b", "-sc"},    14, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[2]), {"integer", "c",},          16, ErrorType::Analysis_ConversionError);
    }
}

TEST_CASE("Positional", "[positional][analysis][errors]") {
    using namespace Argon;
    int input, output, input2, output2;
    std::string name, home;

    auto parser = Positional(10, &input)("Input", "Input count")
                | Positional(20, &output)("Output", "Output count")
                | Option<std::string>("Sally", &name)["--name"]
                | (
                    OptionGroup()["--group"]
                    + Positional(30, &input2)("Input2", "Input2 count")
                    + Positional(40, &output2)("Output2", "Output2 count")
                    + Option<std::string>("Street", &home)["--home"]
                );
    SECTION("Test 1") {
        parser.parse("100 200 --name John --group []");
        CHECK(!parser.hasErrors());
        CHECK(input == 100); CHECK(output == 200); CHECK(name == "John");
        CHECK(input2 == 30); CHECK(output2 == 40); CHECK(home == "Street");
    }
    SECTION("Test 2") {
        parser.parse("100 --name John 200 --group [--home MyHome 50 60]");
        CHECK(!parser.hasErrors());
        CHECK(input == 100); CHECK(output == 200); CHECK(name == "John");
        CHECK(input2 == 50); CHECK(output2 == 60); CHECK(home == "MyHome");
    }
    SECTION("Test 3") {
        parser.parse("100 200 300 400");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 2);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]), {"300"}, 8,  ErrorType::Analysis_UnexpectedToken);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[1]), {"400"}, 12, ErrorType::Analysis_UnexpectedToken);
    }
    SECTION("Test 4") {
        parser.parse("100 --name 200 John 300 400");
        CHECK(parser.hasErrors());
        const auto& analysisErrors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 3);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[0]), {"Output","John"}, 15,  ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[1]), {"300"}, 20, ErrorType::Analysis_UnexpectedToken);
        CheckMessage(RequireMsg(analysisErrors.getErrors()[2]), {"400"}, 24, ErrorType::Analysis_UnexpectedToken);
    }
}

TEST_CASE("Positional policy with parser config", "[positional][errors]") {
    using namespace Argon;
    int input, output;
    std::string name;

    int input2, output2, zipcode;
    std::string street;

    std::string class1, class2;
    int homeroom;

    std::string teacher1, teacher2;
    int classroom1, classroom2;

    auto teachersGroup = OptionGroup()["--teachers"]
        + Positional<std::string>("Teacher", &teacher1)("Teacher 1", "Teacher 1")
        + Positional<std::string>("Teacher", &teacher2)("Teacher 2", "Teacher 2")
        + Option(0, &classroom1)["--classroom1"]
        + Option(0, &classroom2)["--classroom2"];

    auto schoolGroup = OptionGroup()["--school"]
        + Positional<std::string>("Class", &class1)("Class 1", "Class 1")
        + Positional<std::string>("Class", &class2)("Class 2", "Class 2")
        + Option(026, &homeroom)["--homeroom"]
        + teachersGroup;

    auto addressGroup = OptionGroup()["--address"]
        + Positional(30, &input2)("Input2", "Input2 count")
        + Positional(40, &output2)("Output2", "Output2 count")
        + Option<std::string>("Street", &street)["--street"]
        + Option(123, &zipcode)["--zipcode"];


    auto parser = Positional(10, &input)("Input", "Input count")
                | Positional(20, &output)("Output", "Output count")
                | Option<std::string>("Sally", &name)["--name1"]
                | Option<std::string>("Sally", &name)["--name2"]
                | Option<std::string>("Sally", &name)["--name3"]
                | addressGroup
                | schoolGroup;

    SECTION("Before flags test 1") {
        parser.getConfig().setDefaultPositionalPolicy(PositionalPolicy::BeforeFlags);
        parser.parse("--name1 John 100 --name2 Sammy 200 --name3 Joshua 300 Sam --address [400 --street Jam 500] "
                     "--school [History --homeroom 026 --teachers [--classroom1 10 Mr.Smith --classroom2 20 Mrs.Smith] English]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 8);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), {"--name1", "100"}, 13, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), {"--name2", "200"}, 31, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]), {"--name3", "300"}, 50, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[3]), {"--name3", "Sam"}, 54, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[4]), {"--street", "500", "--address"}, 86, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[5]), {"--classroom1", "Mr.Smith",  "--school > --teachers"}, 152, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[6]), {"--classroom2", "Mrs.Smith", "--school > --teachers"}, 177, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[7]), {"--teachers", "English", "--school"}, 188, ErrorType::Syntax_MisplacedPositional);
    }

    SECTION("Before flags test 2") {
        parser.getConfig().setDefaultPositionalPolicy(PositionalPolicy::BeforeFlags);
        parser.parse("100 --name1 John --name2 Sammy 200 300 Sam --name3 Joshua --address [--street Jam 400 500] "
                     "--school [--homeroom 026 History --teachers [--classroom1 10 --classroom2 20 Mr.Smith Mrs.Smith] English]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 9);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), {"--name2", "200"}, 31, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), {"--name2", "300"}, 35, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]), {"--name2", "Sam"}, 39, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[3]), {"--street", "400", "--address"}, 82, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[4]), {"--street", "500", "--address"}, 86, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[5]), {"--homeroom", "History", "--school"}, 116, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[6]), {"--classroom2", "Mr.Smith",  "--school > --teachers"}, 168, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[7]), {"--classroom2", "Mrs.Smith", "--school > --teachers"}, 177, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[8]), {"--teachers", "English", "--school"}, 188, ErrorType::Syntax_MisplacedPositional);
    }

    SECTION("Before flags test 3") {
        parser.getConfig().setDefaultPositionalPolicy(PositionalPolicy::BeforeFlags);
        parser.parse("100 200 --name2 Sammy --school [--teachers [--classroom1 10 Mr.Smith Mrs.Smith --classroom2 20] "
                     "English --homeroom 026 History] Sam --name3 Joshua --address [400 500 --street Jam] 300");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 6);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), {"--classroom1", "Mr.Smith", "--school > --teachers"}, 60, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), {"--classroom1", "Mrs.Smith","--school > --teachers"}, 69, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]), {"--teachers", "English","--school"}, 96,  ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[3]), {"--homeroom", "History","--school"}, 119, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[4]), {"--school", "Sam","--school"}, 128, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[5]), {"--address", "300"}, 180, ErrorType::Syntax_MisplacedPositional);
    }

    SECTION("Before flags no errors") {
        parser.getConfig().setDefaultPositionalPolicy(PositionalPolicy::BeforeFlags);
        parser.parse("100 200 --name1 John --address [300 400 --street Jam] --name2 Sammy --school [History English "
                     "--teachers [Mr.Smith Mrs.Smith --classroom1 10 --classroom2 20] --homeroom 026] --name3 Joshua");
        CHECK(!parser.hasErrors());
        CHECK(parser.getOptionValue<std::string>("--name1") == "John");
        CHECK(parser.getOptionValue<std::string>("--name2") == "Sammy");
        CHECK(parser.getOptionValue<std::string>("--name3") == "Joshua");
        CHECK(parser.getPositionalValue<int, 0>() == 100);
        CHECK(parser.getPositionalValue<int, 1>() == 200);

        CHECK(parser.getOptionValue<std::string>({"--address", "--street"}) == "Jam");
        CHECK(parser.getPositionalValue<int, 0>("--address") == 300);
        CHECK(parser.getPositionalValue<int, 1>("--address") == 400);

        CHECK(parser.getOptionValue<int>({"--school", "--homeroom"}) == 26);
        CHECK(parser.getPositionalValue<std::string, 0>("--school") == "History");
        CHECK(parser.getPositionalValue<std::string, 1>("--school") == "English");

        CHECK(parser.getOptionValue<int>({"--school", "--teachers", "--classroom1"}) == 10);
        CHECK(parser.getOptionValue<int>({"--school", "--teachers", "--classroom2"}) == 20);
        CHECK(parser.getPositionalValue<std::string, 0>({"--school", "--teachers"}) == "Mr.Smith");
        CHECK(parser.getPositionalValue<std::string, 1>({"--school", "--teachers"}) == "Mrs.Smith");
    }

    SECTION("After flags test 1") {
        parser.getConfig().setDefaultPositionalPolicy(PositionalPolicy::AfterFlags);
        parser.parse("--name1 John 100 --name2 Sammy 200 --name3 Joshua 300 Sam --address [400 --street Jam 500] "
                     "--school [History --homeroom 026 --teachers [--classroom1 10 Mr.Smith --classroom2 20 Mrs.Smith] English]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 7);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), {"--name2", "100"}, 13, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), {"--name3", "200"}, 31, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]), {"--address", "300"}, 50, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[3]), {"--address", "Sam"}, 54, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[4]), {"--street", "400", "--address"}, 69, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[5]), {"--homeroom", "History", "--school"}, 101, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[6]), {"--classroom2", "Mr.Smith", "--school > --teachers"}, 152, ErrorType::Syntax_MisplacedPositional);
    }

    SECTION("After flags test 2") {
        parser.getConfig().setDefaultPositionalPolicy(PositionalPolicy::AfterFlags);
        parser.parse("100 --name1 John --name2 Sammy 200 300 Sam --name3 Joshua --address [--street Jam 400 500] "
                     "--school [--homeroom 026 History --teachers [--classroom1 10 --classroom2 20 Mr.Smith Mrs.Smith] English]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 5);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), {"--name1", "100"}, 0,  ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), {"--name3", "200"}, 31, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]), {"--name3", "300"}, 35, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[3]), {"--name3", "Sam"}, 39, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[4]), {"--teachers", "History", "--school"}, 116, ErrorType::Syntax_MisplacedPositional);
    }

    SECTION("After flags test 3") {
        parser.getConfig().setDefaultPositionalPolicy(PositionalPolicy::AfterFlags);
        parser.parse("100 200 --name2 Sammy --school [--teachers [--classroom1 10 Mr.Smith Mrs.Smith --classroom2 20] "
                     "English --homeroom 026 History] Sam --name3 Joshua --address [400 500 --street Jam] 300");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 8);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), {"--name2", "100"}, 0,  ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), {"--name2", "200"}, 4,  ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]), {"--classroom2", "Mr.Smith",  "--school > --teachers"}, 60,  ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[3]), {"--classroom2", "Mrs.Smith", "--school > --teachers"}, 69,  ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[4]), {"--homeroom", "English", "--school"}, 96,  ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[5]), {"--name3", "Sam"}, 128,  ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[6]), {"--street", "400", "--address"}, 158,  ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[7]), {"--street", "500", "--address"}, 162,  ErrorType::Syntax_MisplacedPositional);
    }

    SECTION("After flags no errors") {
        parser.getConfig().setDefaultPositionalPolicy(PositionalPolicy::AfterFlags);
        parser.parse("--name1 John --name2 Sammy --name3 Joshua --school [--homeroom 26 --teachers [--classroom1 10 "
                     "--classroom2 20 Mr.Smith Mrs.Smith] History English] --address [--street Jam 300 400] 100 200");
        CHECK(!parser.hasErrors());
        CHECK(parser.getOptionValue<std::string>("--name1") == "John");
        CHECK(parser.getOptionValue<std::string>("--name2") == "Sammy");
        CHECK(parser.getOptionValue<std::string>("--name3") == "Joshua");
        CHECK(parser.getPositionalValue<int, 0>() == 100);
        CHECK(parser.getPositionalValue<int, 1>() == 200);

        CHECK(parser.getOptionValue<std::string>({"--address", "--street"}) == "Jam");
        CHECK(parser.getPositionalValue<int, 0>("--address") == 300);
        CHECK(parser.getPositionalValue<int, 1>("--address") == 400);

        CHECK(parser.getOptionValue<int>({"--school", "--homeroom"}) == 26);
        CHECK(parser.getPositionalValue<std::string, 0>("--school") == "History");
        CHECK(parser.getPositionalValue<std::string, 1>("--school") == "English");

        CHECK(parser.getOptionValue<int>({"--school", "--teachers", "--classroom1"}) == 10);
        CHECK(parser.getOptionValue<int>({"--school", "--teachers", "--classroom2"}) == 20);
        CHECK(parser.getPositionalValue<std::string, 0>({"--school", "--teachers"}) == "Mr.Smith");
        CHECK(parser.getPositionalValue<std::string, 1>({"--school", "--teachers"}) == "Mrs.Smith");
    }

    SECTION("Mixed policy test") {
        parser.withDefaultPositionalPolicy(PositionalPolicy::AfterFlags);
        teachersGroup.withDefaultPositionalPolicy(PositionalPolicy::BeforeFlags);
        schoolGroup.withDefaultPositionalPolicy(PositionalPolicy::AfterFlags);
        parser.parse("--name1 John 100 --name2 Sammy 200 --name3 Joshua 300 Sam --address [400 --street Jam 500] "
                     "--school [History --homeroom 026 --teachers [--classroom1 10 Mr.Smith --classroom2 20 Mrs.Smith] English]");
        CHECK(parser.hasErrors());
        const auto& syntaxErrors = CheckGroup(parser.getSyntaxErrors(), "Syntax Errors", -1, -1, 8);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[0]), {"--name2", "100"}, 13, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[1]), {"--name3", "200"}, 31, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[2]), {"--address", "300"}, 50, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[3]), {"--address", "Sam"}, 54, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[4]), {"--street", "400", "--address"}, 69, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[5]), {"--homeroom", "History", "--school"}, 101, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[6]), {"--classroom1", "Mr.Smith", "--school > --teachers"}, 152, ErrorType::Syntax_MisplacedPositional);
        CheckMessage(RequireMsg(syntaxErrors.getErrors()[7]), {"--classroom2", "Mrs.Smith", "--school > --teachers"}, 177, ErrorType::Syntax_MisplacedPositional);
    }
}
