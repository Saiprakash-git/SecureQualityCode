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
// #include "../../source/output/codestral/cpp/Task31_MISTRAL_codestral-latest.cpp"  // Include the header for the function
// #include "../../source/output/gemini/cpp/Task31_GEMINI_gemini-1.5-pro-001.cpp"  // Include the header for the function
// #include "../../source/output/gpt4o/cpp/Task31_OPENAI_gpt-4o.cpp"  // Include the header for the function
//#include "../../source/output/llama-3/cpp/Task31_PERPLEXITY_llama-3-sonar-large-32k-chat.cpp"  // Include the header for the function


void testComplementaryStrand1() {
    // Test case with standard input
    assert(DNAStrand("ATTGC") == "TAACG");
}

void testComplementaryStrand2() {
    // Test case with standard input
    assert(DNAStrand("GTAT") == "CATA");
}

void testSingleCharacterA() {
    // Test case with a single character 'A'
    assert(DNAStrand("A") == "T");
}

void testSingleCharacterT() {
    // Test case with a single character 'T'
    assert(DNAStrand("T") == "A");
}

void testSingleCharacterC() {
    // Test case with a single character 'C'
    assert(DNAStrand("C") == "G");
}

void testSingleCharacterG() {
    // Test case with a single character 'G'
    assert(DNAStrand("G") == "C");
}

void testRepeatingCharacters() {
    // Test case with repeating characters
    assert(DNAStrand("AAAA") == "TTTT");
}

void testAlternatingCharacters() {
    // Test case with alternating characters
    assert(DNAStrand("TATA") == "ATAT");
}

void testLongStrand() {
    // Test case with a long DNA strand
    assert(DNAStrand("GCGCGCGCGC") == "CGCGCGCGCG");
}

void testMixedCase() {
    // Test case with a mixed-case DNA strand - note that this assumes input is always uppercase in your implementation
    // Since your function likely assumes correct casing, we do not explicitly test lower-case input here.
    assert(DNAStrand("ATTGC") == "TAACG");
}

int main() {
    // Running all the test cases
    testComplementaryStrand1();
    testComplementaryStrand2();
    testSingleCharacterA();
    testSingleCharacterT();
    testSingleCharacterC();
    testSingleCharacterG();
    testRepeatingCharacters();
    testAlternatingCharacters();
    testLongStrand();
    testMixedCase();

    std::cout << "All test cases passed!" << std::endl;
    return 0;
}