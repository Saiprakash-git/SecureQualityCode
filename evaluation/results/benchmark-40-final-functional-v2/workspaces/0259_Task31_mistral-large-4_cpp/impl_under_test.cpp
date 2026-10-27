#include <iostream>
#include <string>

std::string dnaComplement(const std::string& dna) {
    std::string result;
    result.reserve(dna.size());
    for (char c : dna) {
        switch (c) {
            case 'A': result.push_back('T'); break;
            case 'T': result.push_back('A'); break;
            case 'C': result.push_back('G'); break;
            case 'G': result.push_back('C'); break;
        }
    }
    return result;
}

int main() {
    std::string input;
    std::getline(std::cin, input);
    std::cout << dnaComplement(input) << std::endl;
    return 0;
}