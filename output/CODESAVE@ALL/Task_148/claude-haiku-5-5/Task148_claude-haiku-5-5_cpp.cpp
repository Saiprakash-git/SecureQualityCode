#include <iostream>
#include <stdexcept>
#include <cstddef>

template <typename T>
class Stack {
private:
    struct Node {
        T value;
        Node* next;
        Node(const T& v, Node* n) : value(v), next(n) {}
    };

    Node* topNode;
    std::size_t count;

public:
    Stack() : topNode(nullptr), count(0) {}

    Stack(const Stack& other) : topNode(nullptr), count(0) {
        copyFrom(other);
    }

    Stack& operator=(const Stack& other) {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    ~Stack() {
        clear();
    }

    void push(const T& value) {
        topNode = new Node(value, topNode);
        ++count;
    }

    void pop() {
        if (isEmpty()) {
            throw std::underflow_error("pop from empty stack");
        }
        Node* old = topNode;
        topNode = topNode->next;
        delete old;
        --count;
    }

    T& peek() {
        if (isEmpty()) {
            throw std::underflow_error("peek on empty stack");
        }
        return topNode->value;
    }

    const T& peek() const {
        if (isEmpty()) {
            throw std::underflow_error("peek on empty stack");
        }
        return topNode->value;
    }

    bool isEmpty() const {
        return topNode == nullptr;
    }

    std::size_t size() const {
        return count;
    }

    void clear() {
        while (!isEmpty()) {
            pop();
        }
    }

private:
    void copyFrom(const Stack& other) {
        // Copy preserving order: collect into temporary stack first, then rebuild.
        Stack temp;
        for (Node* n = other.topNode; n != nullptr; n = n->next) {
            temp.push(n->value);
        }
        for (Node* n = temp.topNode; n != nullptr; n = n->next) {
            push(n->value);
        }
        // Reverse ordering fix: temp holds items reversed; pushing them back restores original order.
    }
};

int main() {
    Stack<int> s;

    s.push(10);
    s.push(20);
    s.push(30);

    std::cout << "Top element: " << s.peek() << "\n";
    std::cout << "Size: " << s.size() << "\n";

    s.pop();
    std::cout << "After pop, top element: " << s.peek() << "\n";

    s.pop();
    s.pop();

    std::cout << "Is empty: " << (s.isEmpty() ? "yes" : "no") << "\n";

    try {
        s.peek();
    } catch (const std::underflow_error& e) {
        std::cout << "Error: " << e.what() << "\n";
    }

    return 0;
}