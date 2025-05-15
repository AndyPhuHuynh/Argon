# Argon - Command Line Parser Library for C++

Argon is a lightweight and easy-to-use C++ library for parsing command line arguments. It is designed to be flexible and
make it simple to define and process options in command line tools.

## Features

- Flexible composability of options into one parser
- Group options together under a local scope
- Supports options that take multiple arguments
- Support for both built-in and user defined types
- Specify custom conversion functions
- Customizable error messages for improper conversions
- Extensive error messages for malformed syntax 
- Specify custom flags

## Usage

### Basic option

A parser for a basic option can be created as follows:
```c++
std::string name = "John";
auto parser = Argon::Parser(Argon::Option(&name)["-n"]["--name"]);
```
You can specify which flags refer to the option via the index operator. 

Pass in the address of the variable where you want the given value to be stored as the first argument of the Option constructor.
The option will automatically detect the type of the given variable and will try to parse the input appropriately.

With only one option, you have to explicitly cast the option into a parser.

---

Parsing an input string can be done as follows:
```c++
std::string name = "John";
auto parser = Argon::Parser(Argon::Option(&name)["-n"]["--name"]);

const std::string input = "--name Bob";
parser.parseString(input); // Name is "Bob" here

const std::string input2 = "-n Josh";
parser.parseString(input2); // Name is "Josh" here
```

---

### Combining options

You can combine options together with the | operator:
```c++
std::string name = "John";
int age = 0;
float salary = 0.0f;

auto parser = Argon::Option(&name)["-n"]["--name"]
            | Argon::Option(&age)["-a"]["--age"]
            | Argon::Option(&salary)["-s"]["--salary"];
const std::string input = "--name Bob -a 20 --salary 15.5";

parser.parseString(input);
// name = "Bob", age = 20, salary = 15.5
```

---

### User Defined Data Types

You can also create custom conversion functions that work with user defined data types:
```c++
struct Student {
    std::string name = "Default";
    int age = 0;
};

auto studentConversionFn = [](const std::string& str, Student& out) -> bool {
    if (str == "1") { 
        out = { .name = "Josh", .age = 1 };
        return true;
    } else if (str == "2") { 
        out = { .name = "Sally", .age = 2 };
        return true;
    } 
    return false;
};

Student student1, student2;
auto parser = Argon::Option<Student>(&student1, studentConversionFn)["--first"]
            | Argon::Option<Student>(&student2, studentConversionFn)["--second"];
            
const std::string input = "--first 1 --second 3";
parser.parseString(input);
// student1 = { .name = "Josh", .age = 1 }
// student2 remains in the default state and a default error message is logged
```
The custom conversion function can be any callable object. It should have two arguments: the string value that will be
parsed into the specified type and an out value of the specified type that will be written to. The function needs to
return a boolean that is true when conversion is successful and false when conversion fails.

A default error message is logged when the conversion fails:

> [Analysis Errors]<br>
> └── Invalid value for flag '--second': expected unknown type, got: 3

---

### Custom Error Messages

It is possible to specify a function to generate a custom error message as well:
```c++
auto studentErrorFn = [](const std::string& flag, const std::string& invalidArg) -> std::string {
    return std::format("Invalid value for flag '{}': expected either '1' or '2'", flag);
};  

Student student1, student2;
auto parser = Argon::Option<Student>(&student1, studentConversionFn, studentErrorFn)["--first"]
            | Argon::Option<Student>(&student2, studentConversionFn, studentErrorFn)["--second"];
            
const std::string input = "--first 1 --second 3";
parser.parseString(input);
// student1 = { .name = "Josh", .age = 1 }
// student2 remains in the default state and the custom error message is logged
```

The error message generator function can be any callable object. It should have two arguments which are both const 
references to std::strings. The first string represents the flag the user specified and the seconds string is the invalid
argument. The function should return an std::string which is the appropriate error message.

The custom error will display as follows:
> [Analysis Errors]<br>
> └── Invalid value for flag 'second': expected either '1' or '2'

---

### Multi Argument Options

Users can specify an option to take multiple arguments. You must specify a std::array or std::vector with your desired
data type. With an array, the maximum value of arguments the user may supply is the size of the array. With a vector,
a user can supply any number of arguments.

```c++
std::string school;
std::vector<Student> students;

auto parser = Argon::Option(&school)["--school"]
            | Argon::MultiOption(&students, studentConversionFn, studentErrorFn)["--students"];
            
const std::string input = "--students 1 2 --school University";
```
Above, the value of school will be "University" and the students vector will contain two students: Josh and Sally

---

### Grouping Options Together

You may group options together so that their flags are contained in a local scope

```c++
std::string globalName;
Student student1, student2;

auto parser = Argon::Option(&globalName)["-n"]["--name"]
            | Argon::OptionGroup()["--student1"]
                + Argon::Option(&student1.name)["-n"]["--name"]
                + Argon::Option(&student1.age)["-a"]["--age"]
            | Argon::OptionGroup()["--student2"]
                + Argon::Option(&student2.name)["-n"]["--name"]
                + Argon::Option(&student2.age)["-a"]["--age"]
                
const std::string input = "--name global --student1 [--name Josh --age 1] --student2 [--name Sally --age 2]"
```