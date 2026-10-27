#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

typedef struct Stack {
    Node *top;
    size_t size;
} Stack;

Stack *stack_create(void) {
    Stack *s = malloc(sizeof(Stack));
    if (s) {
        s->top = NULL;
        s->size = 0;
    }
    return s;
}

int stack_is_empty(const Stack *s) {
    return s == NULL || s->top == NULL;
}

int stack_push(Stack *s, int value) {
    if (s == NULL) return 0;
    Node *node = malloc(sizeof(Node));
    if (node == NULL) return 0;
    node->value = value;
    node->next = s->top;
    s->top = node;
    s->size++;
    return 1;
}

int stack_pop(Stack *s, int *out) {
    if (stack_is_empty(s)) return 0;
    Node *node = s->top;
    if (out) *out = node->value;
    s->top = node->next;
    free(node);
    s->size--;
    return 1;
}

int stack_peek(const Stack *s, int *out) {
    if (stack_is_empty(s)) return 0;
    if (out) *out = s->top->value;
    return 1;
}

size_t stack_size(const Stack *s) {
    return s ? s->size : 0;
}

void stack_destroy(Stack *s) {
    if (s == NULL) return;
    Node *cur = s->top;
    while (cur) {
        Node *next = cur->next;
        free(cur);
        cur = next;
    }
    free(s);
}

int main(void) {
    Stack *s = stack_create();
    if (s == NULL) {
        fprintf(stderr, "Failed to create stack\n");
        return EXIT_FAILURE;
    }

    for (int i = 1; i <= 5; i++) {
        stack_push(s, i * 10);
    }
    printf("Size after pushes: %zu\n", stack_size(s));

    int value;
    if (stack_peek(s, &value)) {
        printf("Peek: %d\n", value);
    }

    while (!stack_is_empty(s)) {
        if (stack_pop(s, &value)) {
            printf("Popped: %d\n", value);
        }
    }

    if (!stack_pop(s, &value)) {
        printf("Pop on empty stack failed as expected\n");
    }
    if (!stack_peek(s, &value)) {
        printf("Peek on empty stack failed as expected\n");
    }

    stack_destroy(s);
    return EXIT_SUCCESS;
}