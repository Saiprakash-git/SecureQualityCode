class Stack:
    """A Last-In, First-Out (LIFO) stack data structure."""

    def __init__(self):
        self._items = []

    def push(self, item):
        """Add an item to the top of the stack."""
        self._items.append(item)

    def pop(self):
        """Remove and return the top item from the stack.

        Raises:
            IndexError: If the stack is empty.
        """
        if self.is_empty():
            raise IndexError("pop from empty stack")
        return self._items.pop()

    def peek(self):
        """Return the top item without removing it.

        Raises:
            IndexError: If the stack is empty.
        """
        if self.is_empty():
            raise IndexError("peek from empty stack")
        return self._items[-1]

    def is_empty(self):
        """Check whether the stack is empty."""
        return len(self._items) == 0

    def size(self):
        """Return the number of items in the stack."""
        return len(self._items)

    def __len__(self):
        return len(self._items)

    def __repr__(self):
        return f"Stack({self._items})"


def main():
    stack = Stack()
    
    print("Pushing elements: 10, 20, 30")
    stack.push(10)
    stack.push(20)
    stack.push(30)
    print("Current stack:", stack)

    print("Peek top element:", stack.peek())
    print("Popped element:", stack.pop())
    print("Popped element:", stack.pop())
    print("Current stack after pops:", stack)
    print("Peek top element:", stack.peek())
    print("Stack size:", stack.size())
    print("Is stack empty?:", stack.is_empty())
    print("Popped element:", stack.pop())
    print("Is stack empty?:", stack.is_empty())


if __name__ == "__main__":
    main()