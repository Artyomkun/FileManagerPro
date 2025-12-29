/*
 * FileManager.cpp
 * Чистые утилиты для работы с файловыми путями
 * Без генерации файлов и побочных эффектов
 */

#include "FileManager.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace fs = std::filesystem;

namespace FileManager {

// ============================================
// Реализация функций FileManager
// ============================================

std::vector<fs::path> listFiles(const fs::path& directory,
                                const std::string& filter) {
    std::vector<fs::path> result;
    
    // В реальном проекте здесь был бы код для сканирования директории
    // Для демонстрации возвращаем пустой список
    
    std::cout << "[FileManager] Запрос списка файлов для: " << directory << std::endl;
    std::cout << "[FileManager] Фильтр: " << (filter.empty() ? "*" : filter) << std::endl;
    
    // Возвращаем пустой список, так как это демонстрация без реальных файлов
    return result;
}

bool isCppProjectFile(const fs::path& filepath) {
    if (!filepath.has_extension()) {
        return false;
    }
    
    std::string ext = filepath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    
    const std::vector<std::string> cppExtensions = {
        ".cpp", ".cc", ".cxx", ".c++",
        ".hpp", ".hh", ".hxx", ".h++",
        ".h", ".c",
        ".ipp", ".inl", ".tpp", ".txx"
    };
    
    return std::find(cppExtensions.begin(), cppExtensions.end(), ext) != cppExtensions.end();
}

bool isValidFilename(const std::string& filename) {
    if (filename.empty() || filename.length() > 255) {
        return false;
    }
    
    // Запрещенные символы в именах файлов Windows
    const std::string invalidChars = "<>:\"/\\|?*";
    
    for (char c : filename) {
        // Проверка на управляющие символы (0-31)
        if (c < 32) {
            return false;
        }
        
        // Проверка на запрещенные символы
        if (invalidChars.find(c) != std::string::npos) {
            return false;
        }
    }
    
    // Запрещенные имена в Windows
    const std::vector<std::string> reservedNames = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    std::string upperName = filename;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(),
                  [](unsigned char c) { return std::toupper(c); });
    
    // Удаляем расширение для проверки
    size_t dotPos = upperName.find_last_of('.');
    std::string nameWithoutExt = (dotPos != std::string::npos) ? 
                                  upperName.substr(0, dotPos) : upperName;
    
    if (std::find(reservedNames.begin(), reservedNames.end(), nameWithoutExt) != reservedNames.end()) {
        return false;
    }
    
    return true;
}

fs::path normalizePath(const fs::path& path) {
    if (path.empty()) {
        return path;
    }
    
    try {
        // Приводим к абсолютному пути и нормализуем
        fs::path normalized = fs::absolute(path).lexically_normal();
        
        // Убираем trailing slash для директорий
        std::string pathStr = normalized.string();
        if (pathStr.length() > 1 && 
            (pathStr.back() == '/' || pathStr.back() == '\\')) {
            pathStr.pop_back();
        }
        
        return fs::path(pathStr);
    } catch (const fs::filesystem_error&) {
        // В случае ошибки возвращаем оригинальный путь
        return path;
    }
}

std::string getFileExtension(const fs::path& filepath) {
    if (!filepath.has_extension()) {
        return "";
    }
    
    std::string ext = filepath.extension().string();
    
    // Убираем точку в начале
    if (!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1);
    }
    
    // Приводим к нижнему регистру
    std::transform(ext.begin(), ext.end(), ext.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    
    return ext;
}

FileType getFileType(const fs::path& filepath) {
    if (!filepath.has_extension()) {
        return FileType::UNKNOWN;
    }
    
    std::string ext = getFileExtension(filepath);
    
    // Код
    if (ext == "cpp" || ext == "cc" || ext == "cxx" || 
        ext == "hpp" || ext == "hh" || ext == "hxx" || 
        ext == "c" || ext == "h") {
        return FileType::SOURCE_CODE;
    }
    
    // Скрипты
    if (ext == "py" || ext == "js" || ext == "ts" || 
        ext == "java" || ext == "cs" || ext == "php") {
        return FileType::SCRIPT;
    }
    
    // Документы
    if (ext == "txt" || ext == "md" || ext == "rtf" || 
        ext == "doc" || ext == "docx" || ext == "pdf") {
        return FileType::DOCUMENT;
    }
    
    // Данные
    if (ext == "json" || ext == "xml" || ext == "yaml" || 
        ext == "yml" || ext == "csv" || ext == "ini") {
        return FileType::DATA;
    }
    
    // Изображения
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || 
        ext == "gif" || ext == "bmp" || ext == "svg") {
        return FileType::IMAGE;
    }
    
    // Архивы
    if (ext == "zip" || ext == "rar" || ext == "7z" || 
        ext == "tar" || ext == "gz") {
        return FileType::ARCHIVE;
    }
    
    return FileType::UNKNOWN;
}

std::string formatFileSize(uintmax_t bytes) {
    if (bytes == 0) return "0 Б";
    
    const char* units[] = {"Б", "КБ", "МБ", "ГБ", "ТБ"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    std::ostringstream oss;
    if (unitIndex == 0) {
        oss << static_cast<int>(size) << " " << units[unitIndex];
    } else {
        oss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    }
    
    return oss.str();
}

fs::path combinePaths(const fs::path& base, const fs::path& relative) {
    if (relative.empty()) return base;
    
    // Если относительный путь абсолютный, возвращаем его
    if (relative.is_absolute()) {
        return relative;
    }
    
    // Комбинируем пути
    try {
        return (base / relative).lexically_normal();
    } catch (const fs::filesystem_error&) {
        return base / relative;
    }
}

bool isPathWithinDirectory(const fs::path& path, const fs::path& directory) {
    try {
        // Приводим пути к нормальной форме
        fs::path normPath = fs::absolute(path).lexically_normal();
        fs::path normDir = fs::absolute(directory).lexically_normal();
        
        // Проверяем, что путь начинается с директории
        auto mismatch = std::mismatch(normDir.begin(), normDir.end(),
                                      normPath.begin());
        
        return mismatch.first == normDir.end();
    } catch (const fs::filesystem_error&) {
        return false;
    }
}

std::string getRelativePath(const fs::path& path, const fs::path& base) {
    try {
        fs::path absolutePath = fs::absolute(path);
        fs::path absoluteBase = fs::absolute(base);
        
        // Пытаемся получить относительный путь
        fs::path relative = fs::relative(absolutePath, absoluteBase);
        
        if (relative.empty()) {
            return ".";
        }
        
        return relative.string();
    } catch (const fs::filesystem_error&) {
        return path.string();
    }
}

void demonstratePathManipulation() {
    std::cout << "\n=== Манипуляции с путями (FileManager) ===" << std::endl;
    
    // Примеры путей
    fs::path examplePath = "C:/Projects/FileManagerPro/src/main.cpp";
    fs::path baseDir = "C:/Projects/FileManagerPro";
    
    std::cout << "Пример пути: " << examplePath << std::endl;
    std::cout << "Имя файла: " << examplePath.filename() << std::endl;
    std::cout << "Родительская папка: " << examplePath.parent_path() << std::endl;
    std::cout << "Расширение: " << getFileExtension(examplePath) << std::endl;
    std::cout << "Тип файла: " << static_cast<int>(getFileType(examplePath)) << std::endl;
    
    // Комбинирование путей
    fs::path combined = combinePaths(baseDir, "build/output.exe");
    std::cout << "\nКомбинированный путь: " << combined << std::endl;
    
    // Относительный путь
    std::string relative = getRelativePath(examplePath, baseDir);
    std::cout << "Относительный путь: " << relative << std::endl;
    
    // Валидация имен файлов
    std::cout << "\nВалидация имен файлов:" << std::endl;
    std::cout << "  'main.cpp': " << (isValidFilename("main.cpp") ? "✓" : "✗") << std::endl;
    std::cout << "  'file<bad>.txt': " << (isValidFilename("file<bad>.txt") ? "✓" : "✗") << std::endl;
    std::cout << "  'CON.txt': " << (isValidFilename("CON.txt") ? "✓" : "✗") << std::endl;
    
    // Проверка C++ файлов
    std::cout << "\nПроверка C++ файлов:" << std::endl;
    std::cout << "  'utils.hpp': " << (isCppProjectFile("utils.hpp") ? "✓" : "✗") << std::endl;
    std::cout << "  'data.json': " << (isCppProjectFile("data.json") ? "✓" : "✗") << std::endl;
}

} // namespace FileManager