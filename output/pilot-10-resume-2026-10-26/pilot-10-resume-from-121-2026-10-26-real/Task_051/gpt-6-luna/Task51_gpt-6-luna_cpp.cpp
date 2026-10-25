#include <iostream>
#include <string>
using namespace std;

string encrypt(const string& text, int shift) {
    shift = ((shift % 26) + 26) % 26;
    string result = text;

    for (char& ch : result) {
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>('a' + (ch - 'a' + shift) % 26);
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>('A' + (ch - 'A' + shift) % 26);
        }
    }

    return result;
}

int main() {
    string text;
    getline(cin, text);
    cout << encrypt(text, 3) << '\n';
    return 0;
}