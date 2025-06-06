#include "memory_manager.h"

void initialize_memory_Segment(MemoryBlock **Memory_Segment, const int TOTAL_MEMORY_SIZE) {
    *Memory_Segment = (MemoryBlock *) malloc(sizeof(MemoryBlock));
    if (*Memory_Segment == NULL) {
        printf("Memory segment allocation failed\n");
        return;
    }
    (*Memory_Segment)->size = TOTAL_MEMORY_SIZE;
    (*Memory_Segment)->parent = NULL;
    (*Memory_Segment)->left_child = NULL;
    (*Memory_Segment)->right_child = NULL;
    (*Memory_Segment)->process_pid = -1;
    (*Memory_Segment)->id_from_file = -1;
    (*Memory_Segment)->allocated = 0;
    log_message(LOG_INFO, "Memory segment successfully");
}

MemoryBlock *initialize_memory_Block(const int size) {
    MemoryBlock *block = (MemoryBlock *) malloc(sizeof(MemoryBlock));
    if (block == NULL) {
        printf("Memory block allocation failed\n");
        return NULL;
    }
    block->size = size;
    block->left_child = NULL;
    block->right_child = NULL;
    block->parent = NULL;
    block->process_pid = -1;
    block->id_from_file = -1;
    block->allocated = 0;
    return block;
}

int get_used_space(const MemoryBlock *root) {
    int remaining_space = 0;

    if (root == NULL) return remaining_space;

    const MemoryBlock *left_child = root->left_child;
    while (left_child != NULL) {
        if (left_child->process_pid != -1)
            remaining_space += left_child->allocated;
        left_child = left_child->left_child;
    }

    const MemoryBlock *right_child = root->right_child;
    while (right_child != NULL) {
        if (right_child->process_pid != -1)
            remaining_space += right_child->allocated;
        right_child = right_child->right_child;
    }

    return remaining_space;
}

int highestPowerOf2(const int x) {
    if (x <= 0) return 1;
    int power = 1;
    while (power < x) power *= 2;
    return power;
}

MemoryBlock *traverse_MemorySegment(MemoryBlock *root, const int needed_memory) {
    if (root == NULL || root->size < needed_memory || root->process_pid != -1)
        return NULL;

    if (root->size == needed_memory && root->process_pid == -1 && (
            root->size - get_used_space(root) >= needed_memory)) {
        return root;
    }

    // Recursively try left child
    if (root->left_child == NULL) {
        root->left_child = initialize_memory_Block(root->size / 2);
        root->left_child->parent = root;
    }
    MemoryBlock *left_result = traverse_MemorySegment(root->left_child, needed_memory);
    if (left_result != NULL) return left_result;

    // Recursively try right child
    if (root->right_child == NULL) {
        root->right_child = initialize_memory_Block(root->size / 2);
        root->right_child->parent = root;
    }
    return traverse_MemorySegment(root->right_child, needed_memory);
}

void get_block_address(MemoryBlock *root, MemoryBlock *block, int *start, int *end) {
    if (root == NULL || block == NULL) {
        *start = -1;
        *end = -1;
        return;
    }
    
    if (root == block) {
        *start = 0;
        *end = root->size - 1;
        return;
    }
    
    int base = 0;
    int size = root->size;
    MemoryBlock *current = root;
    
    while (current != NULL && current != block) {
        size = size / 2;
        
        if (is_in_subtree(current->left_child, block)) {
            current = current->left_child;
        }
        else if (is_in_subtree(current->right_child, block)) {
            current = current->right_child;
            base = base + size; 
        }
        else {
            *start = -1;
            *end = -1;
            return;
        }
    }
    
    *start = base;
    *end = base + block->size - 1;
}

bool is_in_subtree(MemoryBlock *root, MemoryBlock *node) {
    if (root == NULL) return false;
    if (root == node) return true;
    return is_in_subtree(root->left_child, node) || is_in_subtree(root->right_child, node);
}

bool allocate_memory(MemoryBlock *root, const int id_from_file, const int process_size , int time){
    const int memory_needed = highestPowerOf2(process_size);

    if (root == NULL) {
        initialize_memory_Segment(&root, 1024);
    }

    MemoryBlock *new_block = traverse_MemorySegment(root, memory_needed);
    if (new_block != NULL) {
        new_block->id_from_file = id_from_file;
        new_block->allocated = process_size;
        
        // Calculate block address
        int start = -1, end = -1;
        get_block_address(root, new_block, &start, &end);
        
        log_message(
            LOG_INFO, "Memory segment allocated successfully for processID=%d MEMORY allocated=%d memory needed=%d from=%d to=%d\n",
            id_from_file, new_block->allocated, new_block->size, start, end);
        
        // Log the memory allocation event
        log_memory_event(time, 1, process_size, id_from_file, start, end);
        return true;
    }
    // log_message(LOG_INFO, "Memory segment cant allocate memory for processID=%d needed=%d\n", id_from_file,
    //             process_size);
    return false;
}

MemoryBlock *get_Process_Memory_Segment(MemoryBlock *root, const int id_from_file, const int pid) {
    if (root == NULL)
        return NULL;

    if ((id_from_file != -1 && root->id_from_file == id_from_file) || (pid != -1 && root->process_pid == pid))
        return root;

    MemoryBlock *left_result = get_Process_Memory_Segment(root->left_child, id_from_file, pid);
    if (left_result) return left_result;

    return get_Process_Memory_Segment(root->right_child, id_from_file, pid);
}

void update_id(const int pid_from_file, const pid_t pid, MemoryBlock *root) {
    MemoryBlock *found = get_Process_Memory_Segment(root, pid_from_file, -1);

    if (found) {
        found->process_pid = pid;
    }
}

void merge_buddy_blocks(MemoryBlock *block) {
    if (block == NULL || block->parent == NULL)
        return;

    MemoryBlock *parent = block->parent;
    MemoryBlock *buddy = (parent->left_child == block) ? parent->right_child : parent->left_child;

    // If buddy exists and is not allocated, we can merge
    if (buddy != NULL && buddy->allocated == 0 &&
        buddy->left_child == NULL && buddy->right_child == NULL) {
        // Free the children
        free(parent->left_child);
        free(parent->right_child);

        // Reset children pointers to NULL
        parent->left_child = NULL;
        parent->right_child = NULL;

        // Recursively try to merge the parent with its buddy
        merge_buddy_blocks(parent);
    }
}

bool deallocate_memory(MemoryBlock *root, const pid_t pid, int time) {
    MemoryBlock *to_delete = get_Process_Memory_Segment(root, -1, pid);
    
    if (to_delete == NULL)
        return false;
    
    int start = -1, end = -1;
    get_block_address(root, to_delete, &start, &end);
    
    // Log deallocated memory before actually deallocating it
    log_memory_event(time, 0, to_delete->allocated, to_delete->id_from_file, start, end);
    log_message(LOG_INFO, "Memory segment deallocated for process ID=%d, memory freed=%d bytes, address range=[%d-%d]",
               to_delete->id_from_file, to_delete->allocated, start, end);
    
    to_delete->process_pid = -1;
    to_delete->id_from_file = -1;
    to_delete->allocated = 0;
    merge_buddy_blocks(to_delete);
    
    return true;
}

void log_memory_event(int time, bool allocate, int bytes, int process_id, int start, int end) {
    FILE *mem_file = fopen("memory.log", "a");
    if (!mem_file) {
        perror("Failed to open memory.log");
        return;
    }
    if (allocate) {
        fprintf(mem_file, "At time %d allocated %d bytes for process %d from %d to %d\n",
                time, bytes, process_id, start, end);
    } else {
        fprintf(mem_file, "At time %d freed %d bytes from process %d from %d to %d\n",
                time, bytes, process_id, start, end);
    }
    fclose(mem_file);
}

void destroy_memory_segment(MemoryBlock *root) {
    if (root == NULL) {
        return;
    }

    destroy_memory_segment(root->left_child);
    destroy_memory_segment(root->right_child);

    if (root->allocated > 0) {
        printf("Freeing block with PID %d, size %d\n", root->process_pid, root->size);
    }

    root->left_child = NULL;
    root->right_child = NULL;
    root->parent = NULL;
    root->process_pid = -1;
    root->size = 0;
    root->allocated = 0;
    root->id_from_file = -1;

    free(root);
    root = NULL;
}

void print_memory_segment(const MemoryBlock *root, const int level) {
    if (root == NULL) {
        return;
    }

    for (int i = 0; i < level; i++) {
        printf("  ");
    }

    printf("Block [Size: %d] ", root->size);

    if (root->allocated > 0) {
        printf("[Allocated: %d, PID: %d, ID: %d]",
               root->allocated, root->process_pid, root->id_from_file);
    } else {
        printf("[Free]");
    }

    printf("\n");

    print_memory_segment(root->left_child, level + 1);
    print_memory_segment(root->right_child, level + 1);
}

void print_memory(const MemoryBlock *root) {
    if (root == NULL) {
        printf("Memory tree is empty.\n");
        return;
    }

    printf("\n===== MEMORY ALLOCATION TREE =====\n");
    print_memory_segment(root, 0);
    printf("=================================\n\n");
}

