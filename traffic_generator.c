#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#define GENERATION_INTERVAL 2000// Generate vehicle every 2 seconds
#define AL2_PRIORITY_CHANCE 30    // 30% chance for AL2 (priority lane)

typedef struct {
    char road;
    int lane;
    unsigned int timestamp;
} VehicleData;



void generateVehicle(VehicleData* v) {
    int rand_val = rand() % 100;
    
    // Higher chance for AL2 (priority lane) to test priority system
    if(rand_val < AL2_PRIORITY_CHANCE) {
        v->road = 'A';
        v->lane = 2;  // AL2 is priority lane
    } else {
        char roads[] = "ABCD";
        v->road = roads[rand() % 4];
        if(v->road =='A'){
            int lane_rand = rand() %100;
            if(lane_rand < 70){
                v-> lane= 2;
            }else{
                v->lane = 3;
            }
            
        }
        else if(v->road == 'B'){
            int lane_rand = rand()%100;
            if(lane_rand < 60){
                v-> lane =2;
    
            }
            else{
                v-> lane = 3;
            }
        }
         else if(v->road == 'C'){
            int lane_rand = rand()%100;
            if(lane_rand < 60){
                v-> lane =2;
    
            }
            else{
                v-> lane = 3;
            }
        }
        else if(v->road == 'D'){
            int lane_rand = rand()%100;
            if(lane_rand < 60){
                v-> lane =2;
    
            }
            else{
                v-> lane = 3;
            }
        }
    }
    
    v->timestamp = (unsigned int)time(NULL);
}

void writeVehicleToFile(VehicleData* v) {
    char filename[32];
    
    switch(v->road) {
        case 'A': sprintf(filename, "lanea.txt"); break;
        case 'B': sprintf(filename, "laneb.txt"); break;
        case 'C': sprintf(filename, "lanec.txt"); break;
        case 'D': sprintf(filename, "laned.txt"); break;
        default: return;
    }
    
    FILE* f = fopen(filename, "a");
    if(!f) {
        printf("Error: Could not open %s\n", filename);
        return;
    }
    
    fprintf(f, "%d\n", v->lane);
    fclose(f);
    
    printf("Generated: Road %c, Lane %d at %u\n", 
           v->road, v->lane, v->timestamp);
}


int main(int argc, char* argv[])
{
    srand((unsigned int)time(NULL));
    
    // Create empty lane files
    FILE* f;
    f = fopen("lanea.txt", "w"); if(f) fclose(f);
    f = fopen("laneb.txt", "w"); if(f) fclose(f);
    f = fopen("lanec.txt", "w"); if(f) fclose(f);
    f = fopen("laned.txt", "w"); if(f) fclose(f);
   
    int vehicleCount = 0;
    int counts[4][3] = {0};  // [road][lane]
    
    while(1) {
        VehicleData v;
        generateVehicle(&v);
        writeVehicleToFile(&v);
        
        vehicleCount++;
        int roadIdx = v.road - 'A';
        counts[roadIdx][v.lane - 1]++;
        
    
        
        Sleep(GENERATION_INTERVAL);
    }
    
    return 0;
}