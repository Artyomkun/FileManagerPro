#ifndef PROJECT_UTILS_HPP
#define PROJECT_UTILS_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace ProjectUtils {

// Структура для хранения информации о файле
struct FileInfo {
    std::string name;
    size_t size;
    std::string extension;
    std::string modifiedDate;
    
    FileInfo(const std::string& n = "", size_t s = 0, const std::string& ext = "", const std::string& date = ""): name(n), size(s), extension(ext), modifiedDate(date) {}
};

// Класс для анализа проекта
class ProjectAnalyzer {
private:
    std::string projectName;
    std::vector<FileInfo> files;
    std::vector<std::string> folders;
    std::map<std::string, int> extensionStats;
    size_t totalSize;
    size_t fileCount;
    size_t folderCount;
    
    std::string formatFileSize(size_t bytes) const;
    
public:
    ProjectAnalyzer(const std::string& projectName);
    
    void addFile(const FileInfo& file);
    void addFolder(const std::string& folderName);
    void displayProjectInfo() const;
    
    size_t getTotalSize() const { return totalSize; }
    size_t getFileCount() const { return fileCount; }
    size_t getFolderCount() const { return folderCount; }
};

// Класс для форматирования кода
class CodeFormatter {
public:
    std::string formatCppCode(const std::string& rawCode);
    std::string validateFileName(const std::string& fileName);
};

// Шаблонный класс для преобразования типов
template<typename T>
class TypeConverter {
public:
    static std::string toString(const T& value);
    static T fromString(const std::string& str);
};

// Утилитные функции
std::string getCurrentDateTime();
std::string generateFileHeader(const std::string& fileName, const std::string& author = "File Manager Pro");
std::vector<std::string> splitPath(const std::string& path);
std::string joinPath(const std::vector<std::string>& parts);
bool isValidExtension(const std::string& extension);

// Демонстрационные функции
void demonstrateContainerOperations();
void demonstrateStringManipulation();
void demonstrateMemoryManagement();
void demonstrateProjectUtils();

} // namespace ProjectUtils

#endif // PROJECT_UTILS_HPP