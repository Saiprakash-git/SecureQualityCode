class Queue:
    def __init__(self):
        self._items = []

    def enqueue(self, item):
        self._items.append(item)
        return item

    def dequeue(self):
        if not self._items:
            raise IndexError("dequeue from empty queue")
        return self._items.pop(0)

    def peek(self):
        if not self._items:
            raise IndexError("peek from empty queue")
        return self._items[0]

    def is_empty(self):
        return len(self._items) == 0

    def __len__(self):
        return len(self._items)