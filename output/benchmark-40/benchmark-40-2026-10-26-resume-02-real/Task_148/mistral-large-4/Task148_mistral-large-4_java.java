public class Stack<T> {
    private Object[] elements;
    private int size;
    private static final int DEFAULT_CAPACITY = 10;

    public Stack() {
        elements = new Object[DEFAULT_CAPACITY];
        size = 0;
    }

    public void push(T item) {
        if (size == elements.length) {
            resize();
        }
        elements[size++] = item;
    }

    @SuppressWarnings("unchecked")
    public T pop() {
        if (isEmpty()) {
            throw new java.util.EmptyStackException();
        }
        T item = (T) elements[--size];
        elements[size] = null;
        return item;
    }

    @SuppressWarnings("unchecked")
    public T peek() {
        if (isEmpty()) {
            throw new java.util.EmptyStackException();
        }
        return (T) elements[size - 1];
    }

    public boolean isEmpty() {
        return size == 0;
    }

    public int size() {
        return size;
    }

    private void resize() {
        Object[] newElements = new Object[elements.length * 2];
        System.arraycopy(elements, 0, newElements, 0, size);
        elements = newElements;
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
    }
}