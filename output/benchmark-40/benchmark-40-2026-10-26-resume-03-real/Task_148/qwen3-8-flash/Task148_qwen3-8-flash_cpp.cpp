#include <iostream>
#include <string>
#include <vector>

template <typename T>
class Stack {
private:
    std::vector<T> items;

public:
    void push(const T& value) {
        items.push_back(value);
    }

    bool pop(T& value) {
        if (items.empty()) {
            return false;
        }

        value = items.back();
        items.pop_back();
        return true;
    }

    bool peek(T& value) const {
        if (items.empty()) {
            return false;
        }

        value = items.back();
        return true;
    }
};

int main() {
    Stack<int> stack;
    std::string command;

    while (std::cin >> command) {
        if (command == "push") {
            int value;
            if (std::cin >> value) {
                stack.push(value);
            }
        } else if (command == "pop") {
            int value;
            if (stack.pop(value)) {
                std::cout << value << '\n';
            } else {
                std::cout << "empty\n";
            }
        } else if (command == "peek") {
            int value;
            if (stack.peek(value)) {
                std::cout << value << '\n';
            } else {
                std::cout << "empty\n";
            }
        }
    }

    return 0;
}