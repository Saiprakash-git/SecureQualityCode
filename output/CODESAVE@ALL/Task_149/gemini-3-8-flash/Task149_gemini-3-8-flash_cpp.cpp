#include <iostream>
#include <stdexcept>
#include <utility>

template <typename T>
class Queue {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& value) : data(value), next(nullptr) {}
        Node(T&& value) : data(std::move(value)), next(nullptr) {}
    };

    Node* frontNode;
    Node* rearNode;
    size_t count;

public:
    Queue() : frontNode(nullptr), rearNode(nullptr), count(0) {}

    ~Queue() {
        clear();
    }

    Queue(const Queue& other) : frontNode(nullptr), rearNode(nullptr), count(0) {
        Node* current = other.frontNode;
        while (current != nullptr) {
            enqueue(current->data);
            current = current->next;
        }
    }

    Queue& operator=(const Queue& other) {
        if (this != &other) {
            clear();
            Node* current = other.frontNode;
            while (current != nullptr) {
                enqueue(current->data);
                current = current->next;
            }
        }
        return *this;
    }

    Queue(Queue&& other) noexcept 
        : frontNode(other.frontNode), rearNode(other.rearNode), count(other.count) {
        other.frontNode = nullptr;
        other.rearNode = nullptr;
        other.count = 0;
    }

    Queue& operator=(Queue&& other) noexcept {
        if (this != &other) {
            clear();
            frontNode = other.frontNode;
            rearNode = other.rearNode;
            count = other.count;
            other.frontNode = nullptr;
            other.rearNode = nullptr;
            other.count = 0;
        }
        return *this;
    }

    void enqueue(const T& value) {
        Node* newNode = new Node(value);
        if (isEmpty()) {
            frontNode = rearNode = newNode;
        } else {
            rearNode->next = newNode;
            rearNode = newNode;
        }
        ++count;
    }

    void enqueue(T&& value) {
        Node* newNode = new Node(std::move(value));
        if (isEmpty()) {
            frontNode = rearNode = newNode;
        } else {
            rearNode->next = newNode;
            rearNode = newNode;
        }
        ++count;
    }

    T dequeue() {
        if (isEmpty()) {
            throw std::underflow_error("Queue is empty: cannot dequeue.");
        }
        Node* temp = frontNode;
        T data = std::move(temp->data);
        frontNode = frontNode->next;
        if (frontNode == nullptr) {
            rearNode = nullptr;
        }
        delete temp;
        --count;
        return data;
    }

    const T& peek() const {
        if (isEmpty()) {
            throw std::underflow_error("Queue is empty: cannot peek.");
        }
        return frontNode->data;
    }

    bool isEmpty() const {
        return frontNode == nullptr;
    }

    size_t size() const {
        return count;
    }

    void clear() {
        while (!isEmpty()) {
            dequeue();
        }
    }
};

int main() {
    Queue<int> q;

    std::cout << "Enqueuing 10, 20, 30 into the queue...\n";
    q.enqueue(10);
    q.enqueue(20);
    q.enqueue(30);

    std::cout << "Front element (peek): " << q.peek() << "\n";
    std::cout << "Queue size: " << q.size() << "\n";

    std::cout << "Dequeued element: " << q.dequeue() << "\n";
    std::cout << "Front element after dequeue: " << q.peek() << "\n";

    std::cout << "Enqueuing 40...\n";
    q.enqueue(40);

    std::cout << "Emptying queue:\n";
    while (!q.isEmpty()) {
        std::cout << "Dequeued: " << q.dequeue() << "\n";
    }

    try {
        q.peek();
    } catch (const std::underflow_error& e) {
        std::cout << "Caught expected exception on peek: " << e.what() << "\n";
    }

    try {
        q.dequeue();
    } catch (const std::underflow_error& e) {
        std::cout << "Caught expected exception on dequeue: " << e.what() << "\n";
    }

    return 0;
}