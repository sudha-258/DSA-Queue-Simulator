#include <stdio.h>
#include "queue.h"

int main() {
    Queue q;
    initQueue(&q);

    Vehicle v1 = {1, 'A', 0, 2};
    Vehicle v2 = {2, 'A', 1, 2};
    Vehicle v3 = {3, 'A', 2, 2};

    enqueue(&q, v1);
    enqueue(&q, v2);
    enqueue(&q, v3);

    Vehicle x = dequeue(&q);
    printf("Dequeued vehicle id: %d\n", x.id);

    x = dequeue(&q);
    printf("Dequeued vehicle id: %d\n", x.id);

    return 0;
}
