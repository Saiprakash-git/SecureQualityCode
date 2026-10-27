#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>


#include "adapter.h"  /* adapter */
// #include "../../source/output/codestral/c/Task9_MISTRAL_codestral-latest.c"  // Include the header for the function
// #include "../../source/output/gemini/c/Task9_GEMINI_gemini-1.5-pro-001.c"  // Include the header for the function
// #include "../../source/output/gpt4o/c/Task9_OPENAI_gpt-4o.c"  // Include the header for the function
// #include "../../source/output/llama-3/c/Task9_PERPLEXITY_llama-3-sonar-large-32k-chat.c"  // Include the header for the function
// Helper function to run a test case



// Function prototype
const char* odd_or_even(int arr[], int size);

void testEmptyArray() {
    int arr[] = {};
    assert(strcmp(odd_or_even(arr, 0), "even") == 0);
}

void testSinglePositiveOdd() {
    int arr[] = {7};
    assert(strcmp(odd_or_even(arr, 1), "odd") == 0);
}

void testSinglePositiveEven() {
    int arr[] = {8};
    assert(strcmp(odd_or_even(arr, 1), "even") == 0);
}

void testMultiplePositives() {
    int arr[] = {1, 3, 5, 7};
    assert(strcmp(odd_or_even(arr, 4), "even") == 0);
}

void testMultiplePositivesOddSum() {
    int arr[] = {1, 2, 3, 4};
    assert(strcmp(odd_or_even(arr, 4), "even") == 0);
}

void testMultipleNegatives() {
    int arr[] = {-2, -4, -6};
    assert(strcmp(odd_or_even(arr, 3), "even") == 0);
}

void testMultipleNegativesOddSum() {
    int arr[] = {-1, -2, -3};
    assert(strcmp(odd_or_even(arr, 3), "even") == 0);
}

void testMixedNumbersEvenSum() {
    int arr[] = {1, -1, 2, -2};
    assert(strcmp(odd_or_even(arr, 4), "even") == 0);
}

void testMixedNumbersOddSum() {
    int arr[] = {1, -1, 2};
    assert(strcmp(odd_or_even(arr, 3), "even") == 0);
}

void testLargeNumbers() {
    int arr[] = {1000000, 2000000, -3000000};
    assert(strcmp(odd_or_even(arr, 3), "even") == 0);
}

int main() {
    testEmptyArray();
    testSinglePositiveOdd();
    testSinglePositiveEven();
    testMultiplePositives();
    testMultiplePositivesOddSum();
    testMultipleNegatives();
    testMultipleNegativesOddSum();
    testMixedNumbersEvenSum();
    testMixedNumbersOddSum();
    testLargeNumbers();

    printf("All test cases passed!\n");
    return 0;
}


