#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

template <typename T>
class Queue {
private:
    struct Node {
        T data;
        Node* next;
        explicit Node(const T& value) : data(value), next(nullptr) {}
        explicit Node(T&& value) : data(std::move(value)), next(nullptr) {}
    };

    Node* frontNode;
    Node* backNode;
    std::size_t count;

public:
    Queue() : frontNode(nullptr), backNode(nullptr), count(0) {}

    Queue(const Queue& other) : frontNode(nullptr), backNode(nullptr), count(0) {
        for (Node* cur = other.frontNode; cur != nullptr; cur = cur->next) {
            enqueue(cur->data);
        }
    }

    Queue& operator=(const Queue& other) {
        if (this != &other) {
            Queue temp(other);
            swap(temp);
        }
        return *this;
    }

    Queue(Queue&& other) noexcept
        : frontNode(other.frontNode), backNode(other.backNode), count(other.count) {
        other.frontNode = nullptr;
        other.backNode = nullptr;
        other.count = 0;
    }

    Queue& operator=(Queue&& other) noexcept {
        if (this != &other) {
            clear();
            frontNode = other.frontNode;
            backNode = other.backNode;
            count = other.count;
            other.frontNode = nullptr;
            other.backNode = nullptr;
            other.count = 0;
        }
        return *this;
    }

    ~Queue() {
        clear();
    }

    void swap(Queue& other) noexcept {
        std::swap(frontNode, other.frontNode);
        std::swap(backNode, other.backNode);
        std::swap(count, other.count);
    }

    void enqueue(const T& value) {
        Node* node = new Node(value);
        if (backNode) {
            backNode->next = node;
        } else {
            frontNode = node;
        }
        backNode = node;
        ++count;
    }

    void enqueue(T&& value) {
        Node* node = new Node(std::move(value));
        if (backNode) {
            backNode->next = node;
        } else {
            frontNode = node;
        }
        backNode = node;
        ++count;
    }

    T dequeue() {
        if (isEmpty()) {
            throw std::underflow_error("dequeue() called on empty queue");
        }
        Node* node = frontNode;
        T value = std::move(node->data);
        frontNode = node->next;
        if (!frontNode) {
            backNode = nullptr;
        }
        delete node;
        --count;
        return value;
    }

    const T& peek() const {
        if (isEmpty()) {
            throw std::underflow_error("peek() called on empty queue");
        }
        return frontNode->data;
    }

    T& peek() {
        if (isEmpty()) {
            throw std::underflow_error("peek() called on empty queue");
        }
        return frontNode->data;
    }

    bool isEmpty() const {
        return count == 0;
    }

    std::size_t size() const {
        return count;
    }

    void clear() {
        while (frontNode) {
            Node* next = frontNode->next;
            delete frontNode;
            frontNode = next;
        }
        backNode = nullptr;
        count = 0;
    }
};

int main() {
    Queue<int> q;

    q.enqueue(10);
    q.enqueue(20);
    q.enqueue(30);

    std::cout << "Size: " << q.size() << "\n";
    std::cout << "Peek: " << q.peek() << "\n";

    std::cout << "Dequeued: " << q.dequeue() << "\n";
    std::cout << "Dequeued: " << q.dequeue() << "\n";

    std::cout << "Peek: " << q.peek() << "\n";
    std::cout << "Size: " << q.size() << "\n";

    q.dequeue();

    try {
        q.dequeue();
    } catch (const std::underflow_error& e) {
        std::cout << "Error: " << e.what() << "\n";
    }

    return 0;
}