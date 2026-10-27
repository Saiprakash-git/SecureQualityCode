#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>


// #include "../../source/output/claude/c/Task25_CLAUDE_claude-3-5-sonnet-20240620.c"  // Include the header for the function
// #include "../../source/output/codestral/c/Task25_MISTRAL_codestral-latest.c"  // Include the header for the function
// #include "../../source/output/gemini/c/Task25_GEMINI_gemini-1.5-pro-001.c"  // Include the header for the function
#include "adapter.h"  /* adapter */
// #include "../../source/output/llama-3/c/Task25_PERPLEXITY_llama-3-sonar-large-32k-chat.c"  // Include the header for the function



void testNumbering() {
    // Test Case 1
    const char* input1[] = {};
    size_t input1Count = 0;
    char** result1 = number(input1, input1Count);
    assert(result1 == NULL || input1Count==0);
    printf("Test Case 1 passed!\n");

    // Test Case 2
    const char* input2[] = {"Hello"};
    size_t input2Count = 1;
    char* expected2[] = {"1: Hello"};
    char** result2 = number(input2, input2Count);
    for (size_t i = 0; i < input2Count; i++) {
        assert(strcmp(result2[i], expected2[i]) == 0);
    }
    //freeResult(result2, input2Count);
    printf("Test Case 2 passed!\n");

    // Test Case 3
    const char* input3[] = {"Hello", "World"};
    size_t input3Count = 2;
    char* expected3[] = {"1: Hello", "2: World"};
    char** result3 = number(input3, input3Count);
    for (size_t i = 0; i < input3Count; i++) {
        assert(strcmp(result3[i], expected3[i]) == 0);
    }
    //freeResult(result3, input3Count);
    printf("Test Case 3 passed!\n");

    // Test Case 4
    const char* input4[] = {"a", "b", "c"};
    size_t input4Count = 3;
    char* expected4[] = {"1: a", "2: b", "3: c"};
    char** result4 = number(input4, input4Count);
    for (size_t i = 0; i < input4Count; i++) {
        assert(strcmp(result4[i], expected4[i]) == 0);
    }
    //freeResult(result4, input4Count);
    printf("Test Case 4 passed!\n");

    // Test Case 5
    const char* input5[] = {"The issue here is that the pow function is not defined in the standard C library for all platforms. ", "While it's commonly available on most systems, there might be specific configurations or environments where it's not included by default."};
    size_t input5Count = 2;
    char* expected5[] = {"1: The issue here is that the pow function is not defined in the standard C library for all platforms. ", "2: While it's commonly available on most systems, there might be specific configurations or environments where it's not included by default."};
    char** result5 = number(input5, input5Count);
    for (size_t i = 0; i < input5Count; i++) {
        assert(strcmp(result5[i], expected5[i]) == 0);
    }
    //freeResult(result5, input5Count);
    printf("Test Case 5 passed!\n");

    // Test Case 6
    const char* input6[] = {"Line 1", "", "Line 3"};
    size_t input6Count = 3;
    char* expected6[] = {"1: Line 1", "2: ", "3: Line 3"};
    char** result6 = number(input6, input6Count);
    for (size_t i = 0; i < input6Count; i++) {
        assert(strcmp(result6[i], expected6[i]) == 0);
    }
    //freeResult(result6, input6Count);
    printf("Test Case 6 passed!\n");

    // Test Case 7
    const char* input7[] = {"!@#$%^", "&*()_+", "[]{}"};
    size_t input7Count = 3;
    char* expected7[] = {"1: !@#$%^", "2: &*()_+", "3: []{}"};
    char** result7 = number(input7, input7Count);
    for (size_t i = 0; i < input7Count; i++) {
        assert(strcmp(result7[i], expected7[i]) == 0);
    }
    //freeResult(result7, input7Count);
    printf("Test Case 7 passed!\n");

    // Test Case 8
    const char* input8[] = {"123", "Your team is writing a fancy new text editor and you've been tasked with implementing the line. Notice the colon and space in between.", "789"};
    size_t input8Count = 3;
    char* expected8[] = {"1: 123", "2: Your team is writing a fancy new text editor and you've been tasked with implementing the line. Notice the colon and space in between.", "3: 789"};
    char** result8 = number(input8, input8Count);
    for (size_t i = 0; i < input8Count; i++) {
        assert(strcmp(result8[i], expected8[i]) == 0);
    }
    //freeResult(result8, input8Count);
    printf("Test Case 8 passed!\n");

    // Test Case 9
    const char* input9[] = {"  leading the gun with his staff so why to wait more, its the time to run out. go, wait, its too early guys lets set down", "trailing  ", "  both  "};
    size_t input9Count = 3;
    char* expected9[] = {"1:   leading the gun with his staff so why to wait more, its the time to run out. go, wait, its too early guys lets set down", "2: trailing  ", "3:   both  "};
    char** result9 = number(input9, input9Count);
    for (size_t i = 0; i < input9Count; i++) {
        assert(strcmp(result9[i], expected9[i]) == 0);
    }
    //freeResult(result9, input9Count);
    printf("Test Case 9 passed!\n");

    // Test Case 10
    size_t input10Count = 1000;
    const char** input10 = (const char**)malloc(input10Count * sizeof(char*));
    char** expected10 = (char**)malloc(input10Count * sizeof(char*));
    for (size_t i = 0; i < input10Count; ++i) {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "line %zu", i);
        input10[i] = strdup(buffer);
        snprintf(buffer, sizeof(buffer), "%zu: line %zu", i + 1, i);
        expected10[i] = strdup(buffer);
    }
    char** result10 = number(input10, input10Count);
    for (size_t i = 0; i < input10Count; i++) {
        assert(strcmp(result10[i], expected10[i]) == 0);
        free((void*)input10[i]);
        free(expected10[i]);
    }
    free(input10);
    free(expected10);
    //freeResult(result10, input10Count);
    printf("Test Case 10 passed!\n");
}

int main() {
    testNumbering();
    printf("All test cases passed!\n");
    return 0;
}

