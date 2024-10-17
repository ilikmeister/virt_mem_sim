#include <stdio.h>
#include <stdlib.h>

#define VIRTUAL_MEMORY_SIZE 32
#define RAM_SIZE 16
#define NUM_PROCESSES 4
#define PAGES_PER_PROCESS 4
#define FRAME_COUNT 8
#define PAGE_SIZE 2

// Initializing structure
typedef struct {
    int process_id;
    int page_num;
    int last_accessed;
} memory;

memory virtual_memory[VIRTUAL_MEMORY_SIZE];   // Fixed virtual memory
memory *RAM[RAM_SIZE];                        // Pointers for dynamic RAM
int page_table[NUM_PROCESSES][PAGES_PER_PROCESS];  // Page tables for processes
int time_step = 0;

// Declaring functions to call in main
void initialize_virtual_memory();
void initialize_page_tables();
void load_page_to_ram(int process_id, int page_num);
int find_empty_frame();
void evict_page(int process_id, int page_num);
int find_lru_global();
int find_lru_local(int process_id);
void print_page_tables(FILE *out_file);
void print_ram(FILE *out_file);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s in.txt out.txt\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Opening in.txt
    FILE *in_file = fopen(argv[1], "r");
    if (in_file == NULL) {
        fprintf(stderr, "Error opening input file\n");
        exit(EXIT_FAILURE);
    }
    // Opening out.txt
    FILE *out_file = fopen(argv[2], "w");
    if (out_file == NULL) {
        fprintf(stderr, "Error opening output file\n");
        fclose(in_file);
        exit(EXIT_FAILURE);
    }

    // Initializing virtual memory and page tables
    initialize_virtual_memory();
    initialize_page_tables();

    // Initializing RAM
    for (int i = 0; i < RAM_SIZE; i++) {
        RAM[i] = NULL;
    }

    // Reading process ids from in.txt and processing pages
    int process_id;
    while (fscanf(in_file, "%d", &process_id) != EOF) {
        int page_num = -1;

        // Finding the next page of the process to load
        for (int i = 0; i < PAGES_PER_PROCESS; i++) {
            if (page_table[process_id][i] == 99) {  // If page is in virtual memory
                page_num = i;
                break;
            }
        }

        if (page_num == -1) {
            continue;  // Moving to the next process
        }

        // Loading the page to RAM
        load_page_to_ram(process_id, page_num);
        time_step++; // Incrementing time
    }

    // Printing the page tables and RAM
    print_page_tables(out_file);
    print_ram(out_file);

    // Closing the files
    fclose(in_file);
    fclose(out_file);

    exit(EXIT_SUCCESS);
}

// Initializing virtual memory with process pages
void initialize_virtual_memory() {
    int index = 0;
    for (int pid = 0; pid < NUM_PROCESSES; pid++) {
        for (int page_num = 0; page_num < PAGES_PER_PROCESS; page_num++) {
            for (int i = 0; i < PAGE_SIZE; i++) {
                virtual_memory[index].process_id = pid;
                virtual_memory[index].page_num = page_num;
                virtual_memory[index].last_accessed = 0;  // Initializing to 0
                index++;
            }
        }
    }
}

// Initializing all page tables
void initialize_page_tables() {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        for (int j = 0; j < PAGES_PER_PROCESS; j++) {
            page_table[i][j] = 99;  // 99 for all pages in disc
        }
    }
}

// Loading a page into RAM
void load_page_to_ram(int process_id, int page_num) {
    int frame_num = find_empty_frame();
    if (frame_num == -1) {  // Using LRU if frame num is not found
        evict_page(process_id, page_num);  // Evicting a page
        frame_num = find_empty_frame();    // Finding an empty frame num
    }

    // Corrected index calculation for virtual memory
    int vm_index = (process_id * PAGES_PER_PROCESS + page_num) * PAGE_SIZE;

    // Loading the page into the found frame
    for (int i = 0; i < PAGE_SIZE; i++) {
        RAM[frame_num * PAGE_SIZE + i] = &virtual_memory[vm_index + i];
    }

    // Updating the page table
    page_table[process_id][page_num] = frame_num;

    // Updating the last accessed time
    for (int i = 0; i < PAGE_SIZE; i++) {
        RAM[frame_num * PAGE_SIZE + i]->last_accessed = time_step;
    }
}

// Finding an empty frame in RAM
int find_empty_frame() {
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (RAM[i * PAGE_SIZE] == NULL) {
            return i; // Returning the empty frame
        }
    }
    return -1;  // No empty frame found
}

// Evicting a page from RAM
void evict_page(int process_id, int page_num) {
    int lru_frame = find_lru_local(process_id);
    if (lru_frame == -1) {
        lru_frame = find_lru_global();  // Using global LRU if no local page
    }

    // Marking the evicted page as 99
    int evicted_pid = RAM[lru_frame * PAGE_SIZE]->process_id;
    int evicted_page = RAM[lru_frame * PAGE_SIZE]->page_num;
    page_table[evicted_pid][evicted_page] = 99;

    // Clearing the frame in RAM
    for (int i = 0; i < PAGE_SIZE; i++) {
        RAM[lru_frame * PAGE_SIZE + i] = NULL;
    }
}

// Finding the least recently used page for a specific process
int find_lru_local(int process_id) {
    int lru_frame = -1;
    int oldest_time = time_step;

    for (int i = 0; i < FRAME_COUNT; i++) {
        if (RAM[i * PAGE_SIZE] != NULL && RAM[i * PAGE_SIZE]->process_id == process_id) {
            if (RAM[i * PAGE_SIZE]->last_accessed < oldest_time) {
                oldest_time = RAM[i * PAGE_SIZE]->last_accessed;
                lru_frame = i;
            }
        }
    }
    return lru_frame;
}

// Finding the least recently used page in RAM
int find_lru_global() {
    int lru_frame = -1;
    int oldest_time = time_step;

    for (int i = 0; i < FRAME_COUNT; i++) {
        if (RAM[i * PAGE_SIZE] != NULL) {
            if (RAM[i * PAGE_SIZE]->last_accessed < oldest_time) {
                oldest_time = RAM[i * PAGE_SIZE]->last_accessed;
                lru_frame = i;
            }
        }
    }
    return lru_frame;
}

// Printing page tables to the output file
void print_page_tables(FILE *out_file) {
    for (int pid = 0; pid < NUM_PROCESSES; pid++) {
        for (int i = 0; i < PAGES_PER_PROCESS; i++) {
            fprintf(out_file, "%d", page_table[pid][i]);
            if (i < PAGES_PER_PROCESS - 1) {
                fprintf(out_file, ", ");
            }
        }
        fprintf(out_file, "\n");
    }
}

// Printing RAM to the output file
void print_ram(FILE *out_file) {
    for (int i = 0; i < RAM_SIZE; i++) {
        if (RAM[i] != NULL) {
            fprintf(out_file, "%d,%d,%d", RAM[i]->process_id, RAM[i]->page_num, RAM[i]->last_accessed);
        } else {
            fprintf(out_file, "empty");
        }
        if (i < RAM_SIZE - 1) {
            fprintf(out_file, "; ");
        }
    }
    fprintf(out_file, ";\n");
}