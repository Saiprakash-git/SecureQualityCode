#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>


#include "adapter.h"  /* adapter */
// #include "../../source/output/codestral/c/Task23_MISTRAL_codestral-latest.c"  // Include the header for the function
// #include "../../source/output/gemini/c/Task23_GEMINI_gemini-1.5-pro-001.c"  // Include the header for the function
// #include "../../source/output/gpt4o/c/Task23_OPENAI_gpt-4o.c"  // Include the header for the function
// #include "../../source/output/llama-3/c/Task23_PERPLEXITY_llama-3-sonar-large-32k-chat.c"  // Include the header for the function





int main() {
    // Test Case 1
    long long m1 = 1071225;
    long long expected1 = 45;
    assert(findNb(m1) == expected1);
    printf("Test Case 1 passed!\n");

    // Test Case 2
    long long m2 = 91716553919377LL;
    long long expected2 = -1;
    assert(findNb(m2) == expected2);
    printf("Test Case 2 passed!\n");

    // Test Case 3
    long long m3 = 1;
    long long expected3 = 1;
    assert(findNb(m3) == expected3);
    printf("Test Case 3 passed!\n");

    // Test Case 4
    long long m4 = 9;
    long long expected4 = 2;
    assert(findNb(m4) == expected4);
    printf("Test Case 4 passed!\n");

    // Test Case 5
    long long m5 = 36;
    long long expected5 = 3;
    assert(findNb(m5) == expected5);
    printf("Test Case 5 passed!\n");

    // Test Case 6
    long long m6 = 1000;
    long long expected6 = -1;
    assert(findNb(m6) == expected6);
    printf("Test Case 6 passed!\n");

    // Test Case 7
    long long m7 = 2025;
    long long expected7 = 9;
    assert(findNb(m7) == expected7);
    printf("Test Case 7 passed!\n");

    // Test Case 8
    long long m8 = 1000000;
    long long expected8 = -1;
    assert(findNb(m8) == expected8);
    printf("Test Case 8 passed!\n");

    // Test Case 9
    long long m9 = 2552550;
    long long expected9 = -1;
    assert(findNb(m9) == expected9);
    printf("Test Case 9 passed!\n");

    // Test Case 10
    long long m10 = 3375;
    long long expected10 = -1;
    assert(findNb(m10) == expected10);
    printf("Test Case 10 passed!\n");

    printf("All test cases passed!\n");

    return 0;
}

