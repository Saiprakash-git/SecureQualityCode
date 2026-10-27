#include <string>

std::string DNAStrand(const std::string& dna) {
    std::string result;
    result.reserve(dna.size());

    for (char c : dna) {
        switch (c) {
            case 'A': result += 'T'; break;
            case 'T': result += 'A'; break;
            case 'C': result += 'G'; break;
            case 'G': result += 'C'; break;
            default: result += c; break;
        }
    }

    return result;
}