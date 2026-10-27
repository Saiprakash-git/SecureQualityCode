import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.util.NoSuchElementException;

public class Main {
    public static class Queue<T> {
        private static class Node<T> {
            private final T value;
            private Node<T> next;

            private Node(T value) {
                this.value = value;
            }
        }

        private Node<T> head;
        private Node<T> tail;
        private int size;

        public void enqueue(T value) {
            Node<T> node = new Node<>(value);
            if (tail == null) {
                head = node;
            } else {
                tail.next = node;
            }
            tail = node;
            size++;
        }

        public T dequeue() {
            if (head == null) {
                throw new NoSuchElementException("Queue is empty");
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
            if (head == null) {
                throw new NoSuchElementException("Queue is empty");
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

    public static void main(String[] args) throws IOException {
        Queue<String> queue = new Queue<>();
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        String line;

        while ((line = reader.readLine()) != null) {
            line = line.trim();
            if (line.isEmpty()) {
                continue;
            }

            int separator = line.indexOf(' ');
            String command = (separator < 0 ? line : line.substring(0, separator))
                    .toLowerCase();
            String value = separator < 0 ? "" : line.substring(separator + 1).trim();

            switch (command) {
                case "enqueue":
                    queue.enqueue(value);
                    break;
                case "dequeue":
                    if (queue.isEmpty()) {
                        System.out.println("EMPTY");
                    } else {
                        System.out.println(queue.dequeue());
                    }
                    break;
                case "peek":
                    if (queue.isEmpty()) {
                        System.out.println("EMPTY");
                    } else {
                        System.out.println(queue.peek());
                    }
                    break;
                default:
                    throw new IllegalArgumentException("Unknown operation: " + command);
            }
        }
    }
}