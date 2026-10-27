#include <string>

bool is_pangram(const std::string& s) {
    bool seen[26] = {false};
    int remaining = 26;

    for (char ch : s) {
        char lower = ch;

        if (lower >= 'A' && lower <= 'Z') {
            lower = char(lower - 'A' + 'a');
        }

        if (lower >= 'a' && lower <= 'z') {
            int index = lower - 'a';

            if (!seen[index]) {
                seen[index] = true;
                --remaining;

                if (remaining == 0) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool isPangram(const std::string& s) {
    return is_pangram(s);
}