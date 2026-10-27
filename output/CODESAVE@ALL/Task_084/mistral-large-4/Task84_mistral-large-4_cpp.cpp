#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstddef>

std::string generateSessionId(std::size_t length = 32) {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    std::uniform_int_distribution<unsigned int> dis(0, 15);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < length; ++i) {
        ss << dis(gen);
    }

    return ss.str();
}

int main() {
    std::string sessionId = generateSessionId();
    std::cout << sessionId << std::endl;
    return 0;
}