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
int page_table[NUM_PROCESSES][PAGES_PER_PROCESS];  // Таблицы страниц процессов
int time_step = 0;                             // Шаг времени
int current_page[NUM_PROCESSES];               // Текущая страница для каждого процесса

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

    // Инициализация ОЗУ и текущих страниц процессов
    for (int i = 0; i < RAM_SIZE; i++) {
        RAM[i] = NULL;
    }
    for (int i = 0; i < NUM_PROCESSES; i++) {
        current_page[i] = 0;
    }

    // Чтение process_id из входного файла и обработка страниц
    int process_id;
    while (fscanf(in_file, "%d", &process_id) != EOF) {
        if (process_id < 0 || process_id >= NUM_PROCESSES) {
            fprintf(stderr, "Неверный process_id: %d\n", process_id);
            continue;
        }

        int page_num = current_page[process_id];
        if (page_num >= PAGES_PER_PROCESS) {
            // Все страницы процесса уже обработаны
            time_step++;
            continue;
        }

        if (page_table[process_id][page_num] != 99) {
            // Страница уже в ОЗУ, обновляем last_accessed
            int frame_num = page_table[process_id][page_num];
            for (int i = 0; i < PAGE_SIZE; i++) {
                RAM[frame_num * PAGE_SIZE + i]->last_accessed = time_step;
            }
        } else {
            // Страница не в ОЗУ, загружаем ее
            load_page_to_ram(process_id, page_num);
        }

        // Переходим к следующей странице процесса
        current_page[process_id]++;

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
                virtual_memory[index].last_accessed = -1; // Инициализируем -1
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
            fprintf(stderr, "Ошибка: Нет свободного фрейма после вытеснения\n");
            exit(EXIT_FAILURE);
        }
    }

    // Загрузка страницы в найденный фрейм
    for (int i = 0; i < PAGE_SIZE; i++) {
        RAM[frame_num * PAGE_SIZE + i] =
            &virtual_memory[(process_id * PAGES_PER_PROCESS + page_num)
                            * PAGE_SIZE + i];
    }

    // Обновление таблицы страниц
    page_table[process_id][page_num] = frame_num;

    // Обновление last_accessed
    for (int i = 0; i < PAGE_SIZE; i++) {
        RAM[frame_num * PAGE_SIZE + i]->last_accessed = time_step;
    }
}

// Функция поиска пустого фрейма в ОЗУ
int find_empty_frame() {
    for (int i = 0; i < FRAME_COUNT; i++) {
        if (RAM[i * PAGE_SIZE] == NULL) {
            return i; // Возвращаем номер пустого фрейма
        }
    }
    return -1; // Пустых фреймов нет
}

// Функция вытеснения страницы из ОЗУ
void evict_page(int process_id) {
    int lru_frame = find_lru_local(process_id);
    if (lru_frame == -1) {
        // Нет страниц процесса в ОЗУ, используем глобальный LRU
        lru_frame = find_lru_global();
    }

    if (lru_frame == -1) {
        fprintf(stderr, "Ошибка: Нет страниц для вытеснения\n");
        exit(EXIT_FAILURE);
    }

    // Информация о вытесняемой странице
    int evicted_pid = RAM[lru_frame * PAGE_SIZE]->process_id;
    int evicted_page = RAM[lru_frame * PAGE_SIZE]->page_num;

    // Обновление таблицы страниц
    page_table[evicted_pid][evicted_page] = 99;

    // Очистка фрейма в ОЗУ
    for (int i = 0; i < PAGE_SIZE; i++) {
        RAM[lru_frame * PAGE_SIZE + i] = NULL;
    }
}

// Функция поиска наименее недавно использованного фрейма (локальный LRU)
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

// Функция поиска наименее недавно использованного фрейма (глобальный LRU)
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

// Функция вывода таблиц страниц в файл
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

// Функция вывода содержимого ОЗУ в файл
void print_ram(FILE *out_file) {
    for (int i = 0; i < FRAME_COUNT; i++) {
        int index = i * PAGE_SIZE;
        if (RAM[index] != NULL) {
            fprintf(out_file, "%d,%d,%d; %d,%d,%d",
                    RAM[index]->process_id,
                    RAM[index]->page_num,
                    RAM[index]->last_accessed,
                    RAM[index + 1]->process_id,
                    RAM[index + 1]->page_num,
                    RAM[index + 1]->last_accessed);
        } else {
            fprintf(out_file, "empty; empty");
        }
        if (i < FRAME_COUNT - 1) {
            fprintf(out_file, "; ");
        }
    }
    fprintf(out_file, "\n");
}