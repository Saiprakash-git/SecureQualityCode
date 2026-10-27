#include <stdio.h>
#include <ctype.h>

int is_pangram(const char *text)
{
    unsigned int letters = 0;

    for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
        if (isalpha(*p)) {
            letters |= 1u << (tolower(*p) - 'a');
        }
    }

    return letters == (1u << 26) - 1;
}

int main(void)
{
    char text[100000];

    if (fgets(text, sizeof(text), stdin) == NULL) {
        return 0;
    }

    puts(is_pangram(text) ? "True" : "False");
    return 0;
}