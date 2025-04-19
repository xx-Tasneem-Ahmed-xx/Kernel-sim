# Makefile for the OS Simulation Project

# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Directories
SRC_DIR = .
DS_DIR = DS
OBJ_DIR = obj
BIN_DIR = .

# Header files directories
INCLUDES = -I. -I$(DS_DIR)

# Object files
CLK_OBJ = $(OBJ_DIR)/clk.o
PROCESS_OBJ = $(OBJ_DIR)/process.o $(CLK_OBJ)
PROCESS_GEN_OBJ = $(OBJ_DIR)/process_generator.o $(CLK_OBJ) $(OBJ_DIR)/priorityQueue.o $(OBJ_DIR)/Queue.o
SCHEDULER_OBJ = $(OBJ_DIR)/scheduler.o $(CLK_OBJ) $(OBJ_DIR)/minHeap.o

# Executables (removed .out extension)
PROCESS = $(BIN_DIR)/process
PROCESS_GEN = $(BIN_DIR)/process_generator
SCHEDULER = $(BIN_DIR)/scheduler

# Default target
all: clean setup $(PROCESS) $(PROCESS_GEN) $(SCHEDULER)
	@echo "Build complete! Run with './process_generator'"

# Setup directory structure
setup:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)
	@touch keyfile

# Build rules for individual components
$(PROCESS): $(PROCESS_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(PROCESS_GEN): $(PROCESS_GEN_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(SCHEDULER): $(SCHEDULER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/%.o: $(DS_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test commands
test_hpf: all
	./process_generator 1

test_srtn: all
	./process_generator 2

test_rr: all
	./process_generator 3

# Clean up
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(PROCESS) $(PROCESS_GEN) $(SCHEDULER)
	rm -f *.out

.PHONY: all clean setup test_hpf test_srtn test_rr
