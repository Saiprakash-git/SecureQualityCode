#include <iostream>
#include <stdexcept>

class Queue {
private:
    struct Node {
        int data;
        Node* next;
        Node(int val) : data(val), next(nullptr) {}
    };
    Node* front;
    Node* rear;
    int size;

public:
    Queue() : front(nullptr), rear(nullptr), size(0) {}

    ~Queue() {
        while (front != nullptr) {
            Node* temp = front;
            front = front->next;
            delete temp;
        }
    }

    void enqueue(int value) {
        Node* newNode = new Node(value);
        if (rear == nullptr) {
            front = rear = newNode;
        } else {
            rear->next = newNode;
            rear = newNode;
        }
        size++;
    }

    int dequeue() {
        if (front == nullptr) {
            throw std::runtime_error("Queue is empty");
        }
        Node* temp = front;
        int value = temp->data;
        front = front->next;
        if (front == nullptr) {
            rear = nullptr;
        }
        delete temp;
        size--;
        return value;
    }

    int peek() const {
        if (front == nullptr) {
            throw std::runtime_error("Queue is empty");
        }
        return front->data;
    }

    bool isEmpty() const {
        return front == nullptr;
    }

    int getSize() const {
        return size;
    }
};

int main() {
    Queue q;
    q.enqueue(10);
    q.enqueue(20);
    q.enqueue(30);
    std::cout << q.peek() << std::endl;
    std::cout << q.dequeue() << std::endl;
    std::cout << q.dequeue() << std::endl;
    std::cout << q.peek() << std::endl;
    return 0;
}