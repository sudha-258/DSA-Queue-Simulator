🚦 DSA-Queue-Simulator

 
The purpose of this assignment was to simulate traffic management at a four-road junction using queue data structures. Each road has multiple lanes, with one priority lane per road. The system is designed to:

Serve vehicles fairly under normal conditions.

Give priority to lanes with higher vehicle accumulation.

Manage traffic lights to avoid collisions.

Estimate the time required for vehicles to pass the junction.

The project is divided into two main programs:

traffic_generator.c – Generates vehicles randomly for each lane.

simulator.c – Processes vehicles, updates queues, and visualizes the junction using SDL2.

Two types of queues are used:

Vehicle Queue: Stores vehicles waiting on each lane.


Lane/Light Priority Queue: Determines which lane is served next based on vehicle counts and priority conditions.

