#include <iostream>
#include <string>
#include <limits>
#include <cctype>
#include <algorithm>

bool isValidUsername(const std::string& username) {
    if (username.empty() || username.length() > 32) {
        return false;
    }
    for (char c : username) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    return true;
}

bool isValidAge(int age) {
    return age >= 0 && age <= 150;
}

std::string sanitizeInput(const std::string& input) {
    std::string result;
    result.reserve(input.length());
    for (char c : input) {
        if (std::isprint(static_cast<unsigned char>(c))) {
            result += c;
        }
    }
    return result;
}

std::string reverseString(const std::string& input) {
    std::string result = input;
    std::reverse(result.begin(), result.end());
    return result;
}

int countVowels(const std::string& input) {
    int count = 0;
    for (char c : input) {
        char lower = std::tolower(static_cast<unsigned char>(c));
        if (lower == 'a' || lower == 'e' || lower == 'i' || lower == 'o' || lower == 'u') {
            count++;
        }
    }
    return count;
}

std::string getValidatedUsername() {
    std::string username;
    while (true) {
        std::cout << "Enter username (alphanumeric and underscore only, max 32 chars): ";
        if (!std::getline(std::cin, username)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Input error. Please try again.\n";
            continue;
        }

        if (username.length() > 100) {
            std::cout << "Input too long. Please limit to 100 characters.\n";
            continue;
        }

        if (isValidUsername(username)) {
            return username;
        } else {
            std::cout << "Invalid username. Use only letters, numbers, and underscores (max 32 chars).\n";
        }
    }
}

int getValidatedAge() {
    int age;
    while (true) {
        std::cout << "Enter age (0-150): ";
        if (!(std::cin >> age)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number.\n";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (isValidAge(age)) {
            return age;
        } else {
            std::cout << "Age must be between 0 and 150.\n";
        }
    }
}

std::string getValidatedText() {
    std::string text;
    while (true) {
        std::cout << "Enter text to process (max 500 chars): ";
        if (!std::getline(std::cin, text)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Input error. Please try again.\n";
            continue;
        }

        if (text.length() > 500) {
            std::cout << "Text too long. Maximum 500 characters allowed.\n";
            continue;
        }

        return sanitizeInput(text);
    }
}

void displayMenu() {
    std::cout << "\nOperations Menu:\n";
    std::cout << "1. Reverse text\n";
    std::cout << "2. Count vowels\n";
    std::cout << "3. Convert to uppercase\n";
    std::cout << "4. Exit\n";
    std::cout << "Choice: ";
}

int getValidatedChoice() {
    int choice;
    while (true) {
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Please enter a number (1-4): ";
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice >= 1 && choice <= 4) {
            return choice;
        } else {
            std::cout << "Please enter a number between 1 and 4: ";
        }
    }
}

std::string toUpperCase(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

int main() {
    std::cout << "=== Secure Input Processing Program ===\n";

    std::string username = getValidatedUsername();
    int age = getValidatedAge();

    std::cout << "\nWelcome, " << username << "! You are " << age << " years old.\n";

    while (true) {
        displayMenu();
        int choice = getValidatedChoice();

        if (choice == 4) {
            std::cout << "Goodbye, " << username << "!\n";
            break;
        }

        std::string text = getValidatedText();

        switch (choice) {
            case 1:
                std::cout << "Reversed: " << reverseString(text) << "\n";
                break;
            case 2:
                std::cout << "Vowel count: " << countVowels(text) << "\n";
                break;
            case 3:
                std::cout << "Uppercase: " << toUpperCase(text) << "\n";
                break;
        }
    }

    return 0;
}