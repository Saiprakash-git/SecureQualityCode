#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define INITIAL_CAPACITY 4

typedef struct {
    int *data;
    size_t size;
    size_t capacity;
} Stack;

bool stack_init(Stack *stack) {
    if (stack == NULL) {
        return false;
    }
    stack->data = (int *)malloc(INITIAL_CAPACITY * sizeof(int));
    if (stack->data == NULL) {
        stack->size = 0;
        stack->capacity = 0;
        return false;
    }
    stack->size = 0;
    stack->capacity = INITIAL_CAPACITY;
    return true;
}

bool stack_is_empty(const Stack *stack) {
    return (stack == NULL || stack->size == 0);
}

bool stack_push(Stack *stack, int value) {
    if (stack == NULL) {
        return false;
    }
    if (stack->size == stack->capacity) {
        size_t new_capacity = stack->capacity * 2;
        int *new_data = (int *)realloc(stack->data, new_capacity * sizeof(int));
        if (new_data == NULL) {
            return false;
        }
        stack->data = new_data;
        stack->capacity = new_capacity;
    }
    stack->data[stack->size++] = value;
    return true;
}

bool stack_pop(Stack *stack, int *out_value) {
    if (stack_is_empty(stack)) {
        return false;
    }
    stack->size--;
    if (out_value != NULL) {
        *out_value = stack->data[stack->size];
    }
    return true;
}

bool stack_peek(const Stack *stack, int *out_value) {
    if (stack_is_empty(stack)) {
        return false;
    }
    if (out_value != NULL) {
        *out_value = stack->data[stack->size - 1];
    }
    return true;
}

void stack_free(Stack *stack) {
    if (stack != NULL) {
        free(stack->data);
        stack->data = NULL;
        stack->size = 0;
        stack->capacity = 0;
    }
}

int main(void) {
    Stack stack;
    if (!stack_init(&stack)) {
        fprintf(stderr, "Failed to initialize stack\n");
        return 1;
    }

    printf("Pushing elements: 10, 20, 30, 40, 50\n");
    stack_push(&stack, 10);
    stack_push(&stack, 20);
    stack_push(&stack, 30);
    stack_push(&stack, 40);
    stack_push(&stack, 50);

    int top_val;
    if (stack_peek(&stack, &top_val)) {
        printf("Current top element (peek): %d\n", top_val);
    }

    printf("Popping elements:\n");
    int popped_val;
    while (stack_pop(&stack, &popped_val)) {
        printf("Popped: %d\n", popped_val);
    }

    if (!stack_pop(&stack, &popped_val)) {
        printf("Stack is now empty. Pop failed as expected.\n");
    }

    stack_free(&stack);
    return 0;
}