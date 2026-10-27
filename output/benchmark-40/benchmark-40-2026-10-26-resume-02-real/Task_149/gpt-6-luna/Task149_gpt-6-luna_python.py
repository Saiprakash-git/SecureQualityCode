from collections import deque
from typing import Deque, Generic, TypeVar

T = TypeVar("T")


class Queue(Generic[T]):
    def __init__(self) -> None:
        self._items: Deque[T] = deque()

    def enqueue(self, item: T) -> None:
        self._items.append(item)

    def dequeue(self) -> T:
        if not self._items:
            raise IndexError("dequeue from an empty queue")
        return self._items.popleft()

    def peek(self) -> T:
        if not self._items:
            raise IndexError("peek from an empty queue")
        return self._items[0]

    def is_empty(self) -> bool:
        return not self._items

    def __len__(self) -> int:
        return len(self._items)