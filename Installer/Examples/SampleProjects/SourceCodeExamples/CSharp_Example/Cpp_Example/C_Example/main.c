/* 
 * Пример программы на C для демонстрации возможностей File Manager Pro
 * Эта программа показывает, как работать с файлами и папками
 */

#include <stdio.h>
#include <dirent.h>
#include <string.h>

// Структура для хранения информации о файле
typedef struct {
    char name[256];
    long size;
    int is_directory;
} FileInfo;

// Функция для получения размера файла
long get_file_size(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) return -1;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    
    return size;
}

// Функция для вывода содержимого папки
void list_directory(const char* path) {
    DIR* dir = opendir(path);
    if (dir == NULL) {
        printf("Не удалось открыть папку: %s\n", path);
        return;
    }
    
    printf("Содержимое папки '%s':\n", path);
    printf("================================\n");
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Пропускаем текущую и родительскую папки
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Формируем полный путь
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        // Получаем информацию о файле
        FileInfo info;
        strncpy(info.name, entry->d_name, sizeof(info.name));
        
        if (entry->d_type == DT_DIR) {
            info.is_directory = 1;
            info.size = 0;
            printf("[DIR]  %s\n", info.name);
        } else {
            info.is_directory = 0;
            info.size = get_file_size(full_path);
            printf("[FILE] %s (%ld байт)\n", info.name, info.size);
        }
    }
    
    closedir(dir);
    printf("================================\n");
}

// Главная функция
int main(int argc, char* argv[]) {
    printf("=== Демонстрация File Manager Pro ===\n\n");
    
    // Показываем текущую папку
    printf("Текущая рабочая папка:\n");
    list_directory(".");
    
    // Пример работы с файлами
    printf("\nСоздание тестового файла...\n");
    FILE* test_file = fopen("test_output.txt", "w");
    if (test_file) {
        fprintf(test_file, "Это тестовый файл, созданный демонстрационной программой.\n");
        fprintf(test_file, "File Manager Pro может просматривать и редактировать такие файлы.\n");
        fprintf(test_file, "Дата создания: %s\n", __DATE__);
        fclose(test_file);
        printf("Файл 'test_output.txt' успешно создан.\n");
    }
    
    // Показываем результат
    printf("\nПосле создания файла:\n");
    list_directory(".");
    
    // Чтение созданного файла
    printf("\nСодержимое созданного файла:\n");
    printf("--------------------------------\n");
    test_file = fopen("test_output.txt", "r");
    if (test_file) {
        char line[256];
        while (fgets(line, sizeof(line), test_file)) {
            printf("%s", line);
        }
        fclose(test_file);
    }
    printf("--------------------------------\n");
    
    printf("\nДемонстрация завершена!\n");
    printf("File Manager Pro позволяет удобно работать с такими файлами.\n");
    
    return 0;
}