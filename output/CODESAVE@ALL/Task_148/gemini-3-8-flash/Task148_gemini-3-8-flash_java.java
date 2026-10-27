import java.util.EmptyStackException;

public class Main {

    public static class Stack<T> {
        private static class Node<T> {
            private final T data;
            private final Node<T> next;

            public Node(T data, Node<T> next) {
                this.data = data;
                this.next = next;
            }
        }

        private Node<T> top;
        private int size;

        public Stack() {
            this.top = null;
            this.size = 0;
        }

        public void push(T item) {
            this.top = new Node<>(item, this.top);
            this.size++;
        }

        public T pop() {
            if (isEmpty()) {
                throw new EmptyStackException();
            }
            T item = this.top.data;
            this.top = this.top.next;
            this.size--;
            return item;
        }

        public T peek() {
            if (isEmpty()) {
                throw new EmptyStackException();
            }
            return this.top.data;
        }

        public boolean isEmpty() {
            return this.top == null;
        }

        public int size() {
            return this.size;
        }
    }

    public static void main(String[] args) {
        Stack<Integer> stack = new Stack<>();

        stack.push(10);
        stack.push(20);
        stack.push(30);

        System.out.println("Current size: " + stack.size());
        System.out.println("Peek: " + stack.peek());
        System.out.println("Popped: " + stack.pop());
        System.out.println("Peek after pop: " + stack.peek());
        System.out.println("Popped: " + stack.pop());
        System.out.println("Popped: " + stack.pop());
        System.out.println("Is empty: " + stack.isEmpty());
    }
}