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
// #include "../../source/output/codestral/cpp/Task18_MISTRAL_codestral-latest.cpp"  // Include the header for the function
// #include "../../source/output/gemini/cpp/Task18_GEMINI_gemini-1.5-pro-001.cpp"  // Include the header for the function
// #include "../../source/output/gpt4o/cpp/Task18_OPENAI_gpt-4o.cpp"  // Include the header for the function
// #include "../../source/output/llama-3/cpp/Task18_PERPLEXITY_llama-3-sonar-large-32k-chat.cpp"  // Include the header for the function


void testConvertBinaryArrayToInt() {
    assert(convertBinaryArrayToInt({0, 0, 0, 1}) == 1); // 0001 = 1
    assert(convertBinaryArrayToInt({0, 0, 1, 0}) == 2); // 0010 = 2
    assert(convertBinaryArrayToInt({0, 1, 0, 1}) == 5); // 0101 = 5
    assert(convertBinaryArrayToInt({1, 0, 0, 1}) == 9); // 1001 = 9
    assert(convertBinaryArrayToInt({0, 1, 1, 0}) == 6); // 0110 = 6
    assert(convertBinaryArrayToInt({1, 1, 1, 1}) == 15); // 1111 = 15
    assert(convertBinaryArrayToInt({1, 0, 1, 1}) == 11); // 1011 = 11
    assert(convertBinaryArrayToInt({1}) == 1); // Single bit 1
    assert(convertBinaryArrayToInt({0}) == 0); // Single bit 0
    assert(convertBinaryArrayToInt({0, 1, 0, 1, 0, 1, 1}) == 43); // 0101011 = 43

    std::cout << "All test cases passed.\n";
}

int main() {
    testConvertBinaryArrayToInt();
    return 0;
}