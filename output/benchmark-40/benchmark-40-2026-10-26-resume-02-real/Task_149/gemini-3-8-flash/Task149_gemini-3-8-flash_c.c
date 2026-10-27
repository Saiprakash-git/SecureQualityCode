#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct Node {
    int data;
    struct Node *next;
} Node;

typedef struct Queue {
    Node *front;
    Node *rear;
} Queue;

void initQueue(Queue *q) {
    if (q != NULL) {
        q->front = NULL;
        q->rear = NULL;
    }
}

bool isEmpty(const Queue *q) {
    return (q == NULL || q->front == NULL);
}

bool enqueue(Queue *q, int value) {
    if (q == NULL) {
        return false;
    }

    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        return false;
    }

    newNode->data = value;
    newNode->next = NULL;

    if (q->rear == NULL) {
        q->front = newNode;
        q->rear = newNode;
    } else {
        q->rear->next = newNode;
        q->rear = newNode;
    }

    return true;
}

bool dequeue(Queue *q, int *result) {
    if (isEmpty(q)) {
        return false;
    }

    Node *temp = q->front;
    if (result != NULL) {
        *result = temp->data;
    }

    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }

    free(temp);
    return true;
}

bool peek(const Queue *q, int *result) {
    if (isEmpty(q) || result == NULL) {
        return false;
    }

    *result = q->front->data;
    return true;
}

void freeQueue(Queue *q) {
    if (q == NULL) {
        return;
    }

    Node *current = q->front;
    while (current != NULL) {
        Node *temp = current;
        current = current->next;
        free(temp);
    }

    q->front = NULL;
    q->rear = NULL;
}

int main(void) {
    Queue q;
    initQueue(&q);

    int val;

    printf("Enqueueing: 10, 20, 30\n");
    enqueue(&q, 10);
    enqueue(&q, 20);
    enqueue(&q, 30);

    if (peek(&q, &val)) {
        printf("Front element (peek): %d\n", val);
    }

    if (dequeue(&q, &val)) {
        printf("Dequeued: %d\n", val);
    }

    if (peek(&q, &val)) {
        printf("Front element after dequeue (peek): %d\n", val);
    }

    printf("Enqueueing: 40\n");
    enqueue(&q, 40);

    while (dequeue(&q, &val)) {
        printf("Dequeued: %d\n", val);
    }

    if (!dequeue(&q, &val)) {
        printf("Queue is now empty.\n");
    }

    freeQueue(&q);
    return 0;
}