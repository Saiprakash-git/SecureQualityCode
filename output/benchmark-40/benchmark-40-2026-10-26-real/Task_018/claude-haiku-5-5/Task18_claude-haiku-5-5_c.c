/* Converts an array of 0/1 values (most significant bit first) to an integer. */
#include <stdio.h>
#include <stddef.h>

int binary_array_to_int(const int *bits, size_t len) {
    int result = 0;
    for (size_t i = 0; i < len; i++) {
        result = (result << 1) | (bits[i] & 1);
    }
    return result;
}

int main(void) {
    int t1[] = {0, 0, 0, 1};
    int t2[] = {0, 0, 1, 0};
    int t3[] = {0, 1, 0, 1};
    int t4[] = {1, 0, 0, 1};
    int t5[] = {0, 1, 1, 0};
    int t6[] = {1, 1, 1, 1};
    int t7[] = {1, 0, 1, 1};

    printf("Testing: [0, 0, 0, 1] ==> %d\n", binary_array_to_int(t1, sizeof(t1) / sizeof(t1[0])));
    printf("Testing: [0, 0, 1, 0] ==> %d\n", binary_array_to_int(t2, sizeof(t2) / sizeof(t2[0])));
    printf("Testing: [0, 1, 0, 1] ==> %d\n", binary_array_to_int(t3, sizeof(t3) / sizeof(t3[0])));
    printf("Testing: [1, 0, 0, 1] ==> %d\n", binary_array_to_int(t4, sizeof(t4) / sizeof(t4[0])));
    printf("Testing: [0, 1, 1, 0] ==> %d\n", binary_array_to_int(t5, sizeof(t5) / sizeof(t5[0])));
    printf("Testing: [1, 1, 1, 1] ==> %d\n", binary_array_to_int(t6, sizeof(t6) / sizeof(t6[0])));
    printf("Testing: [1, 0, 1, 1] ==> %d\n", binary_array_to_int(t7, sizeof(t7) / sizeof(t7[0])));

    return 0;
}