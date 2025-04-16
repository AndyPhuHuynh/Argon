﻿#include "testing.hpp"

#include "Argon/Error.hpp"

static void MessageTest1() {
    using namespace Argon;
    ErrorGroup root;

    root.addErrorMessage("one", 1);
    root.addErrorMessage("three", 3);
    root.addErrorMessage("two", 2);

    auto errors = root.getErrors();

    bool success = true;

    if (errors.size() != 3) {
        success = false;
    }

    if (std::holds_alternative<ErrorMessage>(errors[0])) {
        auto error = std::get<ErrorMessage>(errors[0]);
        success = error.msg == "one" && error.pos == 1;
    } else {
        success = false;
    }

    if (std::holds_alternative<ErrorMessage>(errors[1])) {
        auto error = std::get<ErrorMessage>(errors[1]);
        success = error.msg == "two" && error.pos == 2;
    } else {
        success = false;
    }

    if (std::holds_alternative<ErrorMessage>(errors[2])) {
        auto error = std::get<ErrorMessage>(errors[2]);
        success = error.msg == "three" && error.pos == 3;
    } else {
        success = false;
    }

    check(success, "Error Messages Test 1");
}

static void GroupTest1() {
    using namespace Argon;
    ErrorGroup root;

    root.addErrorMessage("one", 1);
    root.addErrorGroup("GroupOne",20, 30);
    root.addErrorMessage("25", 25);
    root.addErrorMessage("21", 21);
    root.addErrorMessage("35", 35);

    auto errors = root.getErrors();

    bool success = true;

    if (errors.size() != 3) {
        success = false;
    }

    if (std::holds_alternative<ErrorMessage>(errors[0])) {
        auto error = std::get<ErrorMessage>(errors[0]);
        success = error.msg == "one" && error.pos == 1;
    } else {
        success = false;
    }

    auto group = std::get<ErrorGroup>(errors[1]);
    success = group.getGroupName() == "GroupOne";

    const auto& groupErrors = group.getErrors();
    if (std::holds_alternative<ErrorMessage>(groupErrors[0])) {
        auto error = std::get<ErrorMessage>(groupErrors[0]);
        success = error.msg == "21" && error.pos == 21;
    } else {
        success = false;
    }

    if (std::holds_alternative<ErrorMessage>(groupErrors[1])) {
        auto error = std::get<ErrorMessage>(groupErrors[1]);
        success = error.msg == "25" && error.pos == 25;
    } else {
        success = false;
    }

    if (std::holds_alternative<ErrorMessage>(errors[2])) {
        auto error = std::get<ErrorMessage>(errors[2]);
        success = error.msg == "35" && error.pos == 35;
    } else {
        success = false;
    }
    
    check(success, "Error Group Test 1");
}

void runErrorTests() {
    MessageTest1();
    GroupTest1();
}