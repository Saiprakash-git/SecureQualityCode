#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int *items;
    size_t size;
    size_t capacity;
} Stack;

void stack_init(Stack *stack) {
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}

bool stack_push(Stack *stack, int value) {
    if (stack->size == stack->capacity) {
        size_t new_capacity;

        if (stack->capacity == 0) {
            new_capacity = 8;
        } else {
            if (stack->capacity > SIZE_MAX / 2) {
                return false;
            }
            new_capacity = stack->capacity * 2;
        }

        if (new_capacity > SIZE_MAX / sizeof(*stack->items)) {
            return false;
        }

        int *new_items = realloc(stack->items, new_capacity * sizeof(*stack->items));
        if (new_items == NULL) {
            return false;
        }

        stack->items = new_items;
        stack->capacity = new_capacity;
    }

    stack->items[stack->size++] = value;
    return true;
}

bool stack_pop(Stack *stack, int *value) {
    if (stack->size == 0) {
        return false;
    }

    *value = stack->items[--stack->size];
    return true;
}

bool stack_peek(const Stack *stack, int *value) {
    if (stack->size == 0) {
        return false;
    }

    *value = stack->items[stack->size - 1];
    return true;
}

void stack_destroy(Stack *stack) {
    free(stack->items);
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}

int main(void) {
    Stack stack;
    char command[32];
    int value;

    stack_init(&stack);

    while (scanf("%31s", command) == 1) {
        if (strcmp(command, "push") == 0) {
            if (scanf("%d", &value) != 1) {
                break;
            }
            puts(stack_push(&stack, value) ? "OK" : "ERROR");
        } else if (strcmp(command, "pop") == 0) {
            if (stack_pop(&stack, &value)) {
                printf("%d\n", value);
            } else {
                puts("EMPTY");
            }
        } else if (strcmp(command, "peek") == 0) {
            if (stack_peek(&stack, &value)) {
                printf("%d\n", value);
            } else {
                puts("EMPTY");
            }
        } else if (strcmp(command, "quit") == 0) {
            break;
        }
    }

    stack_destroy(&stack);
    return 0;
}