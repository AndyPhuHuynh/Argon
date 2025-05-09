#include "testing.hpp"

#include <iostream>

#include "Argon/Option.hpp"
#include "Argon/Parser.hpp" 

static void boolTest() {
    using namespace Argon;
    
    bool boolTest = true;
    std::cout << "boolTest: " << boolTest << "\n";
    Parser boolParser = Option(&boolTest)["-b"]["--bool"];
    boolParser.parseString("--bool trueasdfads");
    std::cout << "boolTest: " << boolTest << "\n";
}

static void StudentTest() {
    using namespace Argon;

    // std::string name;
    // int age;
    // std::string major;
    //
    // Parser parser = Option(&name)["--name"]
    //                 | Option(&age)["--age"]
    //                 | Option(&major)["--major"];
    //
    // std::string input = "--one 1 --two 2 --three 3 --four 4";
    // parser.parseString(input);
    //
    // return 0;
    // runErrorTests();
    // runScannerTests();
    // runOptionsTests();
    
    std::string name;
    int age;
    std::string major;

    std::string minor;
    std::string instrumentName;
    
    // Parser parser = Option(&name)["--name"]
    //                 | Option(&age)["--age"]
    //                 | Option(&major)["--major"];

    Parser parser = Option(&name)["--name"]
                    | OptionGroup()["--group"]
                        + Option(&age)["--age"]
                        + Option(&major)["--major"]
                        + (OptionGroup()["--nested"]
                            + Option(&minor)["--minor"]
                            + Option(&instrumentName)["--name"]);
    
    std::string str = "--wtf true --name John --group [--age 20aasdf --major CS --nested [--minor music --name violin --null hi --null2 what] --back test] -top-level huh";
    // std::string str = "--name John --outside asdfasd --group [--age 20aasdf --major CS --nested [--minor music --name violin --null hi] --back asdfas] --outter2 asdfas";
    std::string working = "--name John --group [--age 20asdf --major CS --nested [--minor music --name violin]]";
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

    
    std::string input = "--name [-- what john] --unknowngroup [--what the --fuck ?]";
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

    std::string input = "--name --age 10 --major music";
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

    
    std::string input = "--name John --group [--age --major music]";
    parser.parseString(input);
    std::cout << "Name: " << name << "\n";
    std::cout << "Age: " << age << "\n";
    std::cout << "Major: " << major << "\n";
}

int main() {
    using namespace Argon;
    // MissingFlag();
    // MissingFlagNested();
    // UnknownGroup();
    StudentTest();
    // runErrorTests();
    return 0;
    std::string name;
    int age;
    
    Parser parser = Option(&name)["--name"]
                    | OptionGroup()["--group"]
                        + Option(&age)["--age"];

    // std::string input = "--name John --group [--age 20";
    std::string input = "[]";
    // parser.parseString(input);
}
