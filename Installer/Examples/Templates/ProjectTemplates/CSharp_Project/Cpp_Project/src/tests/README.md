# Тестирование в C++ проектах

Этот каталог содержит примеры модульных тестов для демонстрации работы с File Manager Pro.

## Структура тестов

tests/
├── test_filemanager.cpp # Тесты для FileManager утилит
├── test_projectutils.cpp # Тесты для ProjectUtils утилит
├── test_main.cpp # Главный файл запуска тестов
└── README.md # Эта документация

text

## Запуск тестов

### Компиляция тестового приложения

```bash
cd D:\c\FileManagerPro\Installer_CSharp\Examples\Templates\ProjectTemplates\CSharp_Project\Cpp_Project
mkdir build
cd build
cmake ..
make
Запуск тестов:
bash
./tests/run_tests
Что тестируется
FileManager утилиты:
✅ Валидация имен файлов

✅ Определение расширений

✅ Классификация типов файлов

✅ Форматирование размеров файлов

✅ Работа с путями

✅ Определение C++ файлов

ProjectUtils утилиты:
✅ Конвертеры типов

✅ Разбор и сборка путей

✅ Валидация расширений

✅ Анализ проектов

✅ Форматирование кода

Использование с File Manager Pro
1. Навигация по тестам:
Откройте файлы тестов двойным кликом

Используйте дерево папок для быстрого доступа

Переключайтесь между тестами через вкладки

2. Поиск и фильтрация:
Ctrl+F: Поиск в текущем файле

Ctrl+Shift+F: Поиск по всем тестам

Фильтры: *.cpp для файлов с тестами

3. Запуск тестов:
Настройте внешние инструменты для сборки

Используйте терминал File Manager Pro

Интегрируйте с CMake для автоматизации

4. Анализ результатов:
Просматривайте вывод тестов в терминале

Анализируйте покрытие кода

Отлаживайте неудачные тесты

Пример теста
cpp
void testIsValidFilename() {
    // Корректные имена
    assert(FileManager::isValidFilename("file.txt") == true);
    
    // Некорректные имена
    assert(FileManager::isValidFilename("file<bad>.txt") == false);
}
Преимущества File Manager Pro для тестирования
Быстрая навигация: Между тестами и исходным кодом

Интеграция с CMake: Автоматическая сборка тестов

Терминал: Запуск тестов без переключения окон

Подсветка синтаксиса: Для кода тестов и результатов

Управление проектом: Все файлы в одном месте

Рекомендации
Создавайте тесты для каждого модуля

Используйте понятные имена тестов

Документируйте ожидаемое поведение

Запускайте тесты перед коммитом

Используйте File Manager Pro для управления тестами

Лицензия
Примеры тестов распространяются под лицензией MIT.
Используйте как шаблон для своих проектов.

text

## **CMakeLists.txt для тестов** (дополнение к основному)
```cmake
# В основной CMakeLists.txt добавьте:

# Тестовое приложение
add_executable(run_tests
    src/tests/test_main.cpp
    src/tests/test_filemanager.cpp
    src/tests/test_projectutils.cpp
    src/FileManager.cpp
    src/ProjectUtils.cpp
)

target_include_directories(run_tests 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tests
)

# Добавьте цель для запуска тестов
add_custom_target(test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/run_tests
    DEPENDS run_tests
    COMMENT "Запуск тестов File Manager Pro"
)
Эта структура тестов демонстрирует:

Модульное тестирование C++ кода

Интеграцию с CMake для автоматизации

Лучшие практики организации тестов

Возможности File Manager Pro для работы с тестами

Готовые примеры для изучения и использования
