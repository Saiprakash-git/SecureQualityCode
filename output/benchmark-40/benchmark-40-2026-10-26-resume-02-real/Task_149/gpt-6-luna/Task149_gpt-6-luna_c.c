#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int *items;
    size_t capacity;
    size_t size;
    size_t front;
} Queue;

bool queue_init(Queue *queue, size_t initial_capacity) {
    if (queue == NULL) {
        return false;
    }

    if (initial_capacity == 0) {
        initial_capacity = 4;
    }

    if (initial_capacity > SIZE_MAX / sizeof(int)) {
        return false;
    }

    queue->items = malloc(initial_capacity * sizeof(int));
    if (queue->items == NULL) {
        queue->capacity = 0;
        queue->size = 0;
        queue->front = 0;
        return false;
    }

    queue->capacity = initial_capacity;
    queue->size = 0;
    queue->front = 0;
    return true;
}

void queue_destroy(Queue *queue) {
    if (queue == NULL) {
        return;
    }

    free(queue->items);
    queue->items = NULL;
    queue->capacity = 0;
    queue->size = 0;
    queue->front = 0;
}

bool queue_is_empty(const Queue *queue) {
    return queue == NULL || queue->size == 0;
}

size_t queue_size(const Queue *queue) {
    return queue == NULL ? 0 : queue->size;
}

static bool queue_grow(Queue *queue) {
    if (queue->capacity > SIZE_MAX / 2) {
        return false;
    }

    size_t new_capacity = queue->capacity * 2;
    if (new_capacity == 0 || new_capacity > SIZE_MAX / sizeof(int)) {
        return false;
    }

    int *new_items = malloc(new_capacity * sizeof(int));
    if (new_items == NULL) {
        return false;
    }

    for (size_t i = 0; i < queue->size; ++i) {
        new_items[i] = queue->items[(queue->front + i) % queue->capacity];
    }

    free(queue->items);
    queue->items = new_items;
    queue->capacity = new_capacity;
    queue->front = 0;
    return true;
}

bool queue_enqueue(Queue *queue, int value) {
    if (queue == NULL || queue->items == NULL) {
        return false;
    }

    if (queue->size == queue->capacity && !queue_grow(queue)) {
        return false;
    }

    size_t rear = (queue->front + queue->size) % queue->capacity;
    queue->items[rear] = value;
    ++queue->size;
    return true;
}

bool queue_dequeue(Queue *queue, int *value) {
    if (queue == NULL || value == NULL || queue->size == 0) {
        return false;
    }

    *value = queue->items[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    --queue->size;

    if (queue->size == 0) {
        queue->front = 0;
    }

    return true;
}

bool queue_peek(const Queue *queue, int *value) {
    if (queue == NULL || value == NULL || queue->size == 0) {
        return false;
    }

    *value = queue->items[queue->front];
    return true;
}

int main(void) {
    Queue queue;
    if (!queue_init(&queue, 0)) {
        return EXIT_FAILURE;
    }

    queue_enqueue(&queue, 10);
    queue_enqueue(&queue, 20);

    int value;
    if (queue_peek(&queue, &value)) {
        printf("Front: %d\n", value);
    }

    if (queue_dequeue(&queue, &value)) {
        printf("Dequeued: %d\n", value);
    }

    queue_destroy(&queue);
    return EXIT_SUCCESS;
}