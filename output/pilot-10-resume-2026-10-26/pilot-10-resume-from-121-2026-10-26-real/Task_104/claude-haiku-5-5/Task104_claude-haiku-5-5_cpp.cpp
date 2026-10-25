#include <iostream>
#include <cstring>
#include <string>

const size_t BUFFER_SIZE = 64;

size_t readIntoBuffer(char* buffer, size_t size, const std::string& input) {
    if (buffer == nullptr || size == 0) {
        return 0;
    }
    size_t len = input.size();
    if (len >= size) {
        len = size - 1;
    }
    std::memcpy(buffer, input.data(), len);
    buffer[len] = '\0';
    return len;
}

bool wasTruncated(const std::string& input, size_t size) {
    return size == 0 ? !input.empty() : input.size() >= size;
}

int main() {
    char buffer[BUFFER_SIZE];
    std::string line;

    std::cout << "Enter text (max " << (BUFFER_SIZE - 1) << " characters):\n";
    if (!std::getline(std::cin, line)) {
        std::cerr << "No input received.\n";
        return 1;
    }

    size_t stored = readIntoBuffer(buffer, BUFFER_SIZE, line);

    if (wasTruncated(line, BUFFER_SIZE)) {
        std::cout << "Warning: input was truncated.\n";
    }

    std::cout << "Stored (" << stored << " chars): " << buffer << "\n";
    return 0;
}