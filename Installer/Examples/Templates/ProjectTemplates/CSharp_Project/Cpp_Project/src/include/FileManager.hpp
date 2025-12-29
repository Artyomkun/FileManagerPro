#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>

namespace fs = std::filesystem;

namespace FileManager {

// Типы файлов для классификации
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

/**
 * Получает список файлов в директории (демонстрационная функция)
 */
std::vector<fs::path> listFiles(const fs::path& directory,const std::string& filter = "*");

/**
 * Проверяет, является ли файл частью C++ проекта
 */
bool isCppProjectFile(const fs::path& filepath);

/**
 * Проверяет валидность имени файла
 */
bool isValidFilename(const std::string& filename);

/**
 * Нормализует путь (убирает лишние символы, приводит к нормальной форме)
 */
fs::path normalizePath(const fs::path& path);

/**
 * Получает расширение файла без точки
 */
std::string getFileExtension(const fs::path& filepath);

/**
 * Определяет тип файла по расширению
 */
FileType getFileType(const fs::path& filepath);

/**
 * Форматирует размер файла в читаемом виде
 */
std::string formatFileSize(uintmax_t bytes);

/**
 * Комбинирует базовый путь с относительным
 */
fs::path combinePaths(const fs::path& base, const fs::path& relative);

/**
 * Проверяет, находится ли путь внутри указанной директории
 */
bool isPathWithinDirectory(const fs::path& path, const fs::path& directory);

/**
 * Получает относительный путь относительно базовой директории
 */
std::string getRelativePath(const fs::path& path, const fs::path& base);

/**
 * Демонстрация работы с путями
 */
void demonstratePathManipulation();

} // namespace FileManager

#endif // FILE_MANAGER_HPP