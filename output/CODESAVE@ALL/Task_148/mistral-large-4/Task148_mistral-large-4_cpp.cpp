#include <iostream>
#include <stdexcept>

class Stack {
private:
    int* data;
    int capacity;
    int topIndex;

public:
    Stack(int size = 100) : capacity(size), topIndex(-1) {
        data = new int[capacity];
    }

    ~Stack() {
        delete[] data;
    }

    void push(int value) {
        if (topIndex >= capacity - 1) {
            throw std::overflow_error("Stack overflow");
        }
        data[++topIndex] = value;
    }

    int pop() {
        if (isEmpty()) {
            throw std::underflow_error("Stack underflow");
        }
        return data[topIndex--];
    }

    int peek() const {
        if (isEmpty()) {
            throw std::underflow_error("Stack is empty");
        }
        return data[topIndex];
    }

    bool isEmpty() const {
        return topIndex == -1;
    }
};

int main() {
    Stack s;
    s.push(5);
    s.push(10);
    s.push(15);

    std::cout << s.peek() << std::endl;
    std::cout << s.pop() << std::endl;
    std::cout << s.peek() << std::endl;

    return 0;
}