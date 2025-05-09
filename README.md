# Operating System Scheduler with Memory Allocation

This project is an implementation of a CPU scheduler with memory management for a simulated operating system, developed as part of the CMPN303 Operating Systems course at Cairo University, Faculty of Engineering, Computer Engineering Department. The scheduler supports three scheduling algorithms: Non-preemptive Highest Priority First (HPF), Shortest Remaining Time Next (SRTN), and Round Robin (RR). It also integrates a buddy memory allocation system to manage memory for processes. The system is built in C on a Linux platform, utilizing inter-process communication (IPC) techniques and efficient data structures.

## Project Overview

The project simulates an operating system scheduler running on a single-core CPU with infinite memory (Phase 1) and a 1024-byte memory limit with buddy allocation (Phase 2). It consists of four main components:

1. **Process Generator**: Reads process data from an input file, allows the user to select a scheduling algorithm, and sends process information to the scheduler at appropriate times.
2. **Clock Module**: Emulates an integer-based time clock (pre-built).
3. **Scheduler**: The core component that manages process execution based on the chosen algorithm, tracks process states via Process Control Blocks (PCBs), and handles memory allocation/deallocation.
4. **Process**: Simulates CPU-bound processes that notify the scheduler upon completion.

### Objectives
- Evaluate and implement different CPU scheduling algorithms.
- Practice IPC techniques for process communication.
- Optimize memory and time usage through efficient algorithms and data structures.
- Integrate memory management using the buddy allocation system.

### Features
- **Scheduling Algorithms**:
    - **Non-preemptive Highest Priority First (HPF)**: Executes the highest-priority process in the ready queue until completion.
    - **Shortest Remaining Time Next (SRTN)**: Preempts processes to run the one with the shortest remaining time.
    - **Round Robin (RR)**: Allocates a fixed time quantum to each process in a cyclic order.
- **Memory Management**: Uses a buddy memory allocation system to allocate memory (up to 256 bytes per process) from a 1024-byte pool and free it upon process termination.
- **Process States**: Tracks Running, Ready, and Blocked states via PCBs.
- **Performance Metrics**:
    - CPU utilization.
    - Average weighted turnaround time.
    - Average waiting time.
    - Standard deviation of weighted turnaround time.
- **Output Files**:
    - `scheduler.log`: Logs process execution details.
    - `scheduler.perf`: Summarizes performance metrics.
    - `memory.log`: Records memory allocation and deallocation events.

## Repository Structure

```
os-scheduler-showcase/
├── process_generator.c  # Reads input and initiates scheduler/clock
├── scheduler.c         # Core scheduler with scheduling and memory management
├── clk.c               # Clock module (emulates time)
├── process.c           # Simulates CPU-bound processes
├── Makefile            # Compilation instructions
├── processes.txt       # Sample input file with process data
├── scheduler.log       # Output: Process execution log
├── scheduler.perf      # Output: Performance metrics
├── memory.log          # Output: Memory allocation log
├── README.md           # This file
└── LICENSE             # MIT License
```

## Prerequisites

- **Operating System**: Linux (e.g., Ubuntu)
- **Compiler**: GCC
- **Tools**: Make, any C IDE (e.g., Eclipse, Code::Blocks) or text editor
- **Dependencies**: Standard C libraries (no external dependencies)

## How to Run the Project

### 1. Clone the Repository
```bash
git clone git@github.com:your-username/os-scheduler-showcase.git
cd os-scheduler-showcase
```

### 2. Compile the Code
Use the provided Makefile to compile all components:
```bash
make
```
This generates the executables: `process_generator`, `scheduler`, `clk`, and `process`.

### 3. Prepare the Input File
Create or modify an `processes.txt` file with process data in the following format:
```
#id    arrival    runtime    priority    memsize
1      1          6          5           200
2      3          3          3           170
```
- **Fields**: Tab-separated (`\t`).
- **Comments**: Lines starting with `#` are ignored.
- **Constraints**:
    - `id`: Unique process identifier.
    - `arrival`: Arrival time (integer seconds).
    - `runtime`: Execution time (integer seconds).
    - `priority`: 0 (highest) to 10 (lowest).
    - `memsize`: Memory required (≤256 bytes).

### 4. Run the Process Generator
Execute the process generator, which prompts for the scheduling algorithm and parameters (you can run any of the following commands):
```bash
 ./bin/os-sim -s rr -f processes.txt -q 2
 ./bin/os-sim -s hpf -f processes.txt
 ./bin/os-sim -s srtn -f processes.txt
```
- **Prompts**:
    - Choose algorithm:
        - `1`: Non-preemptive HPF
        - `2`: SRTN
        - `3`: Round Robin (requires a time quantum, e.g., 2 seconds)
    - Enter input file path (e.g., `processes.txt`).
- The program initializes the clock, scheduler, and processes, sending data to the scheduler at appropriate times.

### 5. View Outputs
After execution, check the generated files:
- `scheduler.log`: Details process start/stop times, states, and completion.
- `scheduler.perf`: Summarizes CPU utilization, average weighted turnaround time, average waiting time, and standard deviation.
- `memory.log`: Logs memory allocation (e.g., "At time X allocated Y bytes for process Z") and deallocation events.

### 6. Clean Up
To remove generated executables and logs:
```bash
make clean
```

## Assumptions
- Processes are sorted by arrival time in the input file.
- Multiple processes may arrive simultaneously.
- Time is measured in integer seconds (no fractions, e.g., 1.5 seconds).
- Memory size per process is constant during execution.
- Total memory is 1024 bytes; each process requires ≤256 bytes.
- Ties in scheduling (e.g., equal priority in HPF) are resolved by process ID (lower ID first).

## Implementation Details
- **Data Structures**:
    - **Process Control Block (PCB)**: Stores process state (Running, Ready, Blocked), execution time, remaining time, waiting time, priority, memory size, and allocated memory address.
    - **Queues**: Ready queue for scheduling, managed differently per algorithm (e.g., priority queue for HPF, min-heap for SRTN).
- **IPC**: Uses parent-child and two-way communication for process coordination.
- **Buddy Memory Allocation**: Splits and merges memory blocks in powers of 2 to allocate/deallocate process memory efficiently.
- **Error Handling**: Ensures the program does not crash and releases all IPC resources upon exit.

## Grading Criteria
- **Correctness & Understanding**: 50%
- **Modularity & Code Style**: 20% (clear comments, meaningful variable names)
- **Design Complexity & Data Structures**: 20%
- **Teamwork**: 10%

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgments
- Developed for CMPN303 Operating Systems, Cairo University.
- Thanks to the course instructors and TAs for guidance.

---
For issues or contributions, please open an issue or submit a pull request on this repository.
