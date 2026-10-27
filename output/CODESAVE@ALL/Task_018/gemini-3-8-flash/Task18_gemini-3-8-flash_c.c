#include <stdio.h>
#include <stddef.h>

unsigned binary_array_to_num(const unsigned *bits, size_t count) {
    unsigned result = 0;
    for (size_t i = 0; i < count; i++) {
        result = (result << 1) | bits[i];
    }
    return result;
}

int main(void) {
    const unsigned test1[] = {0, 0, 0, 1};
    const unsigned test2[] = {0, 0, 1, 0};
    const unsigned test3[] = {0, 1, 0, 1};
    const unsigned test4[] = {1, 0, 0, 1};
    const unsigned test5[] = {0, 1, 1, 0};
    const unsigned test6[] = {1, 1, 1, 1};
    const unsigned test7[] = {1, 0, 1, 1};

    printf("%u\n", binary_array_to_num(test1, sizeof(test1) / sizeof(test1[0])));
    printf("%u\n", binary_array_to_num(test2, sizeof(test2) / sizeof(test2[0])));
    printf("%u\n", binary_array_to_num(test3, sizeof(test3) / sizeof(test3[0])));
    printf("%u\n", binary_array_to_num(test4, sizeof(test4) / sizeof(test4[0])));
    printf("%u\n", binary_array_to_num(test5, sizeof(test5) / sizeof(test5[0])));
    printf("%u\n", binary_array_to_num(test6, sizeof(test6) / sizeof(test6[0])));
    printf("%u\n", binary_array_to_num(test7, sizeof(test7) / sizeof(test7[0])));

    return 0;
}