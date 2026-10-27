class Stack:
    def __init__(self):
        self._items = []

    def push(self, item):
        self._items.append(item)

    def pop(self):
        if self.is_empty():
            raise IndexError("pop from empty stack")
        return self._items.pop()

    def peek(self):
        if self.is_empty():
            raise IndexError("peek from empty stack")
        return self._items[-1]

    def is_empty(self):
        return len(self._items) == 0

    def size(self):
        return len(self._items)

    def __len__(self):
        return len(self._items)

    def __repr__(self):
        return f"Stack({self._items!r})"


def main():
    stack = Stack()
    stack.push(1)
    stack.push(2)
    stack.push(3)
    print("Stack:", stack)
    print("Peek:", stack.peek())
    print("Pop:", stack.pop())
    print("Pop:", stack.pop())
    print("Size:", stack.size())
    print("Pop:", stack.pop())
    print("Empty:", stack.is_empty())


if __name__ == "__main__":
    main()