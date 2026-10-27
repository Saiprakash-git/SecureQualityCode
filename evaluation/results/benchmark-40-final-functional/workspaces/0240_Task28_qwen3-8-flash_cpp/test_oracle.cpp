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
// #include "../../source/output/codestral/cpp/Task28_MISTRAL_codestral-latest.cpp"  // Include the header for the function
// #include "../../source/output/gemini/cpp/Task28_GEMINI_gemini-1.5-pro-001.cpp"  // Include the header for the function
// #include "../../source/output/gpt4o/cpp/Task28_OPENAI_gpt-4o.cpp"  // Include the header for the function
// #include "../../source/output/llama-3/cpp/Task28_PERPLEXITY_llama-3-sonar-large-32k-chat.cpp"  // Include the header for the function



void testNoLikes() {
    assert(likes({}) == "no one likes this");
}

void testOneLike() {
    assert(likes({"Peter"}) == "Peter likes this");
}

void testTwoLikes() {
    assert(likes({"Jacob", "Alex"}) == "Jacob and Alex like this");
}

void testThreeLikes() {
    assert(likes({"Max", "John", "Mark"}) == "Max, John and Mark like this");
}

void testFourLikes() {
    assert(likes({"Alex", "Jacob", "Mark", "Max"}) == "Alex, Jacob and 2 others like this");
}

void testFiveLikes() {
    assert(likes({"John", "Paul", "George", "Ringo", "Brian"}) == "John, Paul and 3 others like this");
}

void testSixLikes() {
    assert(likes({"Alice", "Bob", "Charlie", "Dave", "Eve", "Frank"}) == "Alice, Bob and 4 others like this");
}

void testNamesWithSpaces() {
    assert(likes({"Anna Marie", "John Doe", "Jane Smith", "Joe Public"}) == "Anna Marie, John Doe and 2 others like this");
}

void testDuplicateNames() {
    assert(likes({"Sam", "Sam", "Alex"}) == "Sam, Sam and Alex like this");
}

void testLongArray() {
    assert(likes({"Tom", "Dick", "Harry", "Sally", "Jane", "Susan", "Bill", "Bob"}) == "Tom, Dick and 6 others like this");
}

// Main function to run all tests
int main() {
    testNoLikes();
    testOneLike();
    testTwoLikes();
    testThreeLikes();
    testFourLikes();
    testFiveLikes();
    testSixLikes();
    testNamesWithSpaces();
    testDuplicateNames();
    testLongArray();

    std::cout << "All test cases passed!" << std::endl;
    return 0;
}