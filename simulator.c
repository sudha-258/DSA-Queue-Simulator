#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <time.h>
#include "queue.h"
#include "priorityQueue.h" // your priority queue header
#include "junction.h"       // if needed

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define VEHICLE_WIDTH 40
#define VEHICLE_HEIGHT 30
#define LANES_PER_ROAD 3
#define VEHICLE_SPEED 2  // pixels per frame

#define NUM_ROADS 4
#define LIGHT_DURATION 5000  // ms

// SDL objects
SDL_Window* window;
SDL_Renderer* renderer;
SDL_mutex* mutex;

// Traffic lights
int currentGreen = 0;
Uint32 lastSwitchTime = 0;

// Queues for lanes
Queue AL1, AL2, AL3;
Queue BL1, BL2, BL3;
Queue CL1, CL2, CL3;
Queue DL1, DL2, DL3;

Queue* lanes[12];

// Initialize all queues
void initAllQueues() {
    initQueue(&AL1); initQueue(&AL2); initQueue(&AL3);
    initQueue(&BL1); initQueue(&BL2); initQueue(&BL3);
    initQueue(&CL1); initQueue(&CL2); initQueue(&CL3);
    initQueue(&DL1); initQueue(&DL2); initQueue(&DL3);
}
void initLanesArray() {
    lanes[0] = &AL1; lanes[1] = &AL2; lanes[2] = &AL3;
    lanes[3] = &BL1; lanes[4] = &BL2; lanes[5] = &BL3;
    lanes[6] = &CL1; lanes[7] = &CL2; lanes[8] = &CL3;
    lanes[9] = &DL1; lanes[10]= &DL2; lanes[11]= &DL3;
}


// Create a vehicle
Vehicle createVehicle(char road, int lane) {
    static int vehicleID = 1;
    Vehicle v;
    v.id = vehicleID++;
    v.road = road;
    v.lane = lane;
    v.active = true;       // important
    v.arrivalTime = SDL_GetTicks();

    // set initial x,y based on road and lane
    switch(road) {
        case 'A': v.x = WINDOW_WIDTH/2 - ROAD_WIDTH/2 + (lane-1)*LANE_WIDTH + (LANE_WIDTH-VEHICLE_WIDTH)/2; v.y = 0; break;
        case 'B': v.x = WINDOW_WIDTH/2 - ROAD_WIDTH/2 + (lane-1)*LANE_WIDTH + (LANE_WIDTH-VEHICLE_WIDTH)/2; v.y = WINDOW_HEIGHT - VEHICLE_HEIGHT; break;
        case 'C': v.x = WINDOW_WIDTH - VEHICLE_WIDTH; v.y = WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + (lane-1)*LANE_WIDTH + (LANE_WIDTH-VEHICLE_HEIGHT)/2; break;
        case 'D': v.x = 0; v.y = WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + (lane-1)*LANE_WIDTH + (LANE_WIDTH-VEHICLE_HEIGHT)/2; break;
    }
    return v;
}


// Draw the road junction
void drawRoads() {
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_Rect vert = {WINDOW_WIDTH/2 - ROAD_WIDTH/2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_Rect hori = {0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &vert);
    SDL_RenderFillRect(renderer, &hori);

    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
    for(int i=1;i<4;i++){
        SDL_RenderDrawLine(renderer, 0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH,
                           WINDOW_WIDTH, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH);
        SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, 0,
                           WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, WINDOW_HEIGHT);
    }
}

// Draw traffic lights
void drawLights() {
    SDL_Rect rect = {0,0,20,20};
    for(int i=0;i<NUM_ROADS;i++){
        SDL_SetRenderDrawColor(renderer, (currentGreen==i)?0:255, (currentGreen==i)?255:0,0,255);
        switch(i){
            case 0: rect.x=WINDOW_WIDTH/2-10; rect.y=10; break;
            case 1: rect.x=WINDOW_WIDTH/2-10; rect.y=WINDOW_HEIGHT-30; break;
            case 2: rect.x=WINDOW_WIDTH-30; rect.y=WINDOW_HEIGHT/2-10; break;
            case 3: rect.x=10; rect.y=WINDOW_HEIGHT/2-10; break;
        }
        SDL_RenderFillRect(renderer, &rect);
    }
}

// Draw vehicles in a queue
void drawQueue(Queue* q) {
    for(int i=0;i<queueSize(q);i++){
        Vehicle v = q->data[(q->front + i) % MAX_QUEUE];
        if(!v.active) continue;
        SDL_Rect body = {v.x, v.y, VEHICLE_WIDTH, VEHICLE_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(renderer, &body);
    }
}


// Move vehicles (simplified)
void moveVehicles() {
    SDL_LockMutex(mutex);
    for(int i=0;i<12;i++){
        Queue *q = lanes[i];
        for(int j=0;j<queueSize(q);j++){
            Vehicle* v = &q->data[(q->front+j)%MAX_QUEUE];
            if(!v->active) continue;

            int roadIdx = v->road - 'A';
            if(roadIdx == currentGreen){
                switch(v->road){
                    case 'A': v->y += VEHICLE_SPEED; if(v->y > WINDOW_HEIGHT) v->active=false; break;
                    case 'B': v->y -= VEHICLE_SPEED; if(v->y < 0) v->active=false; break;
                    case 'C': v->x -= VEHICLE_SPEED; if(v->x < 0) v->active=false; break;
                    case 'D': v->x += VEHICLE_SPEED; if(v->x > WINDOW_WIDTH) v->active=false; break;
                }
            }
        }
    }
    SDL_UnlockMutex(mutex);
}


// Thread to generate vehicles continuously
DWORD WINAPI vehicleGenerator(LPVOID arg){
    char roads[] = "ABCD";
    while(1){
        Sleep(1000); // 1 second
        char road = roads[rand()%4];
        int lane = (rand()%3)+1;

        Vehicle v = createVehicle(road, lane);
        SDL_LockMutex(mutex);
        switch(road){
            case 'A': enqueue(&AL2, v); break;
            case 'B': enqueue(&BL2, v); break;
            case 'C': enqueue(&CL2, v); break;
            case 'D': enqueue(&DL2, v); break;
        }
        SDL_UnlockMutex(mutex);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    initAllQueues();
    initLanesArray();

    srand((unsigned int)time(NULL));
    mutex = SDL_CreateMutex();
    initAllQueues();

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Traffic Junction Simulator",
                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    CreateThread(NULL, 0, vehicleGenerator, NULL, 0, NULL);

    lastSwitchTime = SDL_GetTicks();
    bool running = true;
    SDL_Event e;

    while(running){
        while(SDL_PollEvent(&e)) if(e.type==SDL_QUIT) running=false;

        Uint32 now = SDL_GetTicks();
        if(now - lastSwitchTime > LIGHT_DURATION){
            currentGreen = (currentGreen+1)%NUM_ROADS;
            lastSwitchTime = now;
        }

        moveVehicles();

        SDL_SetRenderDrawColor(renderer, 150,150,150,255);
        SDL_RenderClear(renderer);

        drawRoads();
        drawLights();
        drawQueue(&AL1); drawQueue(&AL2); drawQueue(&AL3);
        drawQueue(&BL1); drawQueue(&BL2); drawQueue(&BL3);
        drawQueue(&CL1); drawQueue(&CL2); drawQueue(&CL3);
        drawQueue(&DL1); drawQueue(&DL2); drawQueue(&DL3);


        SDL_RenderPresent(renderer);
        SDL_Delay(20);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyMutex(mutex);
    SDL_Quit();
    return 0;
}
