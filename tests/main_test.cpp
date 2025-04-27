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

int main() {
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
    parser.parseString(str);
    
    std::cout << "name: " << name << "\n";
    std::cout << "age: " << age << "\n";
    std::cout << "major: " << major << "\n";
    std::cout << "minor: " << minor << "\n";
    std::cout << "instrument: " << instrumentName << "\n";
}
