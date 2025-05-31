#include <array>

#include <chrono>
#include <iostream>

#include "Argon/MultiOption.hpp"
#include "Argon/Option.hpp"
#include "Argon/Parser.hpp"

#include "catch2/catch_all.hpp"

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


int main2() {
    const auto start = std::chrono::steady_clock::now();
    using namespace Argon;
    // MissingFlag();
    // MissingFlagNested();
    // UnknownGroup();
    // GroupErrors();
    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time: " << duration << "\n";
    return 0;
}