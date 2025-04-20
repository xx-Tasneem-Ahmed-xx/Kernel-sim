# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Source files
SRCS = process_generator.c process.c clk.c scheduler.c minHeap.c priorityQueue.c Queue.c pcb.c comparators.c

# Object files
OBJS = $(SRCS:.c=.o)

# Executable name
TARGET = process_generator

# Default rule
all: $(TARGET)

# Link the object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)
