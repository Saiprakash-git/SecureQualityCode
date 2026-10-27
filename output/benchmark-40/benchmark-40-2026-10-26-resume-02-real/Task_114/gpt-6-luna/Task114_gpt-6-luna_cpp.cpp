#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

using namespace std;

string copyString(const string& text) {
    return text;
}

string concatenateStrings(const string& first, const string& second) {
    return first + second;
}

string reverseString(const string& text) {
    string result = text;
    reverse(result.begin(), result.end());
    return result;
}

string toUppercase(const string& text) {
    string result = text;
    transform(result.begin(), result.end(), result.begin(),
              [](unsigned char character) {
                  return static_cast<char>(toupper(character));
              });
    return result;
}

string toLowercase(const string& text) {
    string result = text;
    transform(result.begin(), result.end(), result.begin(),
              [](unsigned char character) {
                  return static_cast<char>(tolower(character));
              });
    return result;
}

string getSubstring(const string& text, size_t start, size_t length) {
    if (start >= text.size()) {
        return "";
    }
    return text.substr(start, length);
}

string removeWhitespace(const string& text) {
    string result;
    for (unsigned char character : text) {
        if (!isspace(character)) {
            result += static_cast<char>(character);
        }
    }
    return result;
}

int readInteger(const string& prompt) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) {
            return -1;
        }

        stringstream input(line);
        int value;
        char extra;
        if (input >> value && !(input >> extra)) {
            return value;
        }

        cout << "Please enter a valid integer.\n";
    }
}

int main() {
    while (true) {
        cout << "\nString Operations\n"
             << "1. Copy a string\n"
             << "2. Concatenate two strings\n"
             << "3. Reverse a string\n"
             << "4. Convert to uppercase\n"
             << "5. Convert to lowercase\n"
             << "6. Extract a substring\n"
             << "7. Remove whitespace\n"
             << "8. Find string length\n"
             << "0. Exit\n";

        int choice = readInteger("Choose an operation: ");
        if (choice == 0 || choice == -1) {
            break;
        }

        string text;
        switch (choice) {
            case 1:
                cout << "Enter a string: ";
                getline(cin, text);
                cout << "Copied string: " << copyString(text) << '\n';
                break;

            case 2: {
                string first, second;
                cout << "Enter the first string: ";
                getline(cin, first);
                cout << "Enter the second string: ";
                getline(cin, second);
                cout << "Concatenated string: "
                     << concatenateStrings(first, second) << '\n';
                break;
            }

            case 3:
                cout << "Enter a string: ";
                getline(cin, text);
                cout << "Reversed string: " << reverseString(text) << '\n';
                break;

            case 4:
                cout << "Enter a string: ";
                getline(cin, text);
                cout << "Uppercase string: " << toUppercase(text) << '\n';
                break;

            case 5:
                cout << "Enter a string: ";
                getline(cin, text);
                cout << "Lowercase string: " << toLowercase(text) << '\n';
                break;

            case 6: {
                cout << "Enter a string: ";
                getline(cin, text);
                int start = readInteger("Enter the starting index (0-based): ");
                int length = readInteger("Enter the substring length: ");
                if (start < 0 || length < 0) {
                    cout << "Index and length must be non-negative.\n";
                } else {
                    cout << "Substring: "
                         << getSubstring(text, static_cast<size_t>(start),
                                         static_cast<size_t>(length))
                         << '\n';
                }
                break;
            }

            case 7:
                cout << "Enter a string: ";
                getline(cin, text);
                cout << "String without whitespace: "
                     << removeWhitespace(text) << '\n';
                break;

            case 8:
                cout << "Enter a string: ";
                getline(cin, text);
                cout << "String length: " << text.size() << '\n';
                break;

            default:
                cout << "Invalid choice. Please try again.\n";
        }
    }

    return 0;
}