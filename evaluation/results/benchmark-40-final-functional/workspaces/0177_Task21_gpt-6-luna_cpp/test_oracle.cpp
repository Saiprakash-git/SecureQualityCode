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
#include <cstring>

#include "adapter.h"  /* adapter */
// #include "../../source/output/codestral/cpp/Task21_MISTRAL_codestral-latest.cpp"  // Include the header for the function
// #include "../../source/output/gemini/cpp/Task21_GEMINI_gemini-1.5-pro-001.cpp"  // Include the header for the function
// #include "../../source/output/gpt4o/cpp/Task21_OPENAI_gpt-4o.cpp"  // Include the header for the function
// #include "../../source/output/llama-3/cpp/Task21_PERPLEXITY_llama-3-sonar-large-32k-chat.cpp"  // Include the header for the function


void testCase1() {
    std::vector<int> input = {1, 2, 3, 4, 5};
    std::vector<int> expected = {2, 3, 4, 5};

    std::vector<int> result = removeSmallest(input);
    assert(result == expected);
    std::cout << "Test Case 1 passed!\n";
}

void testCase2() {
    std::vector<int> input = {5, 3, 2, 1, 4};
    std::vector<int> expected = {5, 3, 2, 4};

    std::vector<int> result = removeSmallest(input);
    assert(result == expected);
    std::cout << "Test Case 2 passed!\n";
}

void testCase3() {
    std::vector<int> input = {2, 2, 1, 2, 1};
    std::vector<int> expected = {2, 2, 2, 1};

    std::vector<int> result = removeSmallest(input);
    assert(result == expected);
    std::cout << "Test Case 3 passed!\n";
}

void testCase4() {
    std::vector<int> input = {1};
    std::vector<int> expected = {};

    std::vector<int> result = removeSmallest(input);
    assert(result == expected);
    std::cout << "Test Case 4 passed!\n";
}

void testCase5() {
    std::vector<int> input = {1, 1, 1, 1};
    std::vector<int> expected = {1, 1, 1};

    std::vector<int> result = removeSmallest(input);
    assert(result == expected);
    std::cout << "Test Case 5 passed!\n";
}

void testCase6() {
    std::vector<int> input = {};
    std::vector<int> expected = {};

    std::vector<int> result = removeSmallest(input);
    assert(result == expected);
    std::cout << "Test Case 6 passed!\n";
}

void testCase7() {
    std::vector<int> input = {2, 3, 2, 3, 1};
    std::vector<int> expected = {2, 3, 2, 3};

    std::vector<int> result = removeSmallest(input);
    assert(result == expected);
    std::cout << "Test Case 7 passed!\n";
}

void testCase8() {
    std::vector<int> input = {7, 7, 7, 7, 7};
    std::vector<int> expected = {7, 7, 7, 7};

    std::vector<int> result = removeSmallest(input);
    assert(result == expected);
    std::cout << "Test Case 8 passed!\n";
}

void testCase9() {
    std::vector<int> input = {5, 6, 7, 8, 9};
    std::vector<int> expected = {6, 7, 8, 9};

    std::vector<int> result = removeSmallest(input);
    assert(result == expected);
    std::cout << "Test Case 9 passed!\n";
}

void testCase10() {
    std::vector<int> input = {10, 10, 10};
    std::vector<int> expected = {10, 10};

    std::vector<int> result = removeSmallest(input);
    assert(result == expected);
    std::cout << "Test Case 10 passed!\n";
}

int main() {
    testCase1();
    testCase2();
    testCase3();
    testCase4();
    testCase5();
    testCase6();
    testCase7();
    testCase8();
    testCase9();
    testCase10();

    std::cout << "All tests passed!\n";
    return 0;
}