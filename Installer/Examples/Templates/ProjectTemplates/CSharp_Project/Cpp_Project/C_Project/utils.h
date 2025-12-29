/**
 * Утилиты для проекта на C
 * 
 * Этот файл содержит вспомогательные функции
 * для демонстрации работы File Manager Pro с проектами C.
 */

#ifndef UTILS_H
#define UTILS_H

/**
 * Складывает два числа
 * @param a Первое число
 * @param b Второе число
 * @return Сумма a и b
 */
int add_numbers(int a, int b);

/**
 * Выводит сообщение на экран
 * @param message Сообщение для вывода
 */
void print_message(const char* message);

/**
 * Создает тестовый файл
 * @param filename Имя файла
 * @return 0 при успехе, -1 при ошибке
 */
int create_test_file(const char* filename);

#endif // UTILS_H