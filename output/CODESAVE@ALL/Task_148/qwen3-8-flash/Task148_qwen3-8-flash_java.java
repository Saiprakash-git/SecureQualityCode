import java.util.NoSuchElementException;

public class Main {
    public static class Stack<T> {
        private static class Node<T> {
            private final T value;
            private final Node<T> next;

            private Node(T value, Node<T> next) {
                this.value = value;
                this.next = next;
            }
        }

        private Node<T> top;