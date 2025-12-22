#include "queue.h"

/* Initialize queue */
void initQueue(Queue *q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

/* Check empty */
int isEmpty(Queue *q) {
    return q->count == 0;
}

/* Check full */
int isFull(Queue *q) {
    return q->count == MAX_QUEUE;
}

/* Enqueue */
int enqueue(Queue *q, Vehicle v) {
    if (isFull(q)) return 0;

    q->rear = (q->rear + 1) % MAX_QUEUE;
    q->data[q->rear] = v;
    q->count++;
    return 1;
}

/* Dequeue */
Vehicle dequeue(Queue *q) {
    Vehicle dummy = {-1, '?', -1, -1};

    if (isEmpty(q))
        return dummy;

    Vehicle v = q->data[q->front];
    q->front = (q->front + 1) % MAX_QUEUE;
    q->count--;
    return v;
}


/* Size */
int queueSize(Queue *q) {
    return q->count;
}
