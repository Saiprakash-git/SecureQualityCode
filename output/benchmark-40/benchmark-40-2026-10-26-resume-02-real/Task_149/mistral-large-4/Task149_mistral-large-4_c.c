#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_SIZE 100

typedef struct {
    int items[MAX_SIZE];
    int front;
    int rear;
    int size;
} Queue;

void init_queue(Queue *q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

bool is_empty(Queue *q) {
    return q->size == 0;
}

bool is_full(Queue *q) {
    return q->size == MAX_SIZE;
}

bool enqueue(Queue *q, int value) {
    if (is_full(q)) {
        return false;
    }
    q->rear = (q->rear + 1) % MAX_SIZE;
    q->items[q->rear] = value;
    q->size++;
    return true;
}

bool dequeue(Queue *q, int *value) {
    if (is_empty(q)) {
        return false;
    }
    *value = q->items[q->front];
    q->front = (q->front + 1) % MAX_SIZE;
    q->size--;
    return true;
}

bool peek(Queue *q, int *value) {
    if (is_empty(q)) {
        return false;
    }
    *value = q->items[q->front];
    return true;
}

int main() {
    Queue q;
    init_queue(&q);

    enqueue(&q, 10);
    enqueue(&q, 20);
    enqueue(&q, 30);

    int val;
    if (peek(&q, &val)) {
        printf("Peek: %d\n", val);
    }

    while (dequeue(&q, &val)) {
        printf("Dequeued: %d\n", val);
    }

    return 0;
}