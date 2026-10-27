public class Main {

    static class Queue<T> {
        private static class Node<T> {
            T value;
            Node<T> next;
            Node(T value) {
                this.value = value;
            }
        }

        private Node<T> head;
        private Node<T> tail;
        private int size;

        public void enqueue(T item) {
            Node<T> node = new Node<>(item);
            if (tail == null) {
                head = tail = node;
            } else {
                tail.next = node;
                tail = node;
            }
            size++;
        }

        public T dequeue() {
            if (isEmpty()) {
                throw new IllegalStateException("Queue is empty");
            }
            T value = head.value;
            head = head.next;
            if (head == null) {
                tail = null;
            }
            size--;
            return value;
        }

        public T peek() {
            if (isEmpty()) {
                throw new IllegalStateException("Queue is empty");
            }
            return head.value;
        }

        public boolean isEmpty() {
            return size == 0;
        }

        public int size() {
            return size;
        }
    }

    public static void main(String[] args) {
        Queue<Integer> queue = new Queue<>();
        int[] items = {10, 20, 30};
        for (int item : items) {
            queue.enqueue(item);
        }
        System.out.println("Peek: " + queue.peek());
        System.out.println("Dequeue: " + queue.dequeue());
        System.out.println("Peek: " + queue.peek());
        System.out.println("Size: " + queue.size());
        System.out.println("Dequeue: " + queue.dequeue());
        System.out.println("Dequeue: " + queue.dequeue());
        System.out.println("Empty: " + queue.isEmpty());
    }
}