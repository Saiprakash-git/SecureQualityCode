#include <stdio.h>
#include <stddef.h>

unsigned int binary_array_to_number(const unsigned int *binary_array, size_t length) {
    unsigned int result = 0;
    for (size_t i = 0; i < length; i++) {
        result = (result << 1) | binary_array[i];
    }
    return result;
}

int main(void) {
    unsigned int test1[] = {0, 0, 0, 1};
    unsigned int test2[] = {0, 0, 1, 0};
    unsigned int test3[] = {0, 1, 0, 1};
    unsigned int test4[] = {1, 0, 0, 1};
    unsigned int test5[] = {0, 0, 1, 0};
    unsigned int test6[] = {0, 1, 1, 0};
    unsigned int test7[] = {1, 1, 1, 1};
    unsigned int test8[] = {1, 0, 1, 1};
    unsigned int test9[] = {1, 0, 1};
    unsigned int test10[] = {1, 1, 0, 1, 0};

    printf("Testing: [0, 0, 0, 1] ==> %u\n", binary_array_to_number(test1, 4));
    printf("Testing: [0, 0, 1, 0] ==> %u\n", binary_array_to_number(test2, 4));
    printf("Testing: [0, 1, 0, 1] ==> %u\n", binary_array_to_number(test3, 4));
    printf("Testing: [1, 0, 0, 1] ==> %u\n", binary_array_to_number(test4, 4));
    printf("Testing: [0, 0, 1, 0] ==> %u\n", binary_array_to_number(test5, 4));
    printf("Testing: [0, 1, 1, 0] ==> %u\n", binary_array_to_number(test6, 4));
    printf("Testing: [1, 1, 1, 1] ==> %u\n", binary_array_to_number(test7, 4));
    printf("Testing: [1, 0, 1, 1] ==> %u\n", binary_array_to_number(test8, 4));
    printf("Testing: [1, 0, 1] ==> %u\n", binary_array_to_number(test9, 3));
    printf("Testing: [1, 1, 0, 1, 0] ==> %u\n", binary_array_to_number(test10, 5));

    return 0;
}