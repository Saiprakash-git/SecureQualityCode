#include <iostream>
#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

std::string generateAuthToken(size_t byteLength, const std::string& prefix) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned int> dis(0, 255);

    std::stringstream ss;
    ss << prefix;

    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    ss << timestamp << "_";

    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < byteLength; ++i) {
        ss << std::setw(2) << dis(gen);
    }

    return ss.str();
}

int main() {
    std::string token = generateAuthToken(32, "auth_");
    std::cout << token << std::endl;
    return 0;
}