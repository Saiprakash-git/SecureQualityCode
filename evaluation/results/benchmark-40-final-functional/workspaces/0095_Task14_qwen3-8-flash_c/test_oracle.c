#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>


#include "adapter.h"  /* adapter */
// #include "../../source/output/codestral/c/Task14_MISTRAL_codestral-latest.c"  // Include the header for the function
// #include "../../source/output/gemini/c/Task14_GEMINI_gemini-1.5-pro-001.c"  // Include the header for the function
// #include "../../source/output/gpt4o/c/Task14_OPENAI_gpt-4o.c"  // Include the header for the function
// #include "../../source/output/llama-3/c/Task14_PERPLEXITY_llama-3-sonar-large-32k-chat.c"  // Include the header for the function
// Helper function to run a test case





// Helper function to run and assert test cases
void assertTestCase(const int *arr, int length, int expected, const char *testName) {
    int result = findEvenIndex(arr, length);
    if (result == expected) {
        printf("[PASS] %s\n", testName);
    } else {
        printf("[FAIL] %s: Expected %d, got %d\n", testName, expected, result);
    }
}

int main() {
    // Test cases
    int arr1[] = {1, 2, 3, 4, 3, 2, 1};
    assertTestCase(arr1, 7, 3, "testCase1");

    int arr2[] = {1, 100, 50, -51, 1, 1};
    assertTestCase(arr2, 6, 1, "testCase2");

    int arr3[] = {20, 10, -80, 10, 10, 15, 35};
    assertTestCase(arr3, 7, 0, "testCase3");

    int arr4[] = {1, 2, 3, 4, 5, 6};
    assertTestCase(arr4, 6, -1, "testCase4");

    int arr5[] = {10, -10, 5, 5, 5};
    assertTestCase(arr5, 5, 3, "testCase5");

    int arr6[] = {-1, -2, -3, -4, -3, -2, -1};
    assertTestCase(arr6, 7, 3, "testCase6");

    int arr7[] = {0, 0, 0, 0, 0};
    assertTestCase(arr7, 5, 0, "testCase7");

    int arr8[] = {10, 20, 30, 40, 50, 60};
    assertTestCase(arr8, 6, -1, "testCase8");

    int arr9[] = {1, 1, 1, 1, 1, 1, 1, 6};
    assertTestCase(arr9, 8, 6, "testCase9");

    int arr10[] = {1, 0, -1, 0, 1, 0, -1, 0, 1, 0, -1, 0};
    assertTestCase(arr10, 12, 3, "testCase10");

    printf("\nAll test cases completed.\n");

    return 0;
}