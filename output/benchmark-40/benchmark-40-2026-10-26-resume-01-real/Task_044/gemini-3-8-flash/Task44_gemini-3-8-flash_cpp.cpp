#include <iostream>
#include <string>
#include <limits>
#include <system_error>
#include <charconv>
#include <cctype>
#include <algorithm>

enum class OperationStatus {
    Success,
    Overflow,
    Underflow,
    DivisionByZero,
    InvalidInput
};

bool parse_safe_integer(const std::string& input, long long min_val, long long max_val, long long& out_value) {
    if (input.empty()) {
        return false;
    }

    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }

    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }

    if (start >= end) {
        return false;
    }

    std::string_view sv(input.data() + start, end - start);
    
    long long value = 0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), value);

    if (ec != std::errc() || ptr != sv.data() + sv.size()) {
        return false;
    }

    if (value < min_val || value > max_val) {
        return false;
    }

    out_value = value;
    return true;
}

bool sanitize_text(const std::string& input, size_t max_len, std::string& output) {
    if (input.empty() || input.length() > max_len) {
        return false;
    }

    std::string sanitized;
    sanitized.reserve(input.length());

    for (unsigned char ch : input) {
        if (std::isalnum(ch) || ch == ' ' || ch == '_' || ch == '-') {
            sanitized.push_back(static_cast<char>(ch));
        } else {
            return false;
        }
    }

    if (sanitized.empty()) {
        return false;
    }

    output = sanitized;
    return true;
}

OperationStatus safe_add(long long a, long long b, long long& result) {
    if ((b > 0 && a > std::numeric_limits<long long>::max() - b) ||
        (b < 0 && a < std::numeric_limits<long long>::min() - b)) {
        return OperationStatus::Overflow;
    }
    result = a + b;
    return OperationStatus::Success;
}

OperationStatus safe_multiply(long long a, long long b, long long& result) {
    if (a > 0) {
        if (b > 0) {
            if (a > std::numeric_limits<long long>::max() / b) return OperationStatus::Overflow;
        } else {
            if (b < std::numeric_limits<long long>::min() / a) return OperationStatus::Underflow;
        }
    } else {
        if (b > 0) {
            if (a < std::numeric_limits<long long>::min() / b) return OperationStatus::Underflow;
        } else {
            if (a != 0 && b < std::numeric_limits<long long>::max() / a) return OperationStatus::Overflow;
        }
    }
    result = a * b;
    return OperationStatus::Success;
}

OperationStatus safe_divide(long long a, long long b, long long& result) {
    if (b == 0) {
        return OperationStatus::DivisionByZero;
    }
    if (a == std::numeric_limits<long long>::min() && b == -1) {
        return OperationStatus::Overflow;
    }
    result = a / b;
    return OperationStatus::Success;
}

std::string read_bounded_line(std::istream& in_stream, size_t max_chars) {
    std::string line;
    char ch = '\0';
    while (line.size() < max_chars && in_stream.get(ch)) {
        if (ch == '\n') {
            break;
        }
        line.push_back(ch);
    }
    if (line.size() == max_chars && in_stream.peek() != '\n' && in_stream.peek() != EOF) {
        in_stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    return line;
}

int main() {
    constexpr size_t MAX_INPUT_LEN = 128;
    constexpr long long RANGE_MIN = -1000000000LL;
    constexpr long long RANGE_MAX = 1000000000LL;

    std::cout << "Enter username (alphanumeric, spaces, '-' or '_', max 32 chars): ";
    std::string raw_username = read_bounded_line(std::cin, MAX_INPUT_LEN);
    std::string sanitized_username;

    if (!sanitize_text(raw_username, 32, sanitized_username)) {
        std::cerr << "Error: Invalid username format or excessive length.\n";
        return 1;
    }

    std::cout << "Enter first integer [" << RANGE_MIN << " to " << RANGE_MAX << "]: ";
    std::string raw_first = read_bounded_line(std::cin, MAX_INPUT_LEN);
    long long val1 = 0;
    if (!parse_safe_integer(raw_first, RANGE_MIN, RANGE_MAX, val1)) {
        std::cerr << "Error: First value is outside allowed range or not a valid integer.\n";
        return 1;
    }

    std::cout << "Enter second integer [" << RANGE_MIN << " to " << RANGE_MAX << "]: ";
    std::string raw_second = read_bounded_line(std::cin, MAX_INPUT_LEN);
    long long val2 = 0;
    if (!parse_safe_integer(raw_second, RANGE_MIN, RANGE_MAX, val2)) {
        std::cerr << "Error: Second value is outside allowed range or not a valid integer.\n";
        return 1;
    }

    std::cout << "Enter operation (+, *, /): ";
    std::string raw_op = read_bounded_line(std::cin, MAX_INPUT_LEN);
    if (raw_op.size() != 1 || (raw_op[0] != '+' && raw_op[0] != '*' && raw_op[0] != '/')) {
        std::cerr << "Error: Invalid operation selected.\n";
        return 1;
    }

    char op = raw_op[0];
    long long result = 0;
    OperationStatus status = OperationStatus::InvalidInput;

    if (op == '+') {
        status = safe_add(val1, val2, result);
    } else if (op == '*') {
        status = safe_multiply(val1, val2, result);
    } else if (op == '/') {
        status = safe_divide(val1, val2, result);
    }

    switch (status) {
        case OperationStatus::Success:
            std::cout << "User: " << sanitized_username << "\n";
            std::cout << "Result of " << val1 << " " << op << " " << val2 << " = " << result << "\n";
            break;
        case OperationStatus::Overflow:
            std::cerr << "Error: Arithmetic overflow occurred.\n";
            return 1;
        case OperationStatus::Underflow:
            std::cerr << "Error: Arithmetic underflow occurred.\n";
            return 1;
        case OperationStatus::DivisionByZero:
            std::cerr << "Error: Division by zero is not permitted.\n";
            return 1;
        default:
            std::cerr << "Error: Operation failed.\n";
            return 1;
    }

    return 0;
}