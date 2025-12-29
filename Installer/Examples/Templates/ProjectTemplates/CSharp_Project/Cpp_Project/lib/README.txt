# === Папка lib - Библиотеки проекта ===

Эта папка предназначена для хранения статических и динамических библиотек,
используемых в C++ проекте. File Manager Pro позволяет удобно управлять
зависимостями проекта.

## Содержимое

### 1. StringUtils.lib

Пример статической библиотеки для работы со строками.
Содержит функции:

- Разделение строк
- Объединение строк
- Преобразование регистра
- Форматирование строк

### 2. MathUtils.lib

Пример статической библиотеки для математических операций.
Содержит функции:

- Основные арифметические операции
- Статистические функции
- Геометрические расчеты
- Генерация случайных чисел

## Использование с CMake

```cmake
# В CMakeLists.txt добавьте:
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_link_libraries(${PROJECT_NAME}
    StringUtils
    MathUtils
)
```

## Использование с Makefile

```makefile
# В Makefile добавьте:
LIBS = -L./lib -lStringUtils -lMathUtils

app: main.o
    g++ -o app main.o $(LIBS)
```

## Управление библиотеками в File Manager Pro

### 1. Добавление библиотек

- Поместите .lib/.a файлы в эту папку
- Обновите конфигурацию сборки
- Пересоберите проект

### 2. Поиск библиотек

- Используйте поиск (Ctrl+F) для нахождения нужных библиотек
- Просматривайте дерево папок для навигации
- Проверяйте версии библиотек

### 3. Обновление библиотек

- Замените старые версии файлов
- Обновите ссылки в проекте
- Проверьте совместимость

## Рекомендации

1. **Организация:**
   - Группируйте связанные библиотеки
   - Используйте понятные имена
   - Храните документацию рядом

2. **Версионирование:**
   - libname_v1.0.lib
   - libname_v1.1.lib
   - libname_debug.lib / libname_release.lib

3. **Документация:**
   - README для каждой библиотеки
   - Примеры использования
   - Информация о лицензии

## Пример структуры для больших проектов

lib/
├── internal/           # Внутренние библиотеки
│   ├── core.lib
│   └── utils.lib
├── external/          # Внешние зависимости
│   ├── boost/
│   └── jsoncpp/
└── third_party/      # Сторонние библиотеки
    └── openssl/

## Создание собственных библиотек

1. **Статическая библиотека:**

    ```bash
    g++ -c StringUtils.cpp -o StringUtils.o
    ar rcs libStringUtils.a StringUtils.o
    ```

2. **Динамическая библиотека:**

    ```bash
    g++ -shared -fPIC StringUtils.cpp -o libStringUtils.so
    ```

3. **Использование в проекте:**

    ```cpp
    #include "StringUtils.hpp"

    int main() {
        std::string result = StringUtils::format("Hello, %s!", "World");
        return 0;
    }
    ```

## Интеграция с File Manager Pro

1. **Быстрая навигация:**
   - Переход между библиотеками и их использованием
   - Поиск по всем библиотекам проекта
   - Просмотр зависимостей

2. **Управление версиями:**
   - Сравнение разных версий библиотек
   - Откат к предыдущим версиям
   - Управление зависимостями

3. **Отладка:**
   - Информация о загруженных библиотеках
   - Проверка совместимости версий
   - Диагностика проблем линковки

## Безопасность

1. **Проверка источников:**
   - Загружайте библиотеки только из доверенных источников
   - Проверяйте контрольные суммы
   - Используйте подписанные библиотеки

2. **Лицензии:**
   - Проверяйте лицензии сторонних библиотек
   - Сохраняйте файлы лицензий
   - Соблюдайте условия использования

## Оптимизация

1. **Разделение сборок:**
   - Debug версии для отладки
   - Release версии для релиза
   - Оптимизированные версии

2. **Кроссплатформенность:**
   - Windows: .lib файлы
   - Linux: .a (статические) или .so (динамические)
   - macOS: .a или .dylib

============================================

File Manager Pro - Управление зависимостями

============================================

## **Пример заголовочного файла для библиотеки (lib/include/StringUtils.hpp):**

```cpp
#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <string>
#include <vector>

/**
 * Библиотека утилит для работы со строками
 * Используется в демонстрационном проекте File Manager Pro
 */
namespace StringUtils {

/**
 * Разделяет строку по разделителю
 */
std::vector<std::string> split(const std::string& str, char delimiter);

/**
 * Объединяет строки через разделитель
 */
std::string join(const std::vector<std::string>& strings,
                 const std::string& delimiter);

/**
 * Проверяет, начинается ли строка с префикса
 */
bool startsWith(const std::string& str, const std::string& prefix);

/**
 * Проверяет, заканчивается ли строка суффиксом
 */
bool endsWith(const std::string& str, const std::string& suffix);

/**
 * Преобразует строку в нижний регистр
 */
std::string toLower(const std::string& str);

/**
 * Преобразует строку в верхний регистр
 */
std::string toUpper(const std::string& str);

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string trim(const std::string& str);

/**
 * Форматирует строку (простой аналог std::format)
 */
template<typename... Args>
std::string format(const std::string& fmt, Args... args);

} // namespace StringUtils

#endif // STRING_UTILS_HPP
```

## **Пример заголовочного файла для библиотеки (lib/include/MathUtils.hpp):**

```cpp
#ifndef MATH_UTILS_HPP
#define MATH_UTILS_HPP

#include <vector>
#include <cmath>

/**
 * Библиотека математических утилит
 * Используется в демонстрационном проекте File Manager Pro
 */
namespace MathUtils {

/**
 * Вычисляет среднее значение
 */
double mean(const std::vector<double>& values);

/**
 * Вычисляет стандартное отклонение
 */
double stddev(const std::vector<double>& values);

/**
 * Находит минимальное значение
 */
double min(const std::vector<double>& values);

/**
 * Находит максимальное значение
 */
double max(const std::vector<double>& values);

/**
 * Округляет число до указанного количества знаков
 */
double round(double value, int decimals = 2);

/**
 * Генерирует случайное число в диапазоне
 */
double random(double min, double max);

/**
 * Линейная интерполяция
 */
double lerp(double a, double b, double t);

} // namespace MathUtils

#endif // MATH_UTILS_HPP
```

## **Пример CMakeLists.txt для работы с библиотеками:**

```cmake
# Поиск библиотек
find_library(STRING_UTILS_LIB StringUtils 
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib
    REQUIRED
)

find_library(MATH_UTILS_LIB MathUtils
    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/lib
    REQUIRED
)

# Подключение библиотек к целевому файлу
target_link_libraries(${PROJECT_NAME}
    ${STRING_UTILS_LIB}
    ${MATH_UTILS_LIB}
)

# Добавление директории с заголовками библиотек
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
)
```

Эта структура демонстрирует:

1. **Организацию библиотек** в проекте
2. **Интеграцию с системами сборки** (CMake, Makefile)
3. **Управление зависимостями** через File Manager Pro
4. **Лучшие практики** работы с библиотеками в C++
