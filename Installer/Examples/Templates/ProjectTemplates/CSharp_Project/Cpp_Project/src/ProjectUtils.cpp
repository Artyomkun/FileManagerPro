/*
 * ProjectUtils.cpp
 * Утилиты для демонстрации возможностей File Manager Pro
 * Чистый C++ код без создания файлов на диске
 */

#include "ProjectUtils.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

namespace ProjectUtils {

// ============================================
// Реализация класса ProjectAnalyzer
// ============================================

ProjectAnalyzer::ProjectAnalyzer(const std::string& projectName)
    : projectName(projectName), totalSize(0), fileCount(0), folderCount(0) {
    std::cout << "Анализатор проекта создан: " << projectName << std::endl;
}

void ProjectAnalyzer::addFile(const FileInfo& file) {
    files.push_back(file);
    totalSize += file.size;
    fileCount++;
    
    // Группировка по расширениям
    extensionStats[file.extension]++;
}

void ProjectAnalyzer::addFolder(const std::string& folderName) {
    folders.push_back(folderName);
    folderCount++;
}

void ProjectAnalyzer::displayProjectInfo() const {
    std::cout << "\n=== Анализ проекта: " << projectName << " ===" << std::endl;
    std::cout << "Всего файлов: " << fileCount << std::endl;
    std::cout << "Всего папок: " << folderCount << std::endl;
    std::cout << "Общий размер: " << formatFileSize(totalSize) << std::endl;
    
    if (!files.empty()) {
        std::cout << "\nСамые большие файлы:" << std::endl;
        auto sortedFiles = files;
        std::sort(sortedFiles.begin(), sortedFiles.end(),
                 [](const FileInfo& a, const FileInfo& b) {
                     return a.size > b.size;
                 });
        
        for (size_t i = 0; i < std::min(sortedFiles.size(), size_t(5)); i++) {
            std::cout << "  " << sortedFiles[i].name << " - " 
                      << formatFileSize(sortedFiles[i].size) << std::endl;
        }
    }
    
    if (!extensionStats.empty()) {
        std::cout << "\nСтатистика по расширениям:" << std::endl;
        for (const auto& [ext, count] : extensionStats) {
            std::cout << "  " << ext << ": " << count << " файлов" << std::endl;
        }
    }
    
    std::cout << "=====================================" << std::endl;
}

std::string ProjectAnalyzer::formatFileSize(size_t bytes) const {
    const char* units[] = {"байт", "КБ", "МБ", "ГБ", "ТБ"};
    double size = static_cast<double>(bytes);
    int unitIndex = 0;
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return oss.str();
}

// ============================================
// Реализация класса CodeFormatter
// ============================================

std::string CodeFormatter::formatCppCode(const std::string& rawCode) {
    std::string formatted = rawCode;
    
    // Удаляем лишние пробелы в начале строк
    size_t pos = 0;
    while ((pos = formatted.find("\n ", pos)) != std::string::npos) {
        size_t spaces = 1;
        while (formatted[pos + spaces] == ' ') spaces++;
        if (spaces > 4) { // Сохраняем отступы, но удаляем лишние
            formatted.erase(pos + 1, spaces - 4);
        }
        pos += 5;
    }
    
    // Заменяем табы на пробелы
    size_t tabPos;
    while ((tabPos = formatted.find('\t')) != std::string::npos) {
        formatted.replace(tabPos, 1, "    ");
    }
    
    return formatted;
}

std::string CodeFormatter::validateFileName(const std::string& fileName) {
    std::string validName = fileName;
    
    // Удаляем недопустимые символы в именах файлов
    const std::string invalidChars = "<>:\"/\\|?*";
    for (char& c : validName) {
        if (invalidChars.find(c) != std::string::npos) {
            c = '_';
        }
    }
    
    // Ограничиваем длину имени
    if (validName.length() > 255) {
        validName = validName.substr(0, 255);
    }
    
    return validName;
}

// ============================================
// Реализация утилитных функций
// ============================================

std::string getCurrentDateTime() {
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);
    return std::string(buffer);
}

std::string generateFileHeader(const std::string& fileName, 
                               const std::string& author) {
    std::ostringstream header;
    
    header << "/*\n";
    header << " * " << fileName << "\n";
    header << " * Создано с помощью File Manager Pro\n";
    header << " * Автор: " << author << "\n";
    header << " * Дата: " << getCurrentDateTime() << "\n";
    header << " */\n\n";
    
    return header.str();
}

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string part;
    std::istringstream stream(path);
    
    while (std::getline(stream, part, '/')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    
    return parts;
}

std::string joinPath(const std::vector<std::string>& parts) {
    std::ostringstream path;
    for (size_t i = 0; i < parts.size(); i++) {
        if (i > 0) path << "/";
        path << parts[i];
    }
    return path.str();
}

bool isValidExtension(const std::string& extension) {
    const std::vector<std::string> validExtensions = {
        ".cpp", ".hpp", ".h", ".c", ".cc", ".cxx",
        ".txt", ".md", ".json", ".xml", ".ini",
        ".jpg", ".png", ".gif", ".bmp"
    };
    
    std::string extLower = extension;
    std::transform(extLower.begin(), extLower.end(), extLower.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    
    return std::find(validExtensions.begin(), validExtensions.end(), extLower) 
           != validExtensions.end();
}

// ============================================
// Реализация шаблонных функций
// ============================================

template<>
std::string TypeConverter<int>::toString(const int& value) {
    return std::to_string(value);
}

template<>
std::string TypeConverter<double>::toString(const double& value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value;
    return oss.str();
}

template<>
std::string TypeConverter<std::string>::toString(const std::string& value) {
    return value;
}

template<>
int TypeConverter<int>::fromString(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (...) {
        return 0;
    }
}

template<>
double TypeConverter<double>::fromString(const std::string& str) {
    try {
        return std::stod(str);
    } catch (...) {
        return 0.0;
    }
}

// ============================================
// Демонстрационные функции
// ============================================

void demonstrateContainerOperations() {
    std::cout << "\n=== Операции с контейнерами STL ===" << std::endl;
    
    // Создание и заполнение контейнера
    std::vector<FileInfo> projectFiles = {
        {"main.cpp", 2048, ".cpp", "2024-01-15"},
        {"utils.hpp", 1024, ".hpp", "2024-01-15"},
        {"config.json", 512, ".json", "2024-01-14"},
        {"readme.md", 256, ".md", "2024-01-13"},
        {"data.bin", 8192, ".bin", "2024-01-12"}
    };
    
    std::cout << "Всего файлов: " << projectFiles.size() << std::endl;
    
    // Поиск файлов по расширению
    auto cppFiles = std::count_if(projectFiles.begin(), projectFiles.end(),
                                 [](const FileInfo& file) {
                                     return file.extension == ".cpp";
                                 });
    
    std::cout << "Файлов .cpp: " << cppFiles << std::endl;
    
    // Сортировка по дате
    std::sort(projectFiles.begin(), projectFiles.end(),
             [](const FileInfo& a, const FileInfo& b) {
                 return a.modifiedDate > b.modifiedDate; // Сначала новые
             });
    
    std::cout << "Последние измененные файлы:" << std::endl;
    for (const auto& file : projectFiles) {
        std::cout << "  " << file.name << " (" << file.modifiedDate << ")" << std::endl;
    }
}

void demonstrateStringManipulation() {
    std::cout << "\n=== Манипуляции со строками ===" << std::endl;
    
    std::string filePath = "C:/Projects/FileManagerPro/src/utils/StringUtils.cpp";
    
    // Разбор пути к файлу
    std::cout << "Полный путь: " << filePath << std::endl;
    
    size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string fileName = filePath.substr(lastSlash + 1);
        std::cout << "Имя файла: " << fileName << std::endl;
    }
    
    size_t lastDot = filePath.find_last_of('.');
    if (lastDot != std::string::npos) {
        std::string extension = filePath.substr(lastDot);
        std::cout << "Расширение: " << extension << std::endl;
    }
    
    // Преобразование регистра
    std::string mixedCase = "FileManagerPro.cpp";
    std::string lowerCase = mixedCase;
    std::transform(lowerCase.begin(), lowerCase.end(), lowerCase.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    
    std::cout << "В нижнем регистре: " << lowerCase << std::endl;
}

void demonstrateMemoryManagement() {
    std::cout << "\n=== Управление памятью ===" << std::endl;
    
    // Демонстрация умных указателей
    std::cout << "Использование unique_ptr:" << std::endl;
    {
        auto uniqueFile = std::make_unique<FileInfo>("unique.txt", 1024, ".txt", "2024-01-15");
        std::cout << "  Создан файл: " << uniqueFile->name << std::endl;
        // unique_ptr автоматически удалится при выходе из области видимости
    }
    
    std::cout << "Использование shared_ptr:" << std::endl;
    {
        auto sharedFile1 = std::make_shared<FileInfo>("shared.txt", 2048, ".txt", "2024-01-15");
        auto sharedFile2 = sharedFile1; // Разделяемое владение
        
        std::cout << "  Счетчик ссылок: " << sharedFile1.use_count() << std::endl;
        std::cout << "  Имя файла: " << sharedFile1->name << std::endl;
    }
    
    std::cout << "Использование weak_ptr:" << std::endl;
    {
        auto shared = std::make_shared<FileInfo>("weak.txt", 512, ".txt", "2024-01-15");
        std::weak_ptr<FileInfo> weak = shared;
        
        if (auto locked = weak.lock()) {
            std::cout << "  Файл доступен: " << locked->name << std::endl;
        } else {
            std::cout << "  Файл недоступен" << std::endl;
        }
    }
}

void demonstrateProjectUtils() {
    std::cout << "\n=== Демонстрация утилит проекта ===" << std::endl;
    
    // Создаем анализатор проекта
    ProjectAnalyzer analyzer("FileManagerPro Demo");
    
    // Добавляем файлы
    analyzer.addFile({"main.cpp", 2048, ".cpp", "2024-01-15"});
    analyzer.addFile({"utils.hpp", 1024, ".hpp", "2024-01-15"});
    analyzer.addFile({"config.json", 512, ".json", "2024-01-14"});
    analyzer.addFile({"readme.md", 256, ".md", "2024-01-13"});
    
    // Добавляем папки
    analyzer.addFolder("src");
    analyzer.addFolder("include");
    analyzer.addFolder("docs");
    
    // Выводим информацию
    analyzer.displayProjectInfo();
    
    // Демонстрация форматирования
    CodeFormatter formatter;
    std::string rawCode = "int main(){\n\tprintf(\"Hello\");\n}";
    std::string formatted = formatter.formatCppCode(rawCode);
    
    std::cout << "\nФорматирование кода:" << std::endl;
    std::cout << "До: " << rawCode << std::endl;
    std::cout << "После: " << formatted << std::endl;
    
    // Демонстрация валидации имени файла
    std::string badName = "file<with>invalid*chars?.txt";
    std::string validName = formatter.validateFileName(badName);
    
    std::cout << "\nВалидация имени файла:" << std::endl;
    std::cout << "Некорректное имя: " << badName << std::endl;
    std::cout << "Корректное имя: " << validName << std::endl;
    
    // Демонстрация вспомогательных функций
    std::cout << "\nВспомогательные функции:" << std::endl;
    std::cout << "Текущая дата: " << getCurrentDateTime() << std::endl;
    
    std::string header = generateFileHeader("demo.cpp", "File Manager Pro");
    std::cout << "Заголовок файла:\n" << header << std::endl;
}

} // namespace ProjectUtils