/**
 * File Manager Pro - Шаблон проекта на C
 * 
 * Этот файл является шаблоном для создания проектов на языке C.
 * File Manager Pro позволяет удобно работать с проектами C/C++/C#.
 */

#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

// Конфигурация проекта
#define PROJECT_NAME "C Project Template"
#define VERSION "1.0.0"

/**
 * Главная функция программы
 */
int main(int argc, char *argv[]) {
    printf("=== %s ===\n", PROJECT_NAME);
    printf("Версия: %s\n", VERSION);
    printf("Создано с помощью File Manager Pro\n\n");
    
    // Проверка аргументов командной строки
    if (argc > 1) {
        printf("Аргументы командной строки:\n");
        for (int i = 1; i < argc; i++) {
            printf("  [%d] %s\n", i, argv[i]);
        }
        printf("\n");
    }
    
    // Использование утилит
    printf("Демонстрация утилит:\n");
    int result = add_numbers(10, 20);
    printf("  add_numbers(10, 20) = %d\n", result);
    
    print_message("Hello from File Manager Pro!");
    
    // Работа с файлами
    printf("\nРабота с файлами:\n");
    const char* filename = "output.txt";
    FILE* file = fopen(filename, "w");
    
    if (file != NULL) {
        fprintf(file, "Проект: %s\n", PROJECT_NAME);
        fprintf(file, "Версия: %s\n", VERSION);
        fprintf(file, "Создан: %s\n", __DATE__);
        fclose(file);
        printf("  Файл '%s' успешно создан\n", filename);
    } else {
        printf("  Ошибка при создании файла\n");
    }
    
    printf("\nПроект готов к использованию!\n");
    printf("Используйте File Manager Pro для навигации и редактирования.\n");
    
    return EXIT_SUCCESS;
}