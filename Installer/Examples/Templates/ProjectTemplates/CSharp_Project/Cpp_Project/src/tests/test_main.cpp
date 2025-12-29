/*
 * Главный файл для запуска всех тестов
 * Демонстрация тестирования в File Manager Pro
 */

#include <iostream>

// Объявления функций тестирования
namespace FileManagerTests {
    void runAllTests();
}

namespace ProjectUtilsTests {
    void runAllTests();
}

int main() {
    std::cout << "====================================\n";
    std::cout << "File Manager Pro - Запуск тестов\n";
    std::cout << "====================================\n\n";
    
    try {
        // Запускаем тесты FileManager
        std::cout << "1. Тестирование FileManager утилит:\n";
        FileManagerTests::runAllTests();
        
        std::cout << "\n";
        
        // Запускаем тесты ProjectUtils
        std::cout << "2. Тестирование ProjectUtils утилит:\n";
        ProjectUtilsTests::runAllTests();
        
        std::cout << "\n";
        std::cout << "====================================\n";
        std::cout << "ВСЕ ТЕСТЫ УСПЕШНО ПРОЙДЕНЫ!\n";
        std::cout << "====================================\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Ошибка при выполнении тестов: " << e.what() << std::endl;
        return 1;
        
    } catch (...) {
        std::cerr << "\n❌ Неизвестная ошибка при выполнении тестов" << std::endl;
        return 1;
    }
}

/*
 * Демонстрация тестирования в File Manager Pro:
 * 
 * 1. Компиляция тестов:
 *    g++ -std=c++17 -I../include test_main.cpp test_filemanager.cpp test_projectutils.cpp \
 *        ../src/FileManager.cpp ../src/ProjectUtils.cpp -o run_tests
 * 
 * 2. Запуск тестов:
 *    ./run_tests
 * 
 * 3. Использование в File Manager Pro:
 *    - Открывайте файлы тестов двойным кликом
 *    - Используйте поиск (Ctrl+F) для навигации
 *    - Запускайте тесты через терминал
 */