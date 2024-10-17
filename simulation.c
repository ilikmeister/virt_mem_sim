#include <stdio.h>
#include <stdlib.h>

#define VIRTUAL_MEMORY_SIZE 32
#define RAM_SIZE 16
#define NUM_PROCESSES 4
#define PAGES_PER_PROCESS 4
#define FRAME_COUNT 8
#define PAGE_SIZE 2

// Структура для представления памяти
typedef struct {
    int process_id;
    int page_num;
    int last_accessed;
} memory;

memory virtual_memory[VIRTUAL_MEMORY_SIZE];    // Виртуальная память
memory *RAM[RAM_SIZE];                         // ОЗУ (массив указателей)
int page_table[NUM_PROCESSES][PAGES_PER_PROCESS];  // Таблицы страниц
int time_step = 0;                             // Шаг времени

// Прототипы функций
void initialize_virtual_memory();
void initialize_page_tables();
void load_page_to_ram(int process_id, int page_num);
int find_empty_frame();
void evict_page(int process_id);
int find_lru_global();
int find_lru_local(int process_id);
void print_page_tables(FILE *out_file);
void print_ram(FILE *out_file);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s in.txt out.txt\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Открытие входного файла
    FILE *in_file = fopen(argv[1], "r");
    if (in_file == NULL) {
        fprintf(stderr, "Ошибка при открытии входного файла\n");
        exit(EXIT_FAILURE);
    }

    // Открытие выходного файла
    FILE *out_file = fopen(argv[2], "w");
    if (out_file == NULL) {
        fprintf(stderr, "Ошибка при открытии выходного файла\n");
        fclose(in_file);
        exit(EXIT_FAILURE);
    }

    // Инициализация виртуальной памяти и таблиц страниц
    initialize_virtual_memory();
    initialize_page_tables();

    // Инициализация ОЗУ
    for (int i = 0; i < RAM_SIZE; i++) {
        RAM[i] = NULL;
    }

    // Чтение process_id из входного файла и обработка страниц
    int process_id;
    while (fscanf(in_file, "%d", &process_id) != EOF) {
        if (process_id < 0 || process_id >= NUM_PROCESSES) {
            fprintf(stderr, "Неверный process_id: %d\n", process_id);
            continue;
        }

        // Поиск следующей страницы процесса для загрузки
        int page_num = -1;
        for (int i = 0; i < PAGES_PER_PROCESS; i++) {
            if (page_table[process_id][i] == 99) {
                page_num = i;
                break;
            }
        }

        if (page_num == -1) {
            // Все страницы процесса уже в ОЗУ
            continue;
        }

        // Загрузка страницы в ОЗУ
        load_page_to_ram(process_id, page_num);

        // Увеличиваем шаг времени
        time_step++;
    }

    // Вывод таблиц страниц и содержимого ОЗУ в выходной файл
    print_page_tables(out_file);
    print_ram(out_file);

    // Закрытие файлов
    fclose(in_file);
    fclose(out_file);

    return 0;
}

// Функция инициализации виртуальной памяти
void initialize_virtual_memory() {
    int index = 0;
    for (int pid = 0; pid < NUM_PROCESSES; pid++) {
        for (int page_num = 0; page_num < PAGES_PER_PROCESS; page_num++) {
            for (int i = 0; i < PAGE_SIZE; i++) {
                virtual_memory[index].process_id = pid;
                virtual_memory[index].page_num = page_num;
                virtual_memory[index].last_accessed = 0; // Инициализируем 0
                index++;
            }
        }
    }
}

// Функция инициализации таблиц страниц
void initialize_page_tables() {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        for (int j = 0; j < PAGES_PER_PROCESS; j++) {
            page_table[i][j] = 99; // 99 означает, что страница на диске
        }
    }
}

// Функция загрузки страницы в ОЗУ
void load_page_to_ram(int process_id, int page_num) {
    int frame_num = find_empty_frame();
    if (frame_num == -1) {
        // Нет свободного фрейма, нужно вытеснить страницу
        evict_page(process_id);
        frame_num = find_empty_frame();
        if (frame_num == -1) {
            fprintf(stderr, "Ошибка: Нет свободного фрейма после вытеснения\n
