#include "catch2/catch_approx.hpp"

#include "Argon/Parser.hpp"
#include "ErrorTestFunctions.hpp"

using namespace Argon;

TEST_CASE("Attributes test 2", "[attributes]") {
    auto parser = Option<int>()[{"-x", "-x2"}]
                | Option<int>()["-y"]
                | Option<int>()["-z"]
                | MultiOption<std::array<int, 3>>()["-w"]
                | (
                    OptionGroup()[{"--group", "-g"}]
                    + Option<int>()["-a"]
                    + Option<int>()["-b"]
                    + Option<int>()["-c"]
                );

    parser.constraints()
        .require({"-x"}, "Flag 'X' is a required flag, this must be set otherwise the program fails")
        .dependsOn({"-x"}, {{"-y"}, {"--group", "-a"}}, [](const std::vector<std::string>& args) {
            std::string msg = "This flag REQUIRES the following flag to be set:";
            for (const auto& arg : args) {
                msg += " ";
                msg += arg;
            }
            return msg;
        });
    parser.parse("-x 10 -y 2 -z 30 -w 1 2 3");
    // parser.printErrors();
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
    // int x, y;
    // auto parser = Option(&x)["main"]["m"]
    //             | Option(&y)["--main2"]["-m2"];
    //
    // parser.parse("--main 1 -m 2 --main2 3 -m2 4");
    //
    // CHECK(!parser.hasErrors());
    // // parser.printErrors();
    // CHECK(x == 2);
    // CHECK(y == 4);
}

TEST_CASE("Duplicate requirement", "[constraints][duplicate]") {
    auto parser = Option<int>()[{"--xcoord", "--xcoordinate", "-x"}]
                | Option<int>()[{"--ycoord", "--ycoordinate", "-y"}]
                | (
                    OptionGroup()[{"--group", "-g"}]
                    + Option<int>()[{"--xcoord", "--xcoordinate", "-x"}]
                    + Option<int>()[{"--ycoord", "--ycoordinate", "-y"}]
                );

    SECTION("Duplicated more than twice") {
        parser.constraints()
            .require({"-x"}).require({"-x"}).require({"-x"})
            .require({"-y"}).require({"-y"}).require({"-y"});

        parser.parse("");
        CHECK(parser.hasErrors());
        const auto& errors = CheckGroup(parser.getValidationErrors(), "Validation Errors", -1, -1, 2);
        CheckMessage(RequireMsg(errors.getErrors()[0]), {"--xcoord"} ,-1, ErrorType::Validation_DuplicateRequirement);
        CheckMessage(RequireMsg(errors.getErrors()[1]), {"--ycoord"} ,-1, ErrorType::Validation_DuplicateRequirement);
   }

    SECTION("Using aliases") {
        parser.constraints()
             .require({"--xcoordinate"}).require({"-x"}).require({"--xcoord"})
             .require({"--ycoordinate"}).require({"-y"}).require({"--ycoord"});

        parser.parse("");
        CHECK(parser.hasErrors());
        const auto& errors = CheckGroup(parser.getValidationErrors(), "Validation Errors", -1, -1, 2);
        CheckMessage(RequireMsg(errors.getErrors()[0]), {"--xcoord"} ,-1, ErrorType::Validation_DuplicateRequirement);
        CheckMessage(RequireMsg(errors.getErrors()[1]), {"--ycoord"} ,-1, ErrorType::Validation_DuplicateRequirement);
    }

    SECTION("Group no errors") {
        parser.constraints()
            .require({"--xcoord"})
            .require({"--ycoord"})
            .require({"--group", "-x"})
            .require({"--group", "-y"});
        parser.parse("-x 10 -y 20 --group [-x 30 -y 40]");

        CHECK(!parser.hasErrors());
        CHECK(parser.getOptionValue<int>("-x") == 10);
        CHECK(parser.getOptionValue<int>("-y") == 20);
        CHECK(parser.getOptionValue<int>({"--group", "-x"}) == 30);
        CHECK(parser.getOptionValue<int>({"--group", "-y"}) == 40);
    }

    SECTION("Group errors") {
        parser.constraints()
            .require({"--xcoord"})
            .require({"--ycoord"})
            .require({"--group", "-x"}).require({"-g", "--xcoord"}).require({"-g", "-x"})
            .require({"--group", "-y"}).require({"-g", "--ycoord"}).require({"-g", "-y"});
        parser.parse("-x 10 -y 20 --group [-x 30 -y 40]");

        CHECK(parser.hasErrors());
        const auto& errors = CheckGroup(parser.getValidationErrors(), "Validation Errors", -1, -1, 2);
        CheckMessage(RequireMsg(errors.getErrors()[0]), {"--group > --xcoord"} ,-1, ErrorType::Validation_DuplicateRequirement);
        CheckMessage(RequireMsg(errors.getErrors()[1]), {"--group > --ycoord"} ,-1, ErrorType::Validation_DuplicateRequirement);
    }
}

TEST_CASE("Mutual exclusion cycle", "[constraints][mutual-exclusion]") {
    auto parser = Option<int>()[{"--xcoord", "--xcoordinate", "-x"}]
                | Option<int>()[{"--ycoord", "--ycoordinate", "-y"}]
                | (
                    OptionGroup()[{"--group", "-g"}]
                    + Option<int>()[{"--xcoord", "--xcoordinate", "-x"}]
                    + Option<int>()[{"--ycoord", "--ycoordinate", "-y"}]
                );

    SECTION("Top level") {
        parser.constraints()
            .mutuallyExclusive(FlagPath{"-x"}, {FlagPath{"-x"}, FlagPath{"-y"}})
            .mutuallyExclusive(FlagPath{"-y"}, {FlagPath{"-x"}, FlagPath{"-y"}});
        parser.parse("");
        CHECK(parser.hasErrors());
        const auto& errors = CheckGroup(parser.getValidationErrors(), "Validation Errors", -1, -1, 2);
        CheckMessage(RequireMsg(errors.getErrors()[0]), {"--xcoord"} ,-1, ErrorType::Validation_MutualExclusionCycle);
        CheckMessage(RequireMsg(errors.getErrors()[1]), {"--ycoord"} ,-1, ErrorType::Validation_MutualExclusionCycle);
    }

    SECTION("Group no errors") {
        parser.constraints()
            .mutuallyExclusive(FlagPath{"-x"}, {FlagPath{"--group", "-x"}, FlagPath{"-y"}})
            .mutuallyExclusive(FlagPath{"-y"}, {FlagPath{"-x"}, FlagPath{"--group", "-y"}});
        parser.parse("");
        CHECK(!parser.hasErrors());
    }

    SECTION("Group errors") {
        parser.constraints()
            .mutuallyExclusive(FlagPath{"--group", "-x"}, {FlagPath{"--group", "-x"}, FlagPath{"-y"}})
            .mutuallyExclusive(FlagPath{"--group", "-y"}, {FlagPath{"-x"}, FlagPath{"--group", "-y"}});
        parser.parse("");
        CHECK(parser.hasErrors());
        const auto& errors = CheckGroup(parser.getValidationErrors(), "Validation Errors", -1, -1, 2);
        CheckMessage(RequireMsg(errors.getErrors()[0]), {"--group > --xcoord"} ,-1, ErrorType::Validation_MutualExclusionCycle);
        CheckMessage(RequireMsg(errors.getErrors()[1]), {"--group > --ycoord"} ,-1, ErrorType::Validation_MutualExclusionCycle);
    }
}

TEST_CASE("Dependent cycle", "[constraints][dependent]") {
    auto parser = Option<int>()[{"--xcoord", "--xcoordinate", "-x"}]
                | Option<int>()[{"--ycoord", "--ycoordinate", "-y"}]
                | (
                    OptionGroup()[{"--group", "-g"}]
                    + Option<int>()[{"--xcoord", "--xcoordinate", "-x"}]
                    + Option<int>()[{"--ycoord", "--ycoordinate", "-y"}]
                );

    SECTION("Top level") {
        parser.constraints()
            .dependsOn(FlagPath{"-x"}, {FlagPath{"-x"}, FlagPath{"-y"}})
            .dependsOn(FlagPath{"-y"}, {FlagPath{"-x"}, FlagPath{"-y"}});
        parser.parse("");
        CHECK(parser.hasErrors());
        const auto& errors = CheckGroup(parser.getValidationErrors(), "Validation Errors", -1, -1, 2);
        CheckMessage(RequireMsg(errors.getErrors()[0]), {"--xcoord"} ,-1, ErrorType::Validation_DependentCycle);
        CheckMessage(RequireMsg(errors.getErrors()[1]), {"--ycoord"} ,-1, ErrorType::Validation_DependentCycle);
    }

    SECTION("Group no errors") {
        parser.constraints()
            .dependsOn(FlagPath{"-x"}, {FlagPath{"--group", "-x"}, FlagPath{"-y"}})
            .dependsOn(FlagPath{"-y"}, {FlagPath{"-x"}, FlagPath{"--group", "-y"}});
        parser.parse("");
        CHECK(!parser.hasErrors());
    }

    SECTION("Group errors") {
        parser.constraints()
            .dependsOn(FlagPath{"--group", "-x"}, {FlagPath{"--group", "-x"}, FlagPath{"-y"}})
            .dependsOn(FlagPath{"--group", "-y"}, {FlagPath{"-x"}, FlagPath{"--group", "-y"}});
        parser.parse("");
        CHECK(parser.hasErrors());
        const auto& errors = CheckGroup(parser.getValidationErrors(), "Validation Errors", -1, -1, 2);
        CheckMessage(RequireMsg(errors.getErrors()[0]), {"--group > --xcoord"} ,-1, ErrorType::Validation_DependentCycle);
        CheckMessage(RequireMsg(errors.getErrors()[1]), {"--group > --ycoord"} ,-1, ErrorType::Validation_DependentCycle);
    }
}

TEST_CASE("Help message 2", "[help][test]") {
    const auto parser =
        Option<int>()["--xcoord"]["-x"]("<int>", "x coordinate of the location. This is a really long description to test how overflow works :D. Wow this is a very amazing feature.")
        | Option<int>()["--ycoord"]["-y"]("<int>", "y coordinate of the location ycoordinateofthelocation ycoordinateofthelocationycoordinateofthelocation")
        | Option<int>()["--zcoord"]["-z"]("<int>", "z coordinate of the location")
        | (
            OptionGroup()["--student"]("[Student Info]", "Specify information about the main character")
            + Option<int>()["--name"]("The name of the student")
            + Option<int>()["--age"]("The age of the student")
            + (
                OptionGroup()["--classes"]("[Class Information]", "The classes the student takes")
                + Option<int>()["--major"]["--maj"]("The main class the student is taking")
                + Option<int>()["--minor"]["--min"]("The side class the student is taking")
            )
        )
        | Option<int>()["--region"]("The region the game takes place in")
        | (
            OptionGroup()["--student2"]("[Student Info 2]", "Specify information about the second character")
            + Option<int>()["--name"]("The name of the student")
            + Option<int>()["--age"]("The age of the student")
            + (
                OptionGroup()["--classes"]("[Class Information]", "The classes the student takes")
                + Option<int>()["--major"]["--maj"]("The main class the student is taking")
                + Option<int>()["--minor"]["--min"]("The side class the student is taking")
            )
        )
        | MultiOption<std::vector<int>>()["--courseids"]("<ids...>", "Specify a list of course ids ");

    parser.constraints()
        .require({"--xcoord"}).require({"--ycoord"}).require({"--zcoord"})
        .require({"--student", "--name"}).require({"--student", "--age"})
        .mutuallyExclusive({"--student2", "--name"}, {FlagPath{"--student2", "--age"}, FlagPath({"--student2" , "--classes", "--minor"})})
        .dependsOn({"--student", "--name"}, {FlagPath{"--student", "--classes", "--major"}, FlagPath{"--student", "--classes", "--minor"}});

    const auto msg = parser.getHelpMessage();
    std::cout << msg << "\n\n\n";
}

TEST_CASE("Positional help message", "[help][positional]") {
    const auto parser =
        Option<int>()["--xcoord"]["-x"]("<int>", "x coordinate of the location.")
        | Option<int>()["--ycoord"]["-y"]("<int>", "y coordinate of the location.")
        | Option<int>()["--zcoord"]["-z"]("<int>", "z coordinate of the location")
        | Option<int>()["--region"]("The region the game takes place in.")
        | MultiOption<std::vector<int>>()["--courseids"]("<ids...>", "Specify a list of course ids.")
        | Positional<int>()("<positionalname>", "Test description.")
        | Positional<int>()("<secondpositional>", "Second test description.")
        | (
            OptionGroup()["--student"]("Specify information about the main character")
            + Option<int>()["--name"]("The name of the student")
            + Option<int>()["--age"]("The age of the student")
            + Positional<int>()("<positionalname>", "Test description.")
            + Positional<int>()("<secondpositional>", "Second test description.")
            + Positional<int>()("<thirdpositional>", "Second test description.")
            + Positional<int>()("<fourthpositional>", "Second test description.")
            + Positional<int>()("<fourthpositional>", "Second test description.")
            + Positional<int>()("<fourthpositional>", "Second test description.")
            + Positional<int>()("<fourthpositional>", "Second test description.")
            + Positional<int>()("<fourthpositional>", "Second test description.")
        );
    std::cout << parser.getHelpMessage();
}

TEST_CASE("Min and max values", "[constraints][min][max]") {
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

    auto parser = Option(&sc)  ["-sc"].withCharMode(CharMode::ExpectInteger).withMin(10).withMax(20)
                | Option(&uc)  ["-uc"].withCharMode(CharMode::ExpectInteger).withMin(10).withMax(20)
                | Option(&c)   ["-c"] .withCharMode(CharMode::ExpectInteger).withMin(10).withMax(20)
                | Option(&ss)  ["-ss"].withMin(10).withMax(20)      | Option(&us)  ["-us"].withMin(10).withMax(20)
                | Option(&si)  ["-si"].withMin(10).withMax(20)      | Option(&ui)  ["-ui"].withMin(10).withMax(20)
                | Option(&sl)  ["-sl"].withMin(10).withMax(20)      | Option(&ul)  ["-ul"].withMin(10).withMax(20)
                | Option(&sll) ["-sll"].withMin(10).withMax(20)     | Option(&ull) ["-ull"].withMin(10).withMax(20)
                | Option(&f)   ["-f"].withMin(10).withMax(20)       | Option(&d)   ["-d"].withMin(10).withMax(20)
                | Option(&ld) ["-ld"].withMin(10).withMax(20);

    SECTION("No errors") {
        const std::string input = "-sc  15      -uc  15     -c 15 "
                                  "-ss  15      -us  15     "
                                  "-si  15      -ui  15     "
                                  "-sl  15      -ul  15     "
                                  "-sll 15      -ull 15     "
                                  "-f   15      -d   15     -ld 15";
        parser.parse(input);
        CHECK(!parser.hasErrors());
        CHECK(sc  == 15);   CHECK(uc  == 15);   CHECK(c == 15);
        CHECK(ss  == 15);   CHECK(us  == 15);
        CHECK(si  == 15);   CHECK(ui  == 15);
        CHECK(sl  == 15);   CHECK(ul  == 15);
        CHECK(sll == 15);   CHECK(ull == 15);
        CHECK(f   ==  Catch::Approx(15.0).epsilon(1e-6));
        CHECK(d   ==  Catch::Approx(15.0).epsilon(1e-6));
        CHECK(ld  ==  Catch::Approx(15.0).epsilon(1e-6));
    }

    SECTION("Less than min") {
        const std::string input = "-sc  0       -uc  0      -c 0  "
                                  "-ss  0       -us  0 "
                                  "-si  0       -ui  0 "
                                  "-sl  0       -ul  0 "
                                  "-sll 0       -ull 0 "
                                  "-f   0       -d   0      -ld 0";
        parser.parse(input);
        CHECK(parser.hasErrors());
        const auto& errors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 14);
        CheckMessage(RequireMsg(errors.getErrors()[0]), {"-sc", "10", "20"}, 5,  ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[1]), {"-uc", "10", "20"}, 18, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[2]), {"-c" , "10", "20"}, 28, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[3]), {"-ss", "10", "20"}, 36, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[4]), {"-us", "10", "20"}, 49, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[5]), {"-si", "10", "20"}, 56, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[6]), {"-ui", "10", "20"}, 69, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[7]), {"-sl", "10", "20"}, 76, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[8]), {"-ul", "10", "20"}, 89, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[9]), {"-sll", "10", "20"}, 96, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[10]), {"-ull", "10", "20"}, 109, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[11]), {"-f",  "10", "20"}, 116, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[12]), {"-d",  "10", "20"}, 129, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[13]), {"-ld", "10", "20"}, 140, ErrorType::Analysis_ConversionError);
    }

    SECTION("At min") {
        const std::string input = "-sc  10      -uc  10     -c 10 "
                                  "-ss  10      -us  10     "
                                  "-si  10      -ui  10     "
                                  "-sl  10      -ul  10     "
                                  "-sll 10      -ull 10     "
                                  "-f   10      -d   10     -ld 10";
        parser.parse(input);
        CHECK(!parser.hasErrors());
        CHECK(sc  == 10);   CHECK(uc  == 10);   CHECK(c == 10);
        CHECK(ss  == 10);   CHECK(us  == 10);
        CHECK(si  == 10);   CHECK(ui  == 10);
        CHECK(sl  == 10);   CHECK(ul  == 10);
        CHECK(sll == 10);   CHECK(ull == 10);
        CHECK(f   ==  Catch::Approx(10.0).epsilon(1e-6));
        CHECK(d   ==  Catch::Approx(10.0).epsilon(1e-6));
        CHECK(ld  ==  Catch::Approx(10.0).epsilon(1e-6));
    }

    SECTION("Greater than max") {
        const std::string input = "-sc  30      -uc  30      -c 0  "
                                  "-ss  30      -us  30 "
                                  "-si  30      -ui  30 "
                                  "-sl  30      -ul  30 "
                                  "-sll 30      -ull 30 "
                                  "-f   30      -d   30      -ld 0";
        parser.parse(input);
        CHECK(parser.hasErrors());
        const auto& errors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 14);
        CheckMessage(RequireMsg(errors.getErrors()[0]), {"-sc", "10", "20"}, 5,  ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[1]), {"-uc", "10", "20"}, 18, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[2]), {"-c" , "10", "20"}, 29, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[3]), {"-ss", "10", "20"}, 37, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[4]), {"-us", "10", "20"}, 50, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[5]), {"-si", "10", "20"}, 58, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[6]), {"-ui", "10", "20"}, 71, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[7]), {"-sl", "10", "20"}, 79, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[8]), {"-ul", "10", "20"}, 92, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[9]), {"-sll", "10", "20"}, 100, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[10]), {"-ull", "10", "20"}, 113, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[11]), {"-f",  "10", "20"}, 121, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[12]), {"-d",  "10", "20"}, 134, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[13]), {"-ld", "10", "20"}, 146, ErrorType::Analysis_ConversionError);
    }

    SECTION("At max") {
        const std::string input = "-sc  20      -uc  20     -c 20 "
                                  "-ss  20      -us  20     "
                                  "-si  20      -ui  20     "
                                  "-sl  20      -ul  20     "
                                  "-sll 20      -ull 20     "
                                  "-f   20      -d   20     -ld 20";
        parser.parse(input);
        CHECK(!parser.hasErrors());
        CHECK(sc  == 20);   CHECK(uc  == 20);   CHECK(c == 20);
        CHECK(ss  == 20);   CHECK(us  == 20);
        CHECK(si  == 20);   CHECK(ui  == 20);
        CHECK(sl  == 20);   CHECK(ul  == 20);
        CHECK(sll == 20);   CHECK(ull == 20);
        CHECK(f   ==  Catch::Approx(20.0).epsilon(1e-6));
        CHECK(d   ==  Catch::Approx(20.0).epsilon(1e-6));
        CHECK(ld  ==  Catch::Approx(20.0).epsilon(1e-6));
    }
}

TEST_CASE("Floating point min and max", "[constraints][min][max][float]") {
    float       f   = 0;
    double      d   = 0;
    long double ld  = 0;

    auto floatOpt = Option(&f)["-f"];
    auto doubleOpt = Option(&d)["-d"];
    auto longDoubleOpt = Option(&ld)["-ld"];

    auto parser = floatOpt | doubleOpt | longDoubleOpt;

    SECTION("Only min set") {
        floatOpt.withMin(10);
        doubleOpt.withMin(10);
        longDoubleOpt.withMin(10);
        parser.parse("-f -10000.123456 -d 0.55 -ld 9.9999");
        CHECK(parser.hasErrors());
        const auto& errors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 3);
        CheckMessage(RequireMsg(errors.getErrors()[0]), {"-f", "greater", "10.0", "-10000.1234"}, 3, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[1]), {"-d", "greater", "10.0", "0.55"},        20, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[2]), {"-ld", "greater", "10.0", "9.9999"},     29, ErrorType::Analysis_ConversionError);
    }

    SECTION("Only max set") {
        floatOpt.withMax(10);
        doubleOpt.withMax(10);
        longDoubleOpt.withMax(10);
        parser.parse("-f 10000.123456 -d 10.55 -ld 19.9999");
        CHECK(parser.hasErrors());
        const auto& errors = CheckGroup(parser.getAnalysisErrors(), "Analysis Errors", -1, -1, 3);
        CheckMessage(RequireMsg(errors.getErrors()[0]), {"-f", "less", "10.0", "10000.1234"}, 3, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[1]), {"-d", "less", "10.0", "10.55"},      19, ErrorType::Analysis_ConversionError);
        CheckMessage(RequireMsg(errors.getErrors()[2]), {"-ld", "less", "10.0", "19.9999"},   29, ErrorType::Analysis_ConversionError);
    }
}