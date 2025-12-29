/*
 * Тесты для ProjectUtils утилит
 */

#include "ProjectUtils.hpp"
#include <cassert>
#include <iostream>

namespace ProjectUtilsTests {

void testTypeConverter() {
    std::cout << "Тест: testTypeConverter\n";
    
    using namespace ProjectUtils;
    
    // int
    assert(TypeConverter<int>::toString(42) == "42");
    assert(TypeConverter<int>::toString(-100) == "-100");
    assert(TypeConverter<int>::fromString("123") == 123);
    assert(TypeConverter<int>::fromString("not_a_number") == 0); // обработка ошибки
    
    // double
    std::string doubleStr = TypeConverter<double>::toString(3.14159);
    assert(doubleStr.find("3.14") != std::string::npos); // проверяем форматирование
    
    // string
    assert(TypeConverter<std::string>::toString("hello") == "hello");
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void testSplitPath() {
    std::cout << "Тест: testSplitPath\n";
    
    using namespace ProjectUtils;
    
    std::vector<std::string> result;
    
    // Unix путь
    result = splitPath("/home/user/projects/app");
    assert(result.size() == 4);
    assert(result[0] == "home");
    assert(result[3] == "app");
    
    // Windows путь (с обратными слешами)
    result = splitPath("C:\\Users\\Name\\Documents");
    assert(result.size() == 3);
    assert(result[0] == "C:");
    assert(result[2] == "Documents");
    
    // Относительный путь
    result = splitPath("src/utils/string.hpp");
    assert(result.size() == 3);
    assert(result[0] == "src");
    assert(result[2] == "string.hpp");
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void testJoinPath() {
    std::cout << "Тест: testJoinPath\n";
    
    using namespace ProjectUtils;
    
    std::vector<std::string> parts;
    
    parts = {"home", "user", "projects"};
    assert(joinPath(parts) == "home/user/projects");
    
    parts = {"C:", "Users", "Name"};
    assert(joinPath(parts) == "C:/Users/Name");
    
    parts = {"src", "utils", "string.hpp"};
    assert(joinPath(parts) == "src/utils/string.hpp");
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void testIsValidExtension() {
    std::cout << "Тест: testIsValidExtension\n";
    
    using namespace ProjectUtils;
    
    // Корректные расширения
    assert(isValidExtension(".cpp") == true);
    assert(isValidExtension(".HPP") == true); // проверка регистра
    assert(isValidExtension(".json") == true);
    assert(isValidExtension(".md") == true);
    assert(isValidExtension(".JPG") == true);
    
    // Некорректные расширения
    assert(isValidExtension(".xyz") == false);
    assert(isValidExtension(".tmp") == false);
    assert(isValidExtension(".bak") == false);
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void testProjectAnalyzer() {
    std::cout << "Тест: testProjectAnalyzer\n";
    
    using namespace ProjectUtils;
    
    ProjectAnalyzer analyzer("Test Project");
    
    // Добавляем файлы
    analyzer.addFile(FileInfo("main.cpp", 2048, ".cpp", "2024-01-15"));
    analyzer.addFile(FileInfo("utils.hpp", 1024, ".hpp", "2024-01-15"));
    analyzer.addFile(FileInfo("data.json", 512, ".json", "2024-01-14"));
    
    // Добавляем папки
    analyzer.addFolder("src");
    analyzer.addFolder("include");
    
    // Проверяем статистику
    assert(analyzer.getFileCount() == 3);
    assert(analyzer.getFolderCount() == 2);
    assert(analyzer.getTotalSize() == 3584); // 2048 + 1024 + 512
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void testCodeFormatter() {
    std::cout << "Тест: testCodeFormatter\n";
    
    using namespace ProjectUtils;
    
    CodeFormatter formatter;
    
    // Тест форматирования кода
    std::string rawCode = "int main(){\n\tprintf(\"test\");\n\tif(true){\n\t\treturn 0;\n\t}\n}";
    std::string formatted = formatter.formatCppCode(rawCode);
    
    assert(formatted.find("    ") != std::string::npos); // проверяем замену табов
    
    // Тест валидации имени файла
    std::string badName = "file<with>*bad|chars?.txt";
    std::string validName = formatter.validateFileName(badName);
    
    assert(validName.find('<') == std::string::npos);
    assert(validName.find('>') == std::string::npos);
    assert(validName.find('*') == std::string::npos);
    assert(validName.find('?') == std::string::npos);
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void runAllTests() {
    std::cout << "=== Запуск тестов ProjectUtils ===\n\n";
    
    testTypeConverter();
    testSplitPath();
    testJoinPath();
    testIsValidExtension();
    testProjectAnalyzer();
    testCodeFormatter();
    
    std::cout << "\n=== Все тесты ProjectUtils пройдены успешно! ===\n";
}

} // namespace ProjectUtilsTests