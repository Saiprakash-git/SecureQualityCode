#include <iostream>
#include <string>

std::string DNAStrand(const std::string& dna) {
    std::string complement = dna;
    for (char& c : complement) {
        switch (c) {
            case 'A': c = 'T'; break;
            case 'T': c = 'A'; break;
            case 'C': c = 'G'; break;
            case 'G': c = 'C'; break;
            default: break;
        }
    }
    return complement;
}

int main() {
    std::cout << DNAStrand("ATTGC") << '\n';
    std::cout << DNAStrand("GTAT") << '\n';
    return 0;
}