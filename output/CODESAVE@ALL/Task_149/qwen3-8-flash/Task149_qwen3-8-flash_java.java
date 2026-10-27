public class Queue<T> {
    private T[] data;
    private int front;
    private int size;

    @SuppressWarnings("unchecked")
    public Queue(int initialCapacity) {
        if (initialCapacity <= 0) {
            throw new IllegalArgumentException("Initial capacity must be positive");
        }
        this.data = (T[]) new Object[initialCapacity];
        this.front = 0;
        this.size = 0;
    }

    public Queue() {
        this(4);
    }

    public void enqueue(T item) {
        if (size == data.length) {
            resize();
        }
        int rear = (front + size) % data.length;
        data[rear] = item;
        size++;
    }

    public T dequeue() {
        if (size == 0) {
            throw new IllegalStateException("Queue is empty");
        }
        T item = data[front];
        data[front] = null;
        front = (front + 1) % data.length;
        size--;

        if (size == 0) {
            front = 0;
        }

        return item;
    }

    public T peek() {
        if (size == 0) {
            throw new IllegalStateException("Queue is empty");
        }
        return data[front];
    }

    public boolean isEmpty() {
        return size == 0;
    }

    public int size() {
        return size;
    }

    @SuppressWarnings("unchecked")
    private void resize() {
        T[] newData = (T[]) new Object[data.length * 2];

        for (int i = 0; i < size; i++) {
            newData[i] = data[(front + i) % data.length];
        }

        data = newData;
        front = 0;
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
        System.out.println("Empty: " + queue.isEmpty());

        queue.enqueue(40);
        System.out.println("Dequeue: " + queue.dequeue());
    }
}