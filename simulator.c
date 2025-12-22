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

// Initialize all queues
void initAllQueues() {
    initQueue(&AL1); initQueue(&AL2); initQueue(&AL3);
    initQueue(&BL1); initQueue(&BL2); initQueue(&BL3);
    initQueue(&CL1); initQueue(&CL2); initQueue(&CL3);
    initQueue(&DL1); initQueue(&DL2); initQueue(&DL3);
}

// Create a vehicle
Vehicle createVehicle(char road, int lane) {
    static int vehicleID = 1;
    Vehicle v;
    v.id = vehicleID++;
    v.road = road;
    v.lane = lane;
    v.arrivalTime = SDL_GetTicks();
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
void drawQueue(Queue* q, char road) {
    for(int i=0;i<queueSize(q);i++){
        Vehicle v = q->data[i];
        SDL_Rect body;
        switch(road){
            case 'A': body = (SDL_Rect){WINDOW_WIDTH/2-LANE_WIDTH, v.id*5, VEHICLE_WIDTH, VEHICLE_HEIGHT}; break;
            case 'B': body = (SDL_Rect){WINDOW_WIDTH/2, WINDOW_HEIGHT - v.id*5, VEHICLE_WIDTH, VEHICLE_HEIGHT}; break;
            case 'C': body = (SDL_Rect){WINDOW_WIDTH - v.id*5, WINDOW_HEIGHT/2-LANE_WIDTH, VEHICLE_WIDTH, VEHICLE_HEIGHT}; break;
            case 'D': body = (SDL_Rect){v.id*5, WINDOW_HEIGHT/2, VEHICLE_WIDTH, VEHICLE_HEIGHT}; break;
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderFillRect(renderer, &body);
    }
}

// Move vehicles (simplified)
void moveVehicles() {
    // For demo, just mark vehicles as passed after some time
    Uint32 now = SDL_GetTicks();
    // Here you could implement actual movement logic
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
        drawQueue(&AL2,'A');
        drawQueue(&BL2,'B');
        drawQueue(&CL2,'C');
        drawQueue(&DL2,'D');

        SDL_RenderPresent(renderer);
        SDL_Delay(20);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyMutex(mutex);
    SDL_Quit();
    return 0;
}
