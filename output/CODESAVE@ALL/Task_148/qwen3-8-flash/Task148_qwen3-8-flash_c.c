#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int *items;
    size_t top;
    size_t capacity;
} Stack;

bool stack_init(Stack *stack, size_t initial_capacity) {
    if (!stack || initial_capacity == 0) {
        return false;
    }

    stack->items = malloc(initial_capacity * sizeof(int));
    if (!stack->items) {
        return false;
    }

    stack->top = 0;
    stack->capacity = initial_capacity;

    return true;
}

bool stack_push(Stack *stack, int value) {
    if (!stack || !stack->items) {
        return false;
    }

    if (stack->top == stack->capacity) {
        size_t new_capacity = stack->capacity * 2;
        int *new_items = realloc(stack->items, new_capacity * sizeof(int));

        if (!new_items) {
            return false;
        }

        stack->items = new_items;
        stack->capacity = new_capacity;
    }

    stack->items[stack->top] = value;
    stack->top++;

    return true;
}

bool stack_pop(Stack *stack, int *value) {
    if (!stack || !stack->items || !value || stack->top == 0) {
        return false;
    }

    stack->top--;
    *value = stack->items[stack->top];

    return true;
}

bool stack_peek(Stack *stack, int *value) {
    if (!stack || !stack->items || !value || stack->top == 0) {
        return false;
    }

    *value = stack->items[stack->top - 1];

    return true;
}

void stack_free(Stack *stack) {
    if (stack && stack->items) {
        free(stack->items);
        stack->items = NULL;
        stack->top = 0;
        stack->capacity = 0;
    }
}

int main(void) {
    Stack stack;

    if (!stack_init(&stack, 4)) {
        fprintf(stderr, "Failed to initialize stack.\n");
        return 1;
    }

    printf("Pushing 10, 20, and 30...\n");

    if (!stack_push(&stack, 10) ||
        !stack_push(&stack, 20) ||
        !stack_push(&stack, 30)) {
        fprintf(stderr, "Push failed.\n");
        stack_free(&stack);
        return 1;
    }

    int value;

    if (stack_peek(&stack, &value)) {
        printf("Peek: %d\n", value);
    }

    if (stack_pop(&stack, &value)) {
        printf("Pop: %d\n", value);
    }

    if (stack_peek(&stack, &value)) {
        printf("Peek after pop: %d\n", value);
    }

    if (stack_pop(&stack, &value)) {
        printf("Pop: %d\n", value);
    }

    if (stack_pop(&stack, &value)) {
        printf("Pop: %d\n", value);
    }

    if (!stack_pop(&stack, &value)) {
        printf("Stack is empty.\n");
    }

    stack_free(&stack);

    return 0;
}