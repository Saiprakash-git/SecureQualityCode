#include <iostream>
#include <optional>
#include <string>
#include <utility>

template <typename T>
class Queue {
private:
    struct Node {
        T value;
        Node* next;

        explicit Node(T item) : value(std::move(item)), next(nullptr) {}
    };

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;

public:
    Queue() = default;

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    ~Queue() {
        while (head_ != nullptr) {
            Node* node = head_;
            head_ = head_->next;
            delete node;
        }
    }

    void enqueue(T value) {
        Node* node = new Node(std::move(value));

        if (tail_ != nullptr) {
            tail_->next = node;
        } else {
            head_ = node;
        }

        tail_ = node;
        ++size_;
    }

    std::optional<T> dequeue() {
        if (head_ == nullptr) {
            return std::nullopt;
        }

        Node* node = head_;
        std::optional<T> value(std::move(node->value));
        head_ = head_->next;

        if (head_ == nullptr) {
            tail_ = nullptr;
        }

        delete node;
        --size_;
        return value;
    }

    std::optional<T> peek() const {
        if (head_ == nullptr) {
            return std::nullopt;
        }

        return head_->value;
    }

    bool empty() const {
        return head_ == nullptr;
    }

    std::size_t size() const {
        return size_;
    }
};

int main() {
    Queue<int> queue;
    std::string operation;

    while (std::cin >> operation) {
        if (operation == "ENQUEUE" || operation == "enqueue") {
            int value;
            if (std::cin >> value) {
                queue.enqueue(value);
            }
        } else if (operation == "DEQUEUE" || operation == "dequeue") {
            const auto value = queue.dequeue();
            if (value) {
                std::cout << *value << '\n';
            } else {
                std::cout << "EMPTY\n";
            }
        } else if (operation == "PEEK" || operation == "peek") {
            const auto value = queue.peek();
            if (value) {
                std::cout << *value << '\n';
            } else {
                std::cout << "EMPTY\n";
            }
        }
    }

    return 0;
}