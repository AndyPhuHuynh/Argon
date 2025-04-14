#include "testing.hpp"

#include <iostream>

#include "Argon/Argon.hpp"
#include "Argon/Option.hpp"

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
    
    runScannerTests();
    runOptionsTests();

    std::string name;
    int age;
    std::string major;

    // Parser parser = Option(&name)["--name"]
    //                 | Option(&age)["--age"]
    //                 | Option(&major)["--major"];

    Parser parser = Option(&name)["--name"]
                    | OptionGroup()["--group"]
                        + Option(&age)["--age"]
                        + Option(&major)["--major"];
    
    std::string str = "--name John --group [--age 20 --major CS]";
    parser.parseString(str);
    
    std::cout << "name: " << name << "\n";
    std::cout << "age: " << age << "\n";
    std::cout << "major: " << major << "\n";
}
