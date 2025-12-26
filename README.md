🚦 DSA-Queue-Simulator

 

📋Project Overview

This project simulates a four-way traffic junction using queue data structures and traffic light timing control. Vehicles are generated randomly in different lanes, stored in queues, and move through the junction based on the active green signal. The system demonstrates how queues help manage traffic flow, avoid congestion, and prioritize smooth vehicle movement at intersections.

 🔑<b> Key Features</b>

–🚗 Queue-Based Traffic Management:
Vehicles in each lane are stored and processed using queue data structures.

–🚦 Automatic Traffic Light Switching:
Signals change after fixed time intervals to control traffic flow.

–🛣️ Multi-Lane Road System:
Each road contains multiple lanes handled independently.

–⚙️ Real-Time Vehicle Movement:
Vehicles move only when their road gets a green signal.

–🎯 Supports Priority / Special Lanes (Extendable):
The structure allows future implementation of emergency or VIP lanes.

–🧮 Efficient Data Structure Implementation:
Demonstrates practical use of queues and circular buffers.

–🖥️ Graphical Simulation using SDL2:
Roads, lanes, vehicles, and traffic lights are visually rendered.

–⏱️ Random Vehicle Generation System:
New vehicles are generated at regular time intervals.

–🧵 Multithreaded Vehicle Processing:
Separate thread manages continuous vehicle generation.


📊Data Structure
| Data Structure  | Implementation | Purpose                                       |
|---------------- |----------------|-----------------------------------------------|
| Queue           | Circular Array | Store vehicles in each lane (12 queues total) |
| Priority Queue  | Min-Heap       | Manage road serving priority                  |
| Mutex Lock      | SDL_mutex      | Thread-safe queue operations                  |




🧩 <b>Algorithm Design</b>
The algorithm used for processing traffic are<br>
• Vehicle Generation: Randomly generate vehicles in any lane at intervals (1 sec)<br>
• Traffic Light Control: Cycle through roads, one green light at a time, for LIGHT_DURATION<br>
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

 







