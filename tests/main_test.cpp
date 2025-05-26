#include <array>

#include "testing.hpp"

#include <chrono>
#include <iostream>

#include "Argon/MultiOption.hpp"
#include "Argon/Option.hpp"
#include "Argon/Parser.hpp" 

static void boolTest() {
    using namespace Argon;
    
    bool boolTest = true;
    std::cout << "boolTest: " << boolTest << "\n";
    Parser boolParser = static_cast<Parser>(Option(&boolTest)["-b"]["--bool"]);
    boolParser.parseString("--bool trueasdfads");
    std::cout << "boolTest: " << boolTest << "\n";
}

static void StudentTest() {
    using namespace Argon;
    
    std::string name;
    int age;
    std::string major;

    std::string minor;
    std::string instrumentName;

    Parser parser = Option(&name)["--name"]
                    | OptionGroup()["--group"]
                        + Option(&age)["--age"]
                        + Option(&major)["--major"]
                        + (OptionGroup()["--nested"]
                            + Option(&minor)["--minor"]
                            + Option(&instrumentName)["--name"]);
    
    const std::string str = "--wtf true --name John --group [--age 20aasdf --major CS --nested [--minor music --name violin --null hi --null2 what] --back test] -top-level huh";
    // const std::string str = "--name John --outside asdfasd --group [--age 20aasdf --major CS --nested [--minor music --name violin --null hi] --back asdfas] --outter2 asdfas";
    // const std::string str = "--name John --group [--age 20asdf --major CS --nested [--minor music --name violin]]";
    parser.parseString(str);
    
    std::cout << "name: " << name << "\n";
    std::cout << "age: " << age << "\n";
    std::cout << "major: " << major << "\n";
    std::cout << "minor: " << minor << "\n";
    std::cout << "instrument: " << instrumentName << "\n";    
}

static void UnknownGroup() {
    using namespace Argon;

    std::string name;
    int age;
    std::string major;

    std::string minor;
    std::string instrumentName;
    
    Parser parser = Option(&name)["--name"]
                    | OptionGroup()["--group"]
                        + Option(&age)["--age"]
                        + Option(&major)["--major"]
                        + (OptionGroup()["--nested"]
                            + Option(&minor)["--minor"]
                            + Option(&instrumentName)["--name"]);


    const std::string input = "--name [-- what john] --unknowngroup [--what the --fuck ?]";
    parser.parseString(input);
}

static void MissingFlag() {
    using namespace Argon;

    std::string name;
    int age;
    std::string major;
    Parser parser = Option(&name)["--name"]
                    | Option(&age)["--age"]
                    | Option(&major)["--major"];

    const std::string input = "--name --age 10 --major music";
    parser.parseString(input);

    std::cout << std::format("Name: {}\n Age: {}\nMajor: {}\n", name, age, major);
}

static void MissingFlagNested() {
    using namespace Argon;

    std::string name;
    int age;
    std::string major;

    std::string minor;
    std::string instrumentName;
    
    Parser parser = Option(&name)["--name"]
                    | OptionGroup()["--group"]
                        + Option(&age)["--age"]
                        + Option(&major)["--major"]
                        + (OptionGroup()["--nested"]
                            + Option(&minor)["--minor"]
                            + Option(&instrumentName)["--name"]);


    const std::string input = "--name John --group [--age --major music]";
    parser.parseString(input);
    std::cout << "Name: " << name << "\n";
    std::cout << "Age: " << age << "\n";
    std::cout << "Major: " << major << "\n";
}

static void MultiOptionTest() {
    using namespace Argon;
    
    std::array<int, 3> intArr{};
    std::vector<double> doubleArr;

    Parser parser = MultiOption(&intArr)["-i"]["--ints"]
                  | MultiOption(&doubleArr)["-d"]["--doubles"];

    const std::string input = "--ints 1 2 3 --doubles 4.0 5.5 6.7";
    parser.parseString(input);

    std::cout << "Ints: " << intArr[0] << " " << intArr[1] << " " << intArr[2] << "\n";
    std::cout << "Doubles: " << doubleArr[0] << " " << doubleArr[1] << " " << doubleArr[2] << "\n";
}

static void MultiOptionGroupTest() {
    using namespace Argon;

    std::array<int, 3> intArr{};
    std::vector<double> doubleArr;

    Parser parser = MultiOption(&intArr)["-i"]["--ints"]
                    | OptionGroup()["--group"]
                        + MultiOption(&doubleArr)["-d"]["--doubles"];

    const std::string input = "--ints 1 2 3 --group [--doubles 4.0 5.5 6.7]";
    parser.parseString(input);

    std::cout << "Ints: " << intArr[0] << " " << intArr[1] << " " << intArr[2] << "\n";
    std::cout << "Doubles: " << doubleArr[0] << " " << doubleArr[1] << " " << doubleArr[2] << "\n";
}

static void GroupErrors() {
    using namespace Argon;

    std::string name;
    int age;
    std::string major;

    auto parser = Option(&name)["--name"]
                | OptionGroup()["--group"]
                    + Option(&age)["--age"];

    // Not an option group flag:
    const std::string notGroup = "--name [--age 10]";
    std::cout << "Not option group flag: \n";
    parser.parseString(notGroup);

    // Unknown flag:
    const std::string unknownFlag = "--name John --huh [--age 20]";
    std::cout << "----------------------------\n";
    std::cout << "Unknown flag: \n";
    parser.parseString(unknownFlag);

    // Missing LBRACK
    const std::string noLBRACK = "--name John --group --age 20]";
    std::cout << "----------------------------\n";
    std::cout << "No lbrack: \n";
    parser.parseString(noLBRACK);

    // Missing RBRACK
    const std::string noRBRACK = "--name John --group [--age 20 --major CS";
    std::cout << "----------------------------\n";
    std::cout << "No rbrack: \n";
    parser.parseString(noRBRACK);
}

static void BasicOption() {
    using namespace Argon;

    std::string name;
    int age;
    std::string major;

    auto parser = Option(&name)["--name"]
                | Option(&age)["--age"]
                | Option(&major)["--major"];

    const std::string input = "--name John --age 20 --major CS";
    parser.parseString(input);

    std::cout << "Name: " << name << "\n";
    std::cout << "Age: " << age << "\n";
    std::cout << "Major: " << major << "\n";
}

static void GetValue() {
    using namespace Argon;

    std::string name;
    int age;
    std::string major;

    auto parser = Option(&name)["--name"]
                | Option(&age)["--age"]
                | Option(&major)["--major"];

    const std::string input = "--name John --age 20 --major CS";
    parser.parseString(input);

    const auto nameOpt = parser.getValue<std::string>("--name");
    const auto ageOpt = parser.getValue<int>("--age");
    const auto majorOpt = parser.getValue<std::string>("--major");
    // const auto wrongOpt = parser.getValue<std::string>("--wrong", "what");

    if (nameOpt.has_value()) {
        std::cout << "Name: " << nameOpt.value() << "\n";
    }
    if (ageOpt.has_value()) {
        std::cout << "Age: " << ageOpt.value() << "\n";
    }
    if (majorOpt.has_value()) {
        std::cout << "Major: " << majorOpt.value() << "\n";
    }
}

static void GetValueNested() {
    using namespace Argon;

    std::string one;
    std::string two;
    std::string three;
    std::string four;

    auto parser = Option(&one)["--one"]
                | (OptionGroup()["--g1"]
                    + Option(&two)["--two"]
                    + (OptionGroup()["--g2"]
                        + Option(&three)["--three"]
                        + (OptionGroup()["--g3"]
                            + Option(&four)["--four"])));

    const std::string input = "--one 1 --g1 [--two 2 --g2 [--three 3 --g3 [--four 4]]]";
    parser.parseString(input);


    const auto oneOpt = parser.getValue<std::string>("--one");
    const auto twoOpt = parser.getValue<std::string>("--g1", "--two");
    const auto threeOpt = parser.getValue<std::string>("--g1", "--g2", "--three");
    const auto fourOpt = parser.getValue<std::string>("--g1", "--g2", "--g3", "--four");

    if (oneOpt.has_value()) {
        std::cout << "One: " << oneOpt.value() << "\n";
    }
    if (twoOpt.has_value()) {
        std::cout << "Two: " << twoOpt.value() << "\n";
    }
    if (threeOpt.has_value()) {
        std::cout << "Three: " << threeOpt.value() << "\n";
    }
    if (fourOpt.has_value()) {
        std::cout << "Four: " << fourOpt.value() << "\n";
    }
}

static void GetValueMultiOption() {
    using namespace Argon;

    std::array<int, 3> ints{};
    std::vector<float> floats{};

    auto parser = MultiOption(&ints)["--ints"]
                | MultiOption(&floats)["--floats"];

    const std::string input = "--ints 1 2 3 --floats 1.5 2.5 3.5";
    parser.parseString(input);

    const auto intsOpt = parser.getMultiValue<std::array<int, 3>>("--ints");
    const auto floatsOpt = parser.getMultiValue<std::vector<float>>("--floats");

    if (intsOpt.has_value()) {
        std::cout << "Ints: \n";
        for (const int x : ints) {
            std::cout << x << "\n";
        }
    }

    if (floatsOpt.has_value()) {
        std::cout << "Floats: \n";
        for (const float f : floats) {
            std::cout << f << "\n";
        }
    }
}

static void GetMultiValueNested() {
    using namespace Argon;

    std::array<int, 3> one{};
    std::array <float, 3> two{};
    std::vector<float> three{};
    std::vector<float> four{};

    auto parser = MultiOption(&one)["--one"]
                | (OptionGroup()["--g1"]
                    + MultiOption(&two)["--two"]
                    + (OptionGroup()["--g2"]
                        + MultiOption(&three)["--three"]
                        + (OptionGroup()["--g3"]
                            + MultiOption(&four)["--four"])));

    const std::string input = "--one 1 10 100 1000 --g1 [--two 2.0 2.2 2.22 --g2 [--three 1.5 2.5 --g3 [--four 4.5 5.5 6.5 7.5 8.5]]]";
    parser.parseString(input);


    const auto oneOpt = parser.getMultiValue<std::array<int, 3>>("--one");
    const auto twoOpt = parser.getMultiValue<std::array<float ,3>>("--g1", "--two");
    const auto threeOpt = parser.getMultiValue<std::vector<float>>("--g1", "--g2", "--three");
    const auto fourOpt = parser.getMultiValue<std::vector<float>>("--g1", "--g2", "--g3", "--four");

    if (oneOpt.has_value()) {
        std::cout << "One: \n";
        const auto& oneArr = oneOpt.value();
        for (const auto x : oneArr) {
            std::cout << x << "\n";
        }
    }
    if (twoOpt.has_value()) {
        std::cout << "Two: \n";
        const auto& twoArr = twoOpt.value();
        for (const auto x : twoArr) {
            std::cout << x << "\n";
        }
    }
    if (threeOpt.has_value()) {
        std::cout << "Three: \n";
        const auto& threeVec = threeOpt.value();
        for (const auto x : threeVec) {
            std::cout << x << "\n";
        }
    }
    if (fourOpt.has_value()) {
        std::cout << "Four: \n";
        const auto& fourVec = fourOpt.value();
        for (const auto x : fourVec) {
            std::cout << x << "\n";
        }
    }
}

static void LValue() {
    using namespace Argon;

    std::string name;
    int age;
    std::string major;

    auto nameOption = Option(&name)["--name"];

    auto parser = Parser();
    parser.addOption(Option(&age)["--age"]);
    parser.addOption(nameOption);

    const std::string input = "--name John --age 20 --major CS";
    parser.parseString(input);

    std::cout << "Name: " << name << "\n";
    std::cout << "Age: " << age << "\n";
    std::cout << "Major: " << major << "\n";
}

int main() {
    const auto start = std::chrono::steady_clock::now();
    using namespace Argon;
    // MissingFlag();
    // MissingFlagNested();
    // UnknownGroup();
    // StudentTest();
    // MultiOptionTest();
    // MultiOptionGroupTest();
    // runScannerTests();
    // runOptionsTests();
    // runErrorTests();
    // GroupErrors();
    // BasicOption();
    // GetValue();
    // GetValueNested();
    // GetValueMultiOption();
    // GetMultiValueNested();
    LValue();
    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time: " << duration << "\n";
    return 0;
}
