# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Directories
OBJ_DIR = obj
BIN_DIR = bin

# Source files
COMMON_SRCS = DS/minHeap.c DS/priorityQueue.c DS/Queue.c pcb.c DS/comparators.c clk.c
COMMON_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))

# Ensure directories exist before building
$(shell mkdir -p $(BIN_DIR) $(OBJ_DIR) $(OBJ_DIR)/DS)

# Process Generator executable
$(BIN_DIR)/os-sim: $(OBJ_DIR)/process_generator.o $(COMMON_OBJS)
    $(CC) $(CFLAGS) -o $@ $^

# Scheduler executable
$(BIN_DIR)/scheduler: $(OBJ_DIR)/scheduler.o $(COMMON_OBJS)
    $(CC) $(CFLAGS) -o $@ $^ -lm

# Process executable
$(BIN_DIR)/process.out: $(OBJ_DIR)/process.o $(OBJ_DIR)/clk.o
    $(CC) $(CFLAGS) -o $@ $^

# Pattern rule for object files
$(OBJ_DIR)/%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@

# Default rule
all: $(BIN_DIR)/os-sim $(BIN_DIR)/scheduler $(BIN_DIR)/process.out

# Run the simulation with different algorithms
run-rr: all
    ./$(BIN_DIR)/os-sim -s rr -f ../processes.txt -q 2

run-hpf: all
    ./$(BIN_DIR)/os-sim -s hpf -f ../processes.txt

run-srtn: all
    ./$(BIN_DIR)/os-sim -s srtn -f ../processes.txt

# Clean up
clean:
    rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

.PHONY: all clean run-rr run-hpf run-srtn