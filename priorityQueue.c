#include "priorityQueue.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Initialize the priority queue
void initPQ(PriorityQueue *pq) {
    pq->size = 0;
}

// Check if priority queue is empty
int isPQEmpty(PriorityQueue *pq) {
    return pq->size == 0;
}

// Insert a lane into the priority queue (sorted by priority descending)
void insertLane(PriorityQueue *pq, Lane l) {
    int i = pq->size - 1;

    // Shift lanes with lower priority to the right
    while (i >= 0 && pq->data[i].priority < l.priority) {
        pq->data[i + 1] = pq->data[i];
        i--;
    }

    pq->data[i + 1] = l;
    pq->size++;
}

// Extract the lane with highest priority
Lane extractHighestPriority(PriorityQueue *pq) {
    Lane temp = {"", -1};

    if (isPQEmpty(pq))
        return temp;

    temp = pq->data[0];

    // Shift remaining lanes
    for (int i = 0; i < pq->size - 1; i++) {
        pq->data[i] = pq->data[i + 1];
    }

    pq->size--;
    return temp;
}

// Update priority of a lane (useful for AL2 priority change)
void updatePriority(PriorityQueue *pq, char *name, int newPriority) {
    int found = 0;
    for (int i = 0; i < pq->size; i++) {
        if (strcmp(pq->data[i].name, name) == 0) {
            pq->data[i].priority = newPriority;
            found = 1;
            break;
        }
    }

    if (!found) return;

    // Re-sort the queue after updating priority
    Lane temp[10];
    int oldSize = pq->size;
    for (int i = 0; i < oldSize; i++) {
        temp[i] = pq->data[i];
    }

    pq->size = 0;
    for (int i = 0; i < oldSize; i++) {
        insertLane(pq, temp[i]);
    }
}

// Debug function: Print current queue
void printPQ(PriorityQueue *pq) {
    printf("Priority Queue: ");
    for (int i = 0; i < pq->size; i++) {
        printf("%s(%d) ", pq->data[i].name, pq->data[i].priority);
    }
    printf("\n");
}
