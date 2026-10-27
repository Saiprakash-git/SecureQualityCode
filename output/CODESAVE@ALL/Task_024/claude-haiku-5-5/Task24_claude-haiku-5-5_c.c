#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

bool validate_pin(const char *pin) {
    if (pin == NULL) {
        return false;
    }
    size_t len = strlen(pin);
    if (len != 4 && len != 6) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)pin[i])) {
            return false;
        }
    }
    return true;
}

int main(void) {
    const char *tests[] = {"1234", "12345", "a234"};
    size_t count = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < count; i++) {
        printf("\"%s\"  -->  %s\n", tests[i], validate_pin(tests[i]) ? "true" : "false");
    }
    return 0;
}