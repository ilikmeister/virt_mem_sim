#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int process_id;
    int page_num;
    int last_accessed;
} memory;

#define VIRTUAL_MEMORY_SIZE 32
memory virtual_memory[VIRTUAL_MEMORY_SIZE];
memory *RAM[16];

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s in.txt out.txt\n", argv[0]);
        return 1; 
    }

    char *in_file_name = argv[1];
    char *out_file_name = argv[2];

    FILE *in_file = fopen(in_file_name, "r");
    if (in_file == NULL) {
        fprintf(stderr, "Wrong File\n");
        return 1;
    }

    FILE *out_file = fopen(out_file_name, "w");
    if (out_file == NULL) {
        fprintf(stderr, "Wrong File\n");
        fclose(in_file);
        return 1;
    }

    // запуск оперативы
    for (int i = 0; i < 16; i++) {
        RAM[i] = NULL;
    }

    void page_tables() { // таблица страниц процессов здесь запускается
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            page_table[i][j] = 99;
        }
    }
}
    // блок с запуском вируталки 
    int index = 0; 
    for (int pid = 0; pid < 4; pid++) { 
        for (int page_num = 0; page_num < 4; page_num++) { 
            for (int i = 0; i < 2; i++) { 
                virtual_memory[index].process_id = pid;
                virtual_memory[index].page_num = page_num;
                virtual_memory[index].last_accessed = 0; 
                index++;
            }
        }
    }

    // пока заглушка 
    fclose(in_file);
    fclose(out_file);

    return 0;
}
