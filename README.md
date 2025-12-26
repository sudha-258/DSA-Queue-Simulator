🚦<b> DSA-Queue-Simulator</b><br>
This project simulates a four-way traffic junction using queue data structures and traffic light timing control. Vehicles are generated randomly in different lanes, stored in queues, and move through the junction based on the active green signal. The system demonstrates how queues help manage traffic flow, avoid congestion, and prioritize smooth vehicle movement at intersections.

<b>Project Demo</b><br>
  ![ Demo](Demo.gif)



<b>Steps to run the program</b><br>
   •Requirement:<br>
    -GCC compiler<br>
    -SDL2<br>
    
    •Compilation code:<br>
     -For traffic_generator.c:   gcc traffic_generator.c -o generator.exe <br>
     -For simulator.c:  gcc simulator.c queue.c priorityQueue.c -I"C:\Users\Sudha Karki\Downloads\Traffic Light\SDL2\include" -L"C:\Users\Sudha                                               Karki\Downloads\Traffic Light\SDL2\lib" -lmingw32 -lSDL2main -lSDL2 -o sim.exe<br>
     
    •To run:<br>
      -For traffic_Generator.c: ./generator<br>
      -For simulator.c: ./sim<br>

 <b> Key Features</b>

•Queue-Based & Efficient Traffic Management:<br>
 Vehicles are stored and processed using queue and circular buffer structures, enabling organized, efficient lane handling.<br>

•Automated Multi-Lane Signal Control:<br>
 Traffic lights switch automatically at fixed intervals, and vehicles move only when their respective road receives a green signal.<br>

•Real-Time Simulation with SDL2:<br>
 Roads, lanes, vehicles, and signals are visually rendered with real-time movement and continuous vehicle generation.<br>

•Extendable & Multithreaded System<br>
 Supports future priority / emergency lanes and uses a separate thread for smooth, continuous vehicle generation and processing.


📊<b>Data Structure</b>
| Data Structure  | Implementation | Purpose                                       |
|---------------- |----------------|-----------------------------------------------|
| Queue           | Circular Array | Store vehicles in each lane (12 queues total) |
| Priority Queue  | Min-Heap       | Manage road serving priority                  |
| Mutex Lock      | SDL_mutex      | Thread-safe queue operations                  |




🧩 <b>Algorithm Design</b><br>
The algorithm used for processing traffic are<br>
• Vehicle Generation:<br>
  -Randomly generate vehicles in any lane at intervals (1 sec)<br>

• Traffic Light Control:<br>
  -Cycle through roads, one green light at a time, for LIGHT_DURATION<br>

• Vehicle Movement:<br>
 – Vehicles move only if their road’s light is green<br>
 – High-priority vehicles handled via priority queue if implemented<br>
 
• Queue Management:<br>
 – Vehicles enter lane queues in order<br>
 – Vehicles leave queue when they cross the junction.<br>

 ⏱️<b>Time Complexity Analysis</b>

• Enqueue / Dequeue (Queue): O(1) – constant time for adding/removing a vehicle<br>
• Queue Iteration for Movement: O(n) – iterate over all vehicles in a lane<br>
• Traffic Light Switching: O(1) – simple modulo arithmetic<br>
• Overall Complexity per Frame: O(total vehicles)

 





















