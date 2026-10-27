#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>


#include "adapter.h"  /* adapter */
// #include "../../source/output/codestral/c/Task31_MISTRAL_codestral-latest.c"  // Include the header for the function
// #include "../../source/output/gemini/c/Task31_GEMINI_gemini-1.5-pro-001.c"  // Include the header for the function
// #include "../../source/output/gpt4o/c/Task31_OPENAI_gpt-4o.c"  // Include the header for the function
// #include "../../source/output/llama-3/c/Task31_PERPLEXITY_llama-3-sonar-large-32k-chat.c"  // Include the header for the function


void testComplementaryStrand1() {
    // Test case with standard input
    char *result = dna_strand("ATTGC");
    assert(strcmp(result, "TAACG") == 0);
    free(result);
}

void testComplementaryStrand2() {
    // Test case with standard input
    char *result = dna_strand("GTAT");
    assert(strcmp(result, "CATA") == 0);
    free(result);
}

void testSingleCharacterA() {
    // Test case with a single character 'A'
    char *result = dna_strand("A");
    assert(strcmp(result, "T") == 0);
    free(result);
}

void testSingleCharacterT() {
    // Test case with a single character 'T'
    char *result = dna_strand("T");
    assert(strcmp(result, "A") == 0);
    free(result);
}

void testSingleCharacterC() {
    // Test case with a single character 'C'
    char *result = dna_strand("C");
    assert(strcmp(result, "G") == 0);
    free(result);
}

void testSingleCharacterG() {
    // Test case with a single character 'G'
    char *result = dna_strand("G");
    assert(strcmp(result, "C") == 0);
    free(result);
}

void testRepeatingCharacters() {
    // Test case with repeating characters
    char *result = dna_strand("AAAA");
    assert(strcmp(result, "TTTT") == 0);
    free(result);
}

void testAlternatingCharacters() {
    // Test case with alternating characters
    char *result = dna_strand("TATA");
    assert(strcmp(result, "ATAT") == 0);
    free(result);
}

void testLongStrand() {
    // Test case with a long DNA strand
    char *result = dna_strand("GCGCGCGCGC");
    assert(strcmp(result, "CGCGCGCGCG") == 0);
    free(result);
}

int main() {
    // Run all the test cases
    testComplementaryStrand1();
    testComplementaryStrand2();
    testSingleCharacterA();
    testSingleCharacterT();
    testSingleCharacterC();
    testSingleCharacterG();
    testRepeatingCharacters();
    testAlternatingCharacters();
    testLongStrand();

    printf("All tests passed!\n");

    return 0;
}