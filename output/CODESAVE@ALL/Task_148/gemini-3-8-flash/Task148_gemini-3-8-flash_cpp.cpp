#include <iostream>
#include <stdexcept>
#include <utility>
#include <cstddef>

template <typename T>
class Stack {
private:
    T* data;
    size_t capacity;
    size_t count;

    void resize(size_t newCapacity) {
        T* newData = new T[newCapacity];
        for (size_t i = 0; i < count; ++i) {
            newData[i] = std::move(data[i]);
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }

public:
    Stack() : data(nullptr), capacity(0), count(0) {}

    ~Stack() {
        delete[] data;
    }

    Stack(const Stack& other) : data(nullptr), capacity(other.capacity), count(other.count) {
        if (capacity > 0) {
            data = new T[capacity];
            for (size_t i = 0; i < count; ++i) {
                data[i] = other.data[i];
            }
        }
    }

    Stack& operator=(const Stack& other) {
        if (this != &other) {
            T* newData = nullptr;
            if (other.capacity > 0) {
                newData = new T[other.capacity];
                for (size_t i = 0; i < other.count; ++i) {
                    newData[i] = other.data[i];
                }
            }
            delete[] data;
            data = newData;
            capacity = other.capacity;
            count = other.count;
        }
        return *this;
    }

    Stack(Stack&& other) noexcept : data(other.data), capacity(other.capacity), count(other.count) {
        other.data = nullptr;
        other.capacity = 0;
        other.count = 0;
    }

    Stack& operator=(Stack&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            capacity = other.capacity;
            count = other.count;
            other.data = nullptr;
            other.capacity = 0;
            other.count = 0;
        }
        return *this;
    }

    void push(const T& value) {
        if (count == capacity) {
            size_t newCap = capacity == 0 ? 4 : capacity * 2;
            resize(newCap);
        }
        data[count++] = value;
    }

    void push(T&& value) {
        if (count == capacity) {
            size_t newCap = capacity == 0 ? 4 : capacity * 2;
            resize(newCap);
        }
        data[count++] = std::move(value);
    }

    T pop() {
        if (isEmpty()) {
            throw std::underflow_error("Stack underflow: Cannot pop from an empty stack.");
        }
        return std::move(data[--count]);
    }

    const T& peek() const {
        if (isEmpty()) {
            throw std::underflow_error("Stack underflow: Cannot peek into an empty stack.");
        }
        return data[count - 1];
    }

    T& peek() {
        if (isEmpty()) {
            throw std::underflow_error("Stack underflow: Cannot peek into an empty stack.");
        }
        return data[count - 1];
    }

    bool isEmpty() const {
        return count == 0;
    }

    size_t size() const {
        return count;
    }
};

int main() {
    Stack<int> s;

    s.push(10);
    s.push(20);
    s.push(30);

    std::cout << "Stack size: " << s.size() << "\n";
    std::cout << "Top element (peek): " << s.peek() << "\n";

    std::cout << "Popped: " << s.pop() << "\n";
    std::cout << "Top element after pop: " << s.peek() << "\n";

    s.push(40);
    std::cout << "Pushed 40. Top element: " << s.peek() << "\n";

    while (!s.isEmpty()) {
        std::cout << "Popped: " << s.pop() << "\n";
    }

    try {
        s.pop();
    } catch (const std::underflow_error& e) {
        std::cout << "Caught expected exception: " << e.what() << "\n";
    }

    return 0;
}