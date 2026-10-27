from collections import deque
from typing import Any


class Queue:
    def __init__(self):
        self._items = deque()

    def enqueue(self, item: Any) -> None:
        self._items.append(item)

    def dequeue(self) -> Any:
        if self.is_empty():
            raise IndexError("dequeue from an empty queue")
        return self._items.popleft()

    def peek(self) -> Any:
        if self.is_empty():
            raise IndexError("peek from an empty queue")
        return self._items[0]

    def is_empty(self) -> bool:
        return len(self._items) == 0

    def size(self) -> int:
        return len(self._items)

    def __len__(self) -> int:
        return self.size()

    def __repr__(self) -> str:
        return f"Queue({list(self._items)})"


def demonstrate_queue():
    queue = Queue()
    
    print(f"Initial queue is empty: {queue.is_empty()}")
    
    for value in [10, 20, 30, 40]:
        print(f"Enqueuing: {value}")
        queue.enqueue(value)
        
    print(f"Current queue: {queue}")
    print(f"Queue size: {queue.size()}")
    print(f"Front element (peek): {queue.peek()}")
    
    print(f"Dequeued element: {queue.dequeue()}")
    print(f"Dequeued element: {queue.dequeue()}")
    
    print(f"Queue after dequeues: {queue}")
    print(f"Front element after dequeues (peek): {queue.peek()}")
    print(f"Queue size after dequeues: {queue.size()}")


if __name__ == "__main__":
    demonstrate_queue()