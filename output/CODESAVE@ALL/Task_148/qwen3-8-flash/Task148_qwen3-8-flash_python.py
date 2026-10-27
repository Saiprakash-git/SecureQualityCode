class Stack:
    def __init__(self):
        self._items = []

    def push(self, value):
        self._items.append(value)
        return value

    def pop(self):
        if not self._items:
            raise IndexError("pop from empty stack")
        return self._items.pop()

    def peek(self):
        if not self._items:
            raise IndexError("peek from empty stack")
        return self._items[-1]

    def is_empty(self):
        return len(self._items) == 0

    def size(self):
        return len(self._items)

    def __len__(self):
        return len(self._items)

    def __repr__(self):
        return f"Stack({self._items})"