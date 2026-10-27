#include <iostream>
#include <array>
#include <cstddef>

std::size_t read_into_fixed_buffer(std::istream& input, char* buffer, std::size_t buffer_size)
{
    if (buffer == nullptr || buffer_size == 0) {
        return 0;
    }

    std::size_t count = 0;

    while (count + 1 < buffer_size) {
        int ch = input.get();

        if (!input) {
            break;
        }

        if (ch == '\n' || ch == '\r') {
            break;
        }

        buffer[count] = static_cast<char>(ch);
        ++count;
    }

    buffer[count] = '\0';
    return count;
}

int main()
{
    std::array<char, 128> buffer{};

    std::cout << "Enter text: ";

    std::size_t chars_read = read_into_fixed_buffer(std::cin, buffer.data(), buffer.size());

    std::cout << "Read " << chars_read << " character(s): " << buffer.data() << '\n';

    return 0;
}