#include <string>

std::string DNAStrand(const std::string& dna) {
    std::string complement;
    complement.reserve(dna.size());

    for (char base : dna) {
        switch (base) {
            case 'A': complement += 'T'; break;
            case 'T': complement += 'A'; break;
            case 'C': complement += 'G'; break;
            case 'G': complement += 'C'; break;
        }
    }

    return complement;
}