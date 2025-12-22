#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

typedef struct {
    char name[4];   // AL2, BL2 etc
    int priority;
} Lane;

typedef struct {
    Lane data[10];
    int size;
} PriorityQueue;

void initPQ(PriorityQueue *pq);
void insertLane(PriorityQueue *pq, Lane l);
Lane extractHighestPriority(PriorityQueue *pq);
void updatePriority(PriorityQueue *pq, char *name, int p);

#endif
