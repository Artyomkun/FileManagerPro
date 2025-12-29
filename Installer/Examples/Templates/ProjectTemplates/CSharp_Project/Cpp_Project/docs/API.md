# Документация API - C++ Проект для File Manager Pro

## Обзор

Этот проект демонстрирует структуру современного C++ проекта с использованием
File Manager Pro для разработки. Проект включает утилиты для работы с файлами,
навигацией и анализом проектов.

## Пространства имен

### `FileManager`

Утилиты для работы с файловой системой.

### `ProjectUtils`

Утилиты для анализа и управления проектами.

## Классы

### `FileManager::FileType`

Перечисление типов файлов.

```cpp
    enum class FileType {
        UNKNOWN,
        SOURCE_CODE,
        SCRIPT,
        DOCUMENT,
        DATA,
        IMAGE,
        ARCHIVE,
        EXECUTABLE
    };
