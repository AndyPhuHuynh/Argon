#pragma once
#include <iostream>
#include <string>

inline int g_failedTests = 0;

inline void check(const bool condition, const std::string& testName) {
    if (condition) {
        std::cout << "\033[32m[PASS]\033[0m " << testName << '\n';
    } else {
        std::cout << "\033[31m[FAIL]\033[0m " << testName << '\n';
        g_failedTests++;
    }
}

inline int reportTestStatus() {
    if (g_failedTests > 0) {
        std::cout << g_failedTests << " test(s) failed." << '\n';
        return 1;
    } else {
        std::cout << "All tests passed!" << '\n';
        return 0;
    }
}

void runScannerTests();
void runOptionsTests();