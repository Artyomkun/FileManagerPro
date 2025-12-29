#include "utils.h"
#include <stdio.h>
#include <time.h>

int add_numbers(int a, int b) {
    return a + b;
}

void print_message(const char* message) {
    printf("  Сообщение: %s\n", message);
}

int create_test_file(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        return -1;
    }
    
    time_t now = time(NULL);
    fprintf(file, "Тестовый файл\n");
    fprintf(file, "Создан: %s", ctime(&now));
    fprintf(file, "Используйте File Manager Pro для просмотра\n");
    
    fclose(file);
    return 0;
}