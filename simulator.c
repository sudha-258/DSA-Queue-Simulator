#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <string.h>
#include "queue.h"
#include "priorityQueue.h"
#include<time.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define VEHICLE_WIDTH 40
#define VEHICLE_HEIGHT 30
#define NUM_ROADS 4
#define MAX_VISUAL 50
#define LIGHT_DURATION 3000 // ms

typedef struct {
    Vehicle v;  // logical vehicle (from queue)
    int x, y;   // screen coordinates
    bool active;
    bool waiting;
} VisualVehicle;

// SDL objects
SDL_Window* window;
SDL_Renderer* renderer;
SDL_mutex* mutex;

// Vehicle queues for each road (incoming lane)
Queue roadQueues[NUM_ROADS];

// Visual vehicle array
VisualVehicle visualQueue[MAX_VISUAL];
int visualCount = 0;

// Traffic light
int currentGreen = 0;
Uint32 lastSwitchTime = 0;

// -------------------- Queue helpers --------------------
void initVehicleQueues() {
    for (int i = 0; i < NUM_ROADS; i++) {
        initQueue(&roadQueues[i]);
    }
}

// -------------------- Vehicle creation --------------------
VisualVehicle createVisualVehicle(Vehicle v) {
    VisualVehicle vis;
    vis.v = v;
    vis.active = true;
    vis.waiting = false;

    // Set starting coordinates based on road
    switch (v.road) {
        case 'A': vis.x = WINDOW_WIDTH/2 - LANE_WIDTH; vis.y = 0; break;
        case 'B': vis.x = WINDOW_WIDTH/2 - LANE_WIDTH; vis.y = WINDOW_HEIGHT; break;
        case 'C': vis.x = WINDOW_WIDTH; vis.y = WINDOW_HEIGHT/2 - LANE_WIDTH; break;
        case 'D': vis.x = 0; vis.y = WINDOW_HEIGHT/2 - LANE_WIDTH; break;
    }

    return vis;
}

// -------------------- Draw functions --------------------
void drawRoads() {
    SDL_SetRenderDrawColor(renderer, 200,200,200,255);
    SDL_Rect vert = {WINDOW_WIDTH/2 - ROAD_WIDTH/2, 0, ROAD_WIDTH, WINDOW_HEIGHT};
    SDL_Rect hori = {0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2, WINDOW_WIDTH, ROAD_WIDTH};
    SDL_RenderFillRect(renderer, &vert);
    SDL_RenderFillRect(renderer, &hori);

    SDL_SetRenderDrawColor(renderer, 0,0,0,255);
    for (int i = 1; i < 4; i++) {
        SDL_RenderDrawLine(renderer, 0, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, WINDOW_WIDTH, WINDOW_HEIGHT/2 - ROAD_WIDTH/2 + i*LANE_WIDTH);
        SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, 0, WINDOW_WIDTH/2 - ROAD_WIDTH/2 + i*LANE_WIDTH, WINDOW_HEIGHT);
    }
}

void drawLights() {
    SDL_Rect rect = {0,0,20,20};
    for (int i = 0; i < NUM_ROADS; i++) {
        SDL_SetRenderDrawColor(renderer, (currentGreen==i)?0:255, (currentGreen==i)?255:0, 0, 255);
        switch(i){
            case 0: rect.x=WINDOW_WIDTH/2-10; rect.y=10; break;
            case 1: rect.x=WINDOW_WIDTH/2-10; rect.y=WINDOW_HEIGHT-30; break;
            case 2: rect.x=WINDOW_WIDTH-30; rect.y=WINDOW_HEIGHT/2-10; break;
            case 3: rect.x=10; rect.y=WINDOW_HEIGHT/2-10; break;
        }
        SDL_RenderFillRect(renderer,&rect);
    }
}

void drawVehicles() {
    SDL_SetRenderDrawColor(renderer, 0,0,255,255);
    SDL_LockMutex(mutex);
    for (int i = 0; i < visualCount; i++) {
        if (!visualQueue[i].active) continue;

        if (visualQueue[i].waiting)
            SDL_SetRenderDrawColor(renderer, 255,0,0,255);
        else
            SDL_SetRenderDrawColor(renderer, 0,0,255,255);

        SDL_Rect body = { visualQueue[i].x, visualQueue[i].y, VEHICLE_WIDTH, VEHICLE_HEIGHT };
        SDL_RenderFillRect(renderer, &body);
    }
    SDL_UnlockMutex(mutex);
}

// -------------------- Move vehicles --------------------
void moveVehicles() {
    SDL_LockMutex(mutex);
    for (int i = 0; i < visualCount; i++) {
        if (!visualQueue[i].active) continue;

        char road = visualQueue[i].v.road;
        int stopLine;

        switch(road){
            case 'A': stopLine = WINDOW_HEIGHT/2 - ROAD_WIDTH/2; break;
            case 'B': stopLine = WINDOW_HEIGHT/2 + ROAD_WIDTH/2; break;
            case 'C': stopLine = WINDOW_WIDTH/2 + ROAD_WIDTH/2; break;
            case 'D': stopLine = WINDOW_WIDTH/2 - ROAD_WIDTH/2; break;
        }

        int roadIdx = road - 'A';
        if (roadIdx != currentGreen) {
            if ((road=='A' && visualQueue[i].y+VEHICLE_HEIGHT>=stopLine) ||
                (road=='B' && visualQueue[i].y<=stopLine) ||
                (road=='C' && visualQueue[i].x<=stopLine) ||
                (road=='D' && visualQueue[i].x+VEHICLE_WIDTH>=stopLine)) {
                visualQueue[i].waiting = true;
                continue;
            }
        }
        visualQueue[i].waiting = false;

        switch(road){
            case 'A': visualQueue[i].y += 2; if (visualQueue[i].y>WINDOW_HEIGHT) visualQueue[i].active=false; break;
            case 'B': visualQueue[i].y -= 2; if (visualQueue[i].y<0) visualQueue[i].active=false; break;
            case 'C': visualQueue[i].x -= 2; if (visualQueue[i].x<0) visualQueue[i].active=false; break;
            case 'D': visualQueue[i].x += 2; if (visualQueue[i].x>WINDOW_WIDTH) visualQueue[i].active=false; break;
        }
    }
    SDL_UnlockMutex(mutex);
}

// -------------------- Read vehicles from file --------------------
void readVehiclesFromFile(Queue* q, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return;

    char road;
    int lane;
    char vehicleID[9];

    while (fscanf(file, " %c %d %8s", &road, &lane, vehicleID) == 3) {
        Vehicle v;
        v.id = atoi(vehicleID); // or generate sequential id
        v.road = road;
        v.lane = lane;
        v.arrivalTime = SDL_GetTicks();
        enqueue(q, v);
    }
    fclose(file);

    // clear file
    file = fopen(filename, "w");
    if (file) fclose(file);
}

// -------------------- Vehicle generation thread --------------------
DWORD WINAPI vehicleInput(LPVOID arg){
    char roads[] = "ABCD";
    while(1){
        Sleep(2000); // every 2 sec
        char road = roads[rand()%4];
        Vehicle v;
        v.id = rand()%1000;
        v.road = road;
        v.lane = 2;
        v.arrivalTime = SDL_GetTicks();

        SDL_LockMutex(mutex);
        enqueue(&roadQueues[road-'A'], v);
        if (visualCount < MAX_VISUAL) {
            visualQueue[visualCount++] = createVisualVehicle(v);
        }
        SDL_UnlockMutex(mutex);
    }
    return 0;
}

// -------------------- Main --------------------
int main(int argc, char* argv[])
{
    srand((unsigned int)time(NULL));
    mutex = SDL_CreateMutex();
    initVehicleQueues();

    // Read initial vehicles from files (example)
    readVehiclesFromFile(&roadQueues[0], "laneA.txt");
    readVehiclesFromFile(&roadQueues[1], "laneB.txt");
    readVehiclesFromFile(&roadQueues[2], "laneC.txt");
    readVehiclesFromFile(&roadQueues[3], "laneD.txt");

    // Initialize SDL
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Traffic Junction", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    CreateThread(NULL, 0, vehicleInput, NULL, 0, NULL);

    lastSwitchTime = SDL_GetTicks();
    bool running = true;
    SDL_Event e;

    while(running) {
        while(SDL_PollEvent(&e)) if(e.type == SDL_QUIT) running=false;

        Uint32 now = SDL_GetTicks();
        if (now - lastSwitchTime > LIGHT_DURATION) {
            currentGreen = (currentGreen + 1) % NUM_ROADS;
            lastSwitchTime = now;
        }

        moveVehicles();

        SDL_SetRenderDrawColor(renderer, 150,150,150,255);
        SDL_RenderClear(renderer);

        drawRoads();
        drawLights();
        drawVehicles();

        SDL_RenderPresent(renderer);
        SDL_Delay(20);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyMutex(mutex);
    SDL_Quit();
    return 0;
}
