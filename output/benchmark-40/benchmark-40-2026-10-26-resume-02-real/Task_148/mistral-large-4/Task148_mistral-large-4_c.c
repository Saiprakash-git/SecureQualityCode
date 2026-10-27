#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 100

typedef struct {
    int data[MAX_SIZE];
    int top;
} Stack;

void init_stack(Stack *s) {
    s->top = -1;
}

int is_empty(Stack *s) {
    return s->top == -1;
}

int is_full(Stack *s) {
    return s->top == MAX_SIZE - 1;
}

int push(Stack *s, int value) {
    if (is_full(s)) {
        return 0;
    }
    s->data[++(s->top)] = value;
    return 1;
}

int pop(Stack *s, int *value) {
    if (is_empty(s)) {
        return 0;
    }
    *value = s->data[(s->top)--];
    return 1;
}

int peek(Stack *s, int *value) {
    if (is_empty(s)) {
        return 0;
    }
    *value = s->data[s->top];
    return 1;
}

int main() {
    Stack s;
    int value;

    init_stack(&s);

    push(&s, 10);
    push(&s, 20);
    push(&s, 30);

    if (peek(&s, &value)) {
        printf("Top element: %d\n", value);
    }

    if (pop(&s, &value)) {
        printf("Popped: %d\n", value);
    }

    if (pop(&s, &value)) {
        printf("Popped: %d\n", value);
    }

    if (peek(&s, &value)) {
        printf("Top element: %d\n", value);
    }

    return 0;
}