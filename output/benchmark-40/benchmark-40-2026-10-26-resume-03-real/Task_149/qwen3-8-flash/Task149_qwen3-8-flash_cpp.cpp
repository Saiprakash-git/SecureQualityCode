#include <iostream>

template <typename T>
class Queue {
private:
    struct Node {
        T data;
        Node* next;

        Node(const T& value) : data(value), next(nullptr) {}
    };

    Node* front;
    Node* back;
    std::size_t count;

public:
    Queue() : front(nullptr), back(nullptr), count(0) {}

    ~Queue() {
        while (front != nullptr) {
            Node* temp = front;
            front = front->next;
            delete temp;
        }
    }

    void enqueue(const T& value) {
        Node* newNode = new Node(value);

        if (back == nullptr) {
            front = newNode;
            back = newNode;
        } else {
            back->next = newNode;
            back = newNode;
        }

        ++count;
    }

    bool dequeue(T& value) {
        if (front == nullptr) {
            return false;
        }

        value = front->data;
        Node* temp = front;
        front = front->next;

        if (front == nullptr) {
            back = nullptr;
        }

        delete temp;
        --count;

        return true;
    }

    bool peek(T& value) const {
        if (front == nullptr) {
            return false;
        }

        value = front->data;
        return true;
    }

    bool isEmpty() const {
        return front == nullptr;
    }

    std::size_t size() const {
        return count;
    }
};

int main() {
    Queue<int> queue;

    queue.enqueue(10);
    queue.enqueue(20);
    queue.enqueue(30);

    int value;

    if (queue.peek(value)) {
        std::cout << "Peek: " << value << '\n';
    }

    if (queue.dequeue(value)) {
        std::cout << "Dequeue: " << value << '\n';
    }

    if (queue.dequeue(value)) {
        std::cout << "Dequeue: " << value << '\n';
    }

    if (queue.dequeue(value)) {
        std::cout << "Dequeue: " << value << '\n';
    }

    if (!queue.dequeue(value)) {
        std::cout << "Queue is empty.\n";
    }

    return 0;
}