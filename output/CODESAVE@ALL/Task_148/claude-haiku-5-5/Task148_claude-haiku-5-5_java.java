public class Main {

    static class Stack<T> {
        private static class Node<T> {
            T value;
            Node<T> next;

            Node(T value, Node<T> next) {
                this.value = value;
                this.next = next;
            }
        }

        private Node<T> head;
        private int size;

        public void push(T value) {
            head = new Node<>(value, head);
            size++;
        }

        public T pop() {
            if (isEmpty()) {
                throw new RuntimeException("Stack is empty");
            }
            T value = head.value;
            head = head.next;
            size--;
            return value;
        }

        public T peek() {
            if (isEmpty()) {
                throw new RuntimeException("Stack is empty");
            }
            return head.value;
        }

        public boolean isEmpty() {
            return head == null;
        }

        public int size() {
            return size;
        }
    }

    public static void main(String[] args) {
        Stack<Integer> stack = new Stack<>();

        stack.push(10);
        stack.push(20);
        stack.push(30);

        System.out.println("Peek: " + stack.peek());
        System.out.println("Pop: " + stack.pop());
        System.out.println("Pop: " + stack.pop());
        System.out.println("Size: " + stack.size());
        System.out.println("Peek: " + stack.peek());
        System.out.println("Pop: " + stack.pop());
        System.out.println("Is empty: " + stack.isEmpty());
    }
}