//  CITS2002 Project 2 2024
//  Student1:   24038357   ILIYAS AKHMET
//  Student2:   24069389   DMITRY PRYTKOV
//  Platform:   Linux

#include <stdio.h>
#include <stdlib.h>

#define VIRTUAL_MEM_SIZE 32
#define RAM_SIZE 16
#define PROCESS_NUM 4
#define PAGES_PER_PROCESS 4
#define FRAME_COUNT 8
#define PAGE_SIZE 2
#define PAGE_NOT_IN_RAM 99

// Initializing the structure
typedef struct {
    int process_id;
    int page_num;
    int last_accessed;
} memory;

memory virtual_memory[VIRTUAL_MEM_SIZE];
memory *RAM[RAM_SIZE];
int page_table[PROCESS_NUM][PAGES_PER_PROCESS];
int time_step = 0;
int current_page[PROCESS_NUM];

// Function prototypes
void initialize_virtual_memory();
void initialize_page_tables();
void load_page_to_ram(int process_id, int page_num);
int find_empty_frame();
void evict_page(int process_id);
int find_lru_global();
int find_lru_local(int process_id);
void print_page_tables(FILE *out_txt);
void print_ram(FILE *out_txt);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s in.txt out.txt\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Opening the input file
    FILE *in_txt = fopen(argv[1], "r");
    if (in_txt == NULL) {
        fprintf(stderr, "Could not open in.txt\n");
        exit(EXIT_FAILURE);
    }

    // Opening the output file
    FILE *out_txt = fopen(argv[2], "w");
    if (out_txt == NULL) {
        fprintf(stderr, "Could not open the out.txt\n");
        fclose(in_txt);
        exit(EXIT_FAILURE);
    }

    // Initializing virtual memory and page tables
    initialize_virtual_memory();
    initialize_page_tables();

    // Initializing RAM and current page of processes
    for (int i = 0; i < RAM_SIZE; i++) {
        RAM[i] = NULL;
    }
    for (int i = 0; i < PROCESS_NUM; i++) {
        current_page[i] = 0;
    }

    // Reading process ids from in.txt and processing pages
    int process_id;
    while (fscanf(in_txt, "%d", &process_id) != EOF) {
        if (process_id < 0 || process_id >= PROCESS_NUM) {
            fprintf(stderr, "Inccorect process id: %d\n", process_id);
            continue;
        }

        int page_num = current_page[process_id];
        if (page_num >= PAGES_PER_PROCESS) {
            // All pages are processed
            time_step++;
            continue;
        }

        if (page_table[process_id][page_num] != PAGE_NOT_IN_RAM) {
            // If page is already in RAM
            int frame_num = page_table[process_id][page_num];
            for (int i = 0; i < PAGE_SIZE; i++) {
                RAM[frame_num * PAGE_SIZE + i]->last_accessed = time_step;
            }
        } else {
            // If page is not in RAM
            load_page_to_ram(process_id, page_num);
        }

        // Moving to the next page of the process
        current_page[process_id]++;

        // Incrementing the time step
        time_step++;
    }

    // Printing the page tables and RAM
    print_page_tables(out_txt);
    print_ram(out_txt);

    // Closing the files
    fclose(in_txt);
    fclose(out_txt);

    return 0;
}

// Function to initialize virtual memory
void initialize_virtual_memory() {
    int index = 0;
    for (int pid = 0; pid < PROCESS_NUM; pid++) {
        for (int page_num = 0; page_num < PAGES_PER_PROCESS; page_num++) {
            for (int i = 0; i < PAGE_SIZE; i++) {
                virtual_memory[index].process_id = pid;
                virtual_memory[index].page_num = page_num;
                virtual_memory[index].last_accessed = -1; // Initialize to -1
                index++;
            }
        }
    }
}

// Function to initialize page table
void initialize_page_tables() {
    for (int i = 0; i < PROCESS_NUM; i++) {
        for (int j = 0; j < PAGES_PER_PROCESS; j++) {
            page_table[i][j] = PAGE_NOT_IN_RAM;
        }
    }
}

// Function to load page to RAM
void load_page_to_ram(int process_id, int page_num) {
    int frame_num = find_empty_frame();
    if (frame_num == -1) {
        // No free frame, need to displace page
        evict_page(process_id);
        frame_num = find_empty_frame();
        if (frame_num == -1) {
            fprintf(stderr, "No free frame\n");
            exit(EXIT_FAILURE);
        }
    }

    // Load the page into the found frame
    for (int i = 0; i < PAGE_SIZE; i++) {
        RAM[frame_num * PAGE_SIZE + i] =
            &virtual_memory[(process_id * PAGES_PER_PROCESS + page_num)
                            * PAGE_SIZE + i];
    }

    // Update page table
    page_table[process_id][page_num] = frame_num;

    // Update last_accessed
    for (int i = 0; i < PAGE_SIZE; i++) {
        RAM[frame_num * PAGE_SIZE + i]->last_accessed = time_step;
    }
}

// Function to search for an empty frame in RAM
int find_empty_frame() {
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (RAM[i * PAGE_SIZE] == NULL) {
            return i; // Return the empty frame number
        }
    }
    return -1; // No empty frames
}

// Function of page evict from RAM
void evict_page(int process_id) {
    int lru_frame = find_lru_local(process_id);
    if (lru_frame == -1) {
	// No process pages in RAM, use global LRU
        lru_frame = find_lru_global();
    }

    if (lru_frame == -1) {
        fprintf(stderr, "No pages to evict\n");
        exit(EXIT_FAILURE);
    }

 // Information about the page being evicted
    int evicted_pid = RAM[lru_frame * PAGE_SIZE]->process_id;
    int evicted_page = RAM[lru_frame * PAGE_SIZE]->page_num;

    // Update page table
    page_table[evicted_pid][evicted_page] = PAGE_NOT_IN_RAM;

    // Clear frame in RAM
    for (int i = 0; i < PAGE_SIZE; i++) {
        RAM[lru_frame * PAGE_SIZE + i] = NULL;
    }
}

// Function to find the least recently used frame (local)
int find_lru_local(int process_id) {
    int lru_frame = -1;
    int oldest_time = time_step;

    for (int i = 0; i < FRAME_COUNT; i++) {
        int index = i * PAGE_SIZE;
        if (RAM[index] != NULL && RAM[index]->process_id == process_id) {
            if (RAM[index]->last_accessed < oldest_time) {
                oldest_time = RAM[index]->last_accessed;
                lru_frame = i;
            }
        }
    }
    return lru_frame;
}

// Function to find the least recently used frame (global)
int find_lru_global() {
    int lru_frame = -1;
    int oldest_time = time_step;

    for (int i = 0; i < FRAME_COUNT; i++) {
        int index = i * PAGE_SIZE;
        if (RAM[index] != NULL) {
            if (RAM[index]->last_accessed < oldest_time) {
                oldest_time = RAM[index]->last_accessed;
                lru_frame = i;
            }
        }
    }
    return lru_frame;
}

// Function for outputting page tables to a file
void print_page_tables(FILE *out_txt) {
    for (int pid = 0; pid < PROCESS_NUM; pid++) {
        for (int i = 0; i < PAGES_PER_PROCESS; i++) {
            fprintf(out_txt, "%d", page_table[pid][i]);
            if (i < PAGES_PER_PROCESS - 1) {
                fprintf(out_txt, ", ");
            }
        }
        fprintf(out_txt, "\n");
    }
}

// Function for outputting RAM contents to a file
void print_ram(FILE *out_txt) {
    for (int i = 0; i < FRAME_COUNT; i++) {
        int index = i * PAGE_SIZE;
        if (RAM[index] != NULL) {
            fprintf(out_txt, "%d,%d,%d; %d,%d,%d",
                    RAM[index]->process_id,
                    RAM[index]->page_num,
                    RAM[index]->last_accessed,
                    RAM[index + 1]->process_id,
                    RAM[index + 1]->page_num,
                    RAM[index + 1]->last_accessed);
        } else {
            fprintf(out_txt, "empty; empty");
        }
        if (i < FRAME_COUNT - 1) {
            fprintf(out_txt, "; ");
        }
    }
    fprintf(out_txt, ";\n");
}
