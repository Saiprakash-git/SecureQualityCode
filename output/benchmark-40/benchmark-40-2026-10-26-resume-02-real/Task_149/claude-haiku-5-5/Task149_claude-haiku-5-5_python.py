class Queue:
    def __init__(self):
        self._items = []
        self._head = 0

    def enqueue(self, item):
        self._items.append(item)

    def dequeue(self):
        if self.is_empty():
            raise IndexError("dequeue from empty queue")
        item = self._items[self._head]
        self._items[self._head] = None
        self._head += 1
        if self._head > 32 and self._head * 2 >= len(self._items):
            self._items = self._items[self._head:]
            self._head = 0
        return item

    def peek(self):
        if self.is_empty():
            raise IndexError("peek from empty queue")
        return self._items[self._head]

    def is_empty(self):
        return self._head >= len(self._items)

    def size(self):
        return len(self._items) - self._head

    def __len__(self):
        return self.size()

    def __repr__(self):
        return f"Queue({self._items[self._head:]!r})"


def main():
    q = Queue()
    q.enqueue(1)
    q.enqueue(2)
    q.enqueue(3)
    print(q)
    print("Peek:", q.peek())
    print("Dequeue:", q.dequeue())
    print("Dequeue:", q.dequeue())
    print("Size:", q.size())
    print("Dequeue:", q.dequeue())
    print("Empty:", q.is_empty())


if __name__ == "__main__":
    main()