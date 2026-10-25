#include <iostream>
#include <cstring>

const size_t BUFFER_SIZE = 128;

bool readIntoBuffer(char* buffer, size_t bufferSize, size_t& bytesRead) {
    if (buffer == nullptr || bufferSize == 0) {
        bytesRead = 0;
        return false;
    }

    std::cin.getline(buffer, bufferSize);

    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        bytesRead = bufferSize - 1;
        return false;
    }

    bytesRead = std::strlen(buffer);
    return true;
}

void processBuffer(const char* buffer, size_t length) {
    std::cout << "Buffer contents: " << buffer << std::endl;
    std::cout << "Length: " << length << std::endl;
}

int main() {
    char fixedBuffer[BUFFER_SIZE];
    size_t bytesRead = 0;

    std::cout << "Enter input (max " << BUFFER_SIZE - 1 << " characters): ";

    bool success = readIntoBuffer(fixedBuffer, BUFFER_SIZE, bytesRead);

    if (success) {
        processBuffer(fixedBuffer, bytesRead);
    } else {
        std::cout << "Input truncated or error occurred. Read " << bytesRead << " characters." << std::endl;
        processBuffer(fixedBuffer, bytesRead);
    }

    return 0;
}