#include <iostream>
#include <vector>
#include <cassert>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cassert>
#include <iostream>
#include <vector>
#include <cassert>

#include "adapter.h"  /* adapter */
// #include "../../source/output/codestral/cpp/Task14_MISTRAL_codestral-latest.cpp"  // Include the header for the function
// #include "../../source/output/gemini/cpp/Task14_GEMINI_gemini-1.5-pro-001.cpp"  // Include the header for the function
// #include "../../source/output/gpt4o/cpp/Task14_OPENAI_gpt-4o.cpp"  // Include the header for the function
// #include "../../source/output/llama-3/cpp/Task14_PERPLEXITY_llama-3-sonar-large-32k-chat.cpp"  // Include the header for the function


// Helper function to run and assert test cases
void assertTestCase(const std::vector<int>& arr, int expected, const std::string& testName) {
    Solution solution;
    int result = solution.findEvenIndex(arr);
    if (result == expected) {
        std::cout << "[PASS] " << testName << std::endl;
    } else {
        std::cout << "[FAIL] " << testName << ": Expected " << expected << ", got " << result << std::endl;
    }
}

int main() {
    // Test cases
    assertTestCase({1, 2, 3, 4, 3, 2, 1}, 3, "testCase1");
    assertTestCase({1, 100, 50, -51, 1, 1}, 1, "testCase2");
    assertTestCase({20, 10, -80, 10, 10, 15, 35}, 0, "testCase3");
    assertTestCase({1, 2, 3, 4, 5, 6}, -1, "testCase4");
    assertTestCase({10, -10, 5, 5, 5}, 3, "testCase5");
    assertTestCase({-1, -2, -3, -4, -3, -2, -1}, 3, "testCase6");
    assertTestCase({0, 0, 0, 0, 0}, 0, "testCase7");
    assertTestCase({10, 20, 30, 40, 50, 60}, -1, "testCase8");
    assertTestCase({1, 1, 1, 1, 1, 1, 1, 6}, 6, "testCase9");
    assertTestCase({1, 0, -1, 0, 1, 0, -1, 0, 1, 0, -1, 0}, 3, "testCase10");

    std::cout << "\nAll test cases completed." << std::endl;

    return 0;
}