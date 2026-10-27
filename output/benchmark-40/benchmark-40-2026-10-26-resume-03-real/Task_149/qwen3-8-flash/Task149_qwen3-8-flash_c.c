#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int *data;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
} Queue;

static bool queue_grow(Queue *q)
{
    size_t new_capacity;
    int *new_data;
    size_t i;

    if (q == NULL || q->data == NULL) {
        return false;
    }

    new_capacity = (q->capacity == 0) ? 1 : q->capacity * 2;
    new_data = malloc(new_capacity * sizeof(int));

    if (new_data == NULL) {
        return false;
    }

    for (i = 0; i < q->count; i++) {
        new_data[i] = q->data[(q->head + i) % q->capacity];
    }

    free(q->data);

    q->data = new_data;
    q->capacity = new_capacity;
    q->head = 0;
    q->tail = q->count;

    return true;
}

bool queue_init(Queue *q, size_t capacity)
{
    if (q == NULL) {
        return false;
    }

    if (capacity == 0) {
        capacity = 1;
    }

    q->data = malloc(capacity * sizeof(int));

    if (q->data == NULL) {
        return false;
    }

    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;

    return true;
}

bool queue_enqueue(Queue *q, int value)
{
    if (q == NULL) {
        return false;
    }

    if (q->data == NULL || q->capacity == 0) {
        return queue_init(q, 1);
    }

    if (q->count == q->capacity) {
        if (!queue_grow(q)) {
            return false;
        }
    }

    q->data[q->tail] = value;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;

    return true;
}

bool queue_dequeue(Queue *q, int *output)
{
    if (q == NULL || output == NULL) {
        return false;
    }

    if (q->count == 0) {
        return false;
    }

    *output = q->data[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;

    return true;
}

bool queue_peek(const Queue *q, int *output)
{
    if (q == NULL || output == NULL) {
        return false;
    }

    if (q->count == 0) {
        return false;
    }

    *output = q->data[q->head];

    return true;
}

void queue_free(Queue *q)
{
    if (q == NULL) {
        return;
    }

    free(q->data);

    q->data = NULL;
    q->capacity = 0;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}

int main(void)
{
    Queue q;
    int value;
    size_t i;
    int input[] = {10, 20, 30, 40, 50};
    size_t input_count = sizeof(input) / sizeof(input[0]);

    if (!queue_init(&q, 2)) {
        fprintf(stderr, "Failed to initialize queue.\n");
        return 1;
    }

    for (i = 0; i < input_count; i++) {
        if (!queue_enqueue(&q, input[i])) {
            fprintf(stderr, "Failed to enqueue %d.\n", input[i]);
            queue_free(&q);
            return 1;
        }
    }

    if (queue_peek(&q, &value)) {
        printf("Peek: %d\n", value);
    } else {
        printf("Queue is empty.\n");
    }

    while (queue_dequeue(&q, &value)) {
        printf("Dequeue: %d\n", value);
    }

    if (queue_peek(&q, &value)) {
        printf("Peek: %d\n", value);
    } else {
        printf("Queue is empty.\n");
    }

    queue_free(&q);

    return 0;
}