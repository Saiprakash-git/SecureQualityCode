#include <iostream>
#include <optional>
#include <string>
#include <vector>

class Stack {
public:
    void push(int value) {
        elements_.push_back(value);
    }

    std::optional<int> pop() {
        if (elements_.empty()) {
            return std::nullopt;
        }

        int value = elements_.back();
        elements_.pop_back();
        return value;
    }

    std::optional<int> peek() const {
        if (elements_.empty()) {
            return std::nullopt;
        }

        return elements_.back();
    }

    bool empty() const {
        return elements_.empty();
    }

    std::size_t size() const {
        return elements_.size();
    }

private:
    std::vector<int> elements_;
};

int main() {
    Stack stack;
    std::string operation;

    while (std::cin >> operation) {
        if (operation == "push") {
            int value;
            if (std::cin >> value) {
                stack.push(value);
            }
        } else if (operation == "pop") {
            const auto value = stack.pop();
            if (value) {
                std::cout << *value << '\n';
            } else {
                std::cout << "EMPTY\n";
            }
        } else if (operation == "peek") {
            const auto value = stack.peek();
            if (value) {
                std::cout << *value << '\n';
            } else {
                std::cout << "EMPTY\n";
            }
        }
    }

    return 0;
}