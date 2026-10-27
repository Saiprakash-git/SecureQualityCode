#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node *next;
} Node;

typedef struct Queue {
    Node *front;
    Node *rear;
    size_t size;
} Queue;

Queue *queue_create(void) {
    Queue *q = malloc(sizeof(Queue));
    if (!q) return NULL;
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
    return q;
}

int queue_is_empty(const Queue *q) {
    return q == NULL || q->front == NULL;
}

int queue_enqueue(Queue *q, int value) {
    if (!q) return 0;
    Node *node = malloc(sizeof(Node));
    if (!node) return 0;
    node->data = value;
    node->next = NULL;
    if (q->rear) {
        q->rear->next = node;
    } else {
        q->front = node;
    }
    q->rear = node;
    q->size++;
    return 1;
}

int queue_dequeue(Queue *q, int *out) {
    if (queue_is_empty(q)) return 0;
    Node *node = q->front;
    if (out) *out = node->data;
    q->front = node->next;
    if (!q->front) q->rear = NULL;
    free(node);
    q->size--;
    return 1;
}

int queue_peek(const Queue *q, int *out) {
    if (queue_is_empty(q)) return 0;
    if (out) *out = q->front->data;
    return 1;
}

size_t queue_size(const Queue *q) {
    return q ? q->size : 0;
}

void queue_destroy(Queue *q) {
    if (!q) return;
    Node *cur = q->front;
    while (cur) {
        Node *next = cur->next;
        free(cur);
        cur = next;
    }
    free(q);
}

int main(void) {
    Queue *q = queue_create();
    if (!q) {
        fprintf(stderr, "Failed to create queue\n");
        return EXIT_FAILURE;
    }

    for (int i = 1; i <= 5; i++) {
        queue_enqueue(q, i * 10);
        printf("Enqueued: %d\n", i * 10);
    }

    int value;
    if (queue_peek(q, &value)) {
        printf("Peek: %d\n", value);
    }

    printf("Size: %zu\n", queue_size(q));

    while (!queue_is_empty(q)) {
        if (queue_dequeue(q, &value)) {
            printf("Dequeued: %d\n", value);
        }
    }

    if (!queue_peek(q, &value)) {
        printf("Queue is empty, peek failed\n");
    }

    queue_destroy(q);
    return EXIT_SUCCESS;
}