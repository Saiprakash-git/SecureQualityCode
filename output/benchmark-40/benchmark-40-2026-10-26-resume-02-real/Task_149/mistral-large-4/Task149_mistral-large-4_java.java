import java.util.NoSuchElementException;

public class Queue<T> {
    private Node<T> head;
    private Node<T> tail;
    private int size;

    private static class Node<T> {
        T data;
        Node<T> next;

        Node(T data) {
            this.data = data;
            this.next = null;
        }
    }

    public Queue() {
        this.head = null;
        this.tail = null;
        this.size = 0;
    }

    public void enqueue(T item) {
        Node<T> newNode = new Node<>(item);
        if (this.tail == null) {
            this.head = newNode;
            this.tail = newNode;
        } else {
            this.tail.next = newNode;
            this.tail = newNode;
        }
        this.size++;
    }

    public T dequeue() {
        if (this.head == null) {
            throw new NoSuchElementException("Queue is empty");
        }
        T data = this.head.data;
        this.head = this.head.next;
        if (this.head == null) {
            this.tail = null;
        }
        this.size--;
        return data;
    }

    public T peek() {
        if (this.head == null) {
            throw new NoSuchElementException("Queue is empty");
        }
        return this.head.data;
    }

    public boolean isEmpty() {
        return this.size == 0;
    }

    public int size() {
        return this.size;
    }

    public static void main(String[] args) {
        Queue<Integer> queue = new Queue<>();
        queue.enqueue(10);
        queue.enqueue(20);
        queue.enqueue(30);
        System.out.println("Peek: " + queue.peek());
        System.out.println("Dequeue: " + queue.dequeue());
        System.out.println("Dequeue: " + queue.dequeue());
        System.out.println("Peek: " + queue.peek());
        System.out.println("Size: " + queue.size());
    }
}