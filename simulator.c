#include<SDL2/SDL.h>
#include<stdio.h>
#include<stdbool.h>
#include<windows.h>
#include<string.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define ROAD_WIDTH 150
#define LANE_WIDTH 50
#define VEHICLE_WIDTH 40
#define VEHICLE_HEIGHT 30
#define NUM_ROADS 4
#define MAX_QUEUE 50
#define LIGHT_DURATION 3000  // ms

typedef struct {
    int x,y;
    char road;      // 'A','B','C','D'
    bool active;
    bool waiting;
} Vehicle;

typedef struct {
    Vehicle vehicles[MAX_QUEUE];
    int front,rear;
} VehicleQueue;

// SDL objects
SDL_Window* window;
SDL_Renderer* renderer;

// Queues and globals
VehicleQueue queues[NUM_ROADS];
int currentGreen = 0;
Uint32 lastSwitchTime = 0;
SDL_mutex* mutex;

// Initialize queue
void initQueue(VehicleQueue* q){ q->front=0;q->rear=-1; }
bool isEmpty(VehicleQueue* q){ return q->rear<q->front; }
void enqueue(VehicleQueue* q, Vehicle v){ if(q->rear+1<MAX_QUEUE) q->vehicles[++q->rear]=v; }
Vehicle* peek(VehicleQueue* q){ return !isEmpty(q)? &q->vehicles[q->front]:NULL; }
void dequeue(VehicleQueue* q){ if(!isEmpty(q)) q->front++; }

// Spawn vehicle at start of road
Vehicle createVehicle(char road){
    Vehicle v;
    v.road = road;
    v.active = true;
    v.waiting = false;
    switch(road){
        case 'A': v.x=WINDOW_WIDTH/2-LANE_WIDTH; v.y=0; break;
        case 'B': v.x=WINDOW_WIDTH/2-LANE_WIDTH; v.y=WINDOW_HEIGHT; break;
        case 'C': v.x=WINDOW_WIDTH; v.y=WINDOW_HEIGHT/2-LANE_WIDTH; break;
        case 'D': v.x=0; v.y=WINDOW_HEIGHT/2-LANE_WIDTH; break;
    }
    return v;
}

// Draw road junction
void drawRoads(){
    SDL_SetRenderDrawColor(renderer,200,200,200,255);
    SDL_Rect vert={WINDOW_WIDTH/2-ROAD_WIDTH/2,0,ROAD_WIDTH,WINDOW_HEIGHT};
    SDL_Rect hori={0,WINDOW_HEIGHT/2-ROAD_WIDTH/2,WINDOW_WIDTH,ROAD_WIDTH};
    SDL_RenderFillRect(renderer,&vert);
    SDL_RenderFillRect(renderer,&hori);

    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    for(int i=1;i<4;i++){
        SDL_RenderDrawLine(renderer,0,WINDOW_HEIGHT/2-ROAD_WIDTH/2+i*LANE_WIDTH,WINDOW_WIDTH,WINDOW_HEIGHT/2-ROAD_WIDTH/2+i*LANE_WIDTH);
        SDL_RenderDrawLine(renderer,WINDOW_WIDTH/2-ROAD_WIDTH/2+i*LANE_WIDTH,0,WINDOW_WIDTH/2-ROAD_WIDTH/2+i*LANE_WIDTH,WINDOW_HEIGHT);
    }
}

// Draw traffic lights
void drawLights(){
    SDL_Rect rect={0,0,20,20};
    for(int i=0;i<NUM_ROADS;i++){
        SDL_SetRenderDrawColor(renderer,(currentGreen==i)?0:255,(currentGreen==i)?255:0,0,255);
        switch(i){
            case 0: rect.x=WINDOW_WIDTH/2-10; rect.y=10; break;
            case 1: rect.x=WINDOW_WIDTH/2-10; rect.y=WINDOW_HEIGHT-30; break;
            case 2: rect.x=WINDOW_WIDTH-30; rect.y=WINDOW_HEIGHT/2-10; break;
            case 3: rect.x=10; rect.y=WINDOW_HEIGHT/2-10; break;
        }
        SDL_RenderFillRect(renderer,&rect);
    }
}

// Draw vehicles
void drawVehicles(){
    SDL_SetRenderDrawColor(renderer,0,0,255,255);
    for(int i=0;i<NUM_ROADS;i++){
        SDL_LockMutex(mutex);
        for(int j=queues[i].front;j<=queues[i].rear;j++){
            Vehicle* v=&queues[i].vehicles[j];
            if(!v->active) continue;

            if(v->waiting) SDL_SetRenderDrawColor(renderer,255,0,0,255);
            else SDL_SetRenderDrawColor(renderer,0,0,255,255);

            SDL_Rect body={v->x,v->y,VEHICLE_WIDTH,VEHICLE_HEIGHT};
            SDL_RenderFillRect(renderer,&body);

            SDL_SetRenderDrawColor(renderer,0,0,0,255);
            SDL_Rect w1={v->x+2,v->y+2,8,8}, w2={v->x+VEHICLE_WIDTH-10,v->y+2,8,8};
            SDL_Rect w3={v->x+2,v->y+VEHICLE_HEIGHT-10,8,8}, w4={v->x+VEHICLE_WIDTH-10,v->y+VEHICLE_HEIGHT-10,8,8};
            SDL_RenderFillRect(renderer,&w1); SDL_RenderFillRect(renderer,&w2);
            SDL_RenderFillRect(renderer,&w3); SDL_RenderFillRect(renderer,&w4);
        }
        SDL_UnlockMutex(mutex);
    }
}

// Move vehicles
void moveVehicles(){
    for(int i=0;i<NUM_ROADS;i++){
        SDL_LockMutex(mutex);
        for(int j=queues[i].front;j<=queues[i].rear;j++){
            Vehicle* v=&queues[i].vehicles[j];
            if(!v->active) continue;

            int roadIdx=v->road-'A';
            int stopLine;
            switch(v->road){
                case 'A': stopLine=WINDOW_HEIGHT/2-ROAD_WIDTH/2; break;
                case 'B': stopLine=WINDOW_HEIGHT/2+ROAD_WIDTH/2; break;
                case 'C': stopLine=WINDOW_WIDTH/2+ROAD_WIDTH/2; break;
                case 'D': stopLine=WINDOW_WIDTH/2-ROAD_WIDTH/2; break;
            }

            if(roadIdx!=currentGreen){
                if((v->road=='A' && v->y+VEHICLE_HEIGHT>=stopLine)||
                   (v->road=='B' && v->y<=stopLine)||
                   (v->road=='C' && v->x<=stopLine)||
                   (v->road=='D' && v->x+VEHICLE_WIDTH>=stopLine)){
                       v->waiting=true; continue;
                   }
            }
            v->waiting=false;

            switch(v->road){
                case 'A': v->y+=2; if(v->y>WINDOW_HEIGHT)v->active=false; break;
                case 'B': v->y-=2; if(v->y<0)v->active=false; break;
                case 'C': v->x-=2; if(v->x<0)v->active=false; break;
                case 'D': v->x+=2; if(v->x>WINDOW_WIDTH)v->active=false; break;
            }
        }
        SDL_UnlockMutex(mutex);
    }
}

// Thread to read vehicles from pipe/server (simulate here)
DWORD WINAPI vehicleInput(LPVOID arg){
    char roads[]="ABCD";
    while(1){
        Sleep(2000); // every 2 sec
        char road=roads[rand()%4];
        Vehicle v=createVehicle(road);
        SDL_LockMutex(mutex);
        enqueue(&queues[road-'A'],v);
        SDL_UnlockMutex(mutex);
    }
    return 0;
}

int main(){
    srand((unsigned int)time(NULL));
    mutex=SDL_CreateMutex();

    for(int i=0;i<NUM_ROADS;i++) initQueue(&queues[i]);

    SDL_Init(SDL_INIT_VIDEO);
    window=SDL_CreateWindow("Dynamic Traffic Junction",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WINDOW_WIDTH,WINDOW_HEIGHT,SDL_WINDOW_SHOWN);
    renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);

    CreateThread(NULL,0,vehicleInput,NULL,0,NULL);

    lastSwitchTime=SDL_GetTicks();
    bool running=true;
    SDL_Event e;

    while(running){
        while(SDL_PollEvent(&e)){ if(e.type==SDL_QUIT) running=false; }

        Uint32 now=SDL_GetTicks();
        if(now-lastSwitchTime>LIGHT_DURATION){
            currentGreen=(currentGreen+1)%NUM_ROADS;
            lastSwitchTime=now;
        }

        moveVehicles();

        SDL_SetRenderDrawColor(renderer,150,150,150,255);
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
