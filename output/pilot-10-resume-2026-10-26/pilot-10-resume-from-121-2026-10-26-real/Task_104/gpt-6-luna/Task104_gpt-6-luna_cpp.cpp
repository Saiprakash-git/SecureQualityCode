#include <cstddef>
#include <iostream>

struct ReadResult {
    std::size_t length;
    bool truncated;
    bool receivedInput;
};

ReadResult readFixedLine(std::istream& input, char* buffer, std::size_t capacity) {
    std::size_t length = 0;
    bool truncated = false;
    bool receivedInput = false;
    char character;

    while (input.get(character)) {
        receivedInput = true;

        if (character == '\n') {
            break;
        }

        if (capacity > 0 && length < capacity - 1) {
            buffer[length++] = character;
        } else {
            truncated = true;
        }
    }

    if (capacity > 0) {
        buffer[length] = '\0';
    }

    return {length, truncated, receivedInput};
}

int main() {
    constexpr std::size_t bufferSize = 256;
    char buffer[bufferSize];

    const ReadResult result = readFixedLine(std::cin, buffer, bufferSize);
    if (!result.receivedInput) {
        return 0;
    }

    std::cout.write(buffer, static_cast<std::streamsize>(result.length));
    std::cout.put('\n');

    if (result.truncated) {
        std::cerr << "Input was too long and has been truncated.\n";
    }

    return 0;
}