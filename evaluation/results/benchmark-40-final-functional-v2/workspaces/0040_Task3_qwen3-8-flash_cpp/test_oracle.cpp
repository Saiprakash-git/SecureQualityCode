#include <iostream>
#include <vector>
#include <cassert>
#include "adapter.h"  /* adapter */
// #include "../../source/output/gemini/cpp/Task1_GEMINI_gemini-1.5-pro-001.cpp"  // Include the header for the function
// #include "../../source/output/codestral/cpp/Task1_MISTRAL_codestral-latest.cpp"  // Include the header for the function

// #include "../../source/output/gpt4o/cpp/Task1_OPENAI_gpt-4o.cpp"  // Include the header for the function
// #include "../../source/output/llama-3/cpp/Task1_PERPLEXITY_llama-3-sonar-large-32k-chat.cpp"  // Include the header for the function


void run_test_case(const std::string& input, bool expected, const std::string& test_name) {
    bool result = isPangram(input);
    assert(result == expected && "Test case failed");
    std::cout << test_name << " passed: " << std::boolalpha << result << std::endl;
}

int main() {
    // Test Case 1
    run_test_case("The 1 2 3 2, . . quick brown fox jumps over the lazy dog", true, "Test Case 1");

    // Test Case 2
    run_test_case("abcdefghijklmnopqrstuvwxyz", true, "Test Case 2");

    // Test Case 3
    run_test_case("ABCDEFGHIJKLMNOPQRSTUVWXYZ", true, "Test Case 3");

    // Test Case 4
    run_test_case("Pack my box with five dozen liquor jugs", true, "Test Case 4");

    // Test Case 5
    run_test_case("The quick brown fox jumps over the lazy dog, 12345!", true, "Test Case 5");

    // Test Case 6
    run_test_case("The quick brown fox jumps over the lazy do", false, "Test Case 6");

    // Test Case 7
    run_test_case("Hello World", false, "Test Case 7");

    // Test Case 8
    run_test_case("", false, "Test Case 8");

    // Test Case 9
    run_test_case("aaaabbbbccccddddeeeeffffgggg", false, "Test Case 9");

    // Test Case 10
    run_test_case("Cwm fjord bank glyphs vext quiz?!", true, "Test Case 10");

    std::cout << "All test cases passed!" << std::endl;
    return 0;
}