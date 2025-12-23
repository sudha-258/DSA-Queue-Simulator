#include <stdio.h>
#include <stdlib.h>
<<<<<<< HEAD
#include <unistd.h>   // for sleep()
=======
#include <windows.h>   // for sleep()
>>>>>>> 18eb85ad4ab8badc84ebcaa47b77195615f54394

#define MAX 10

// Queue structure
typedef struct {
    int car[MAX];
    int front, rear;
} Queue;

// Function declarations
void initQueue(Queue *q);
int isEmpty(Queue *q);
int isFull(Queue *q);
void enqueue(Queue *q, int value);
int dequeue(Queue *q);
void showStatus(Queue A, Queue B, Queue C, Queue D);

int main() {
    Queue A, B, C, D;
    initQueue(&A);
    initQueue(&B);
    initQueue(&C);
    initQueue(&D);

    // Add cars to queues
    for(int i = 1; i <= 5; i++) {
        enqueue(&A, i);
        enqueue(&B, i + 10);
        enqueue(&C, i + 20);
        enqueue(&D, i + 30);
    }

    int green = 0; // 0=A, 1=B, 2=C, 3=D

    while(1) {
        system("cls"); // use "clear" on Linux

        showStatus(A, B, C, D);

        printf("\nGREEN LIGHT: Road %c\n", 'A' + green);

        switch(green) {
            case 0:
                if(!isEmpty(&A))
                    printf("Car %d passed from Road A\n", dequeue(&A));
                break;
            case 1:
                if(!isEmpty(&B))
                    printf("Car %d passed from Road B\n", dequeue(&B));
                break;
            case 2:
                if(!isEmpty(&C))
                    printf("Car %d passed from Road C\n", dequeue(&C));
                break;
            case 3:
                if(!isEmpty(&D))
                    printf("Car %d passed from Road D\n", dequeue(&D));
                break;
        }

        green = (green + 1) % 4; // rotate light
        sleep(2); // 2 seconds per signal
    }

    return 0;
}

// Queue functions
void initQueue(Queue *q) {
    q->front = q->rear = -1;
}

int isEmpty(Queue *q) {
    return q->front == -1;
}

int isFull(Queue *q) {
    return q->rear == MAX - 1;
}

void enqueue(Queue *q, int value) {
    if(isFull(q)) return;
    if(isEmpty(q)) q->front = 0;
    q->rear++;
    q->car[q->rear] = value;
}

int dequeue(Queue *q) {
    int value = q->car[q->front];
    q->front++;
    if(q->front > q->rear)
        q->front = q->rear = -1;
    return value;
}

void showStatus(Queue A, Queue B, Queue C, Queue D) {
    printf("TRAFFIC JUNCTION STATUS\n");
    printf("------------------------\n");
    printf("Road A cars: %d\n", A.rear - A.front + 1);
    printf("Road B cars: %d\n", B.rear - B.front + 1);
    printf("Road C cars: %d\n", C.rear - C.front + 1);
    printf("Road D cars: %d\n", D.rear - D.front + 1);
}
