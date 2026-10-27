#include <iostream>
#include <limits>
#include <cstring>
#include <cstddef>

enum class InputResult {
    Success,
    Truncated,
    Empty,
    EndOfFile,
    Error
};

InputResult readLineToBuffer(std::istream& input_stream, char* buffer, std::size_t max_size) {
    if (!buffer || max_size == 0) {
        return InputResult::Error;
    }

    buffer[0] = '\0';

    if (!input_stream.good()) {
        if (input_stream.eof()) {
            return InputResult::EndOfFile;
        }
        return InputResult::Error;
    }

    input_stream.getline(buffer, static_cast<std::streamsize>(max_size));

    if (input_stream.eof()) {
        if (buffer[0] == '\0') {
            return InputResult::EndOfFile;
        }
        return InputResult::Success;
    }

    if (input_stream.fail()) {
        if (!input_stream.bad()) {
            input_stream.clear();
            input_stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return InputResult::Truncated;
        }
        return InputResult::Error;
    }

    if (buffer[0] == '\0') {
        return InputResult::Empty;
    }

    return InputResult::Success;
}

const char* resultToString(InputResult result) {
    switch (result) {
        case InputResult::Success:
            return "Success";
        case InputResult::Truncated:
            return "Input exceeded buffer size and was safely truncated";
        case InputResult::Empty:
            return "Empty input received";
        case InputResult::EndOfFile:
            return "End-of-file reached";
        case InputResult::Error:
            return "Input stream error";
        default:
            return "Unknown status";
    }
}

int main() {
    constexpr std::size_t BUFFER_SIZE = 16;
    char fixed_buffer[BUFFER_SIZE];

    std::cout << "Fixed buffer size: " << BUFFER_SIZE << " bytes (max " << BUFFER_SIZE - 1 << " characters).\n";
    std::cout << "Enter text: ";

    InputResult status = readLineToBuffer(std::cin, fixed_buffer, BUFFER_SIZE);

    std::cout << "\nOperation Status: " << resultToString(status) << "\n";
    std::cout << "Stored Content:   \"" << fixed_buffer << "\"\n";
    std::cout << "Length Stored:    " << std::strlen(fixed_buffer) << " characters\n";

    return (status == InputResult::Error) ? 1 : 0;
}