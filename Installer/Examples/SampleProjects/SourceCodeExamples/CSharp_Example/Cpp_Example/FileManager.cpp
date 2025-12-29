#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono>

#ifdef _WIN32
    #include <windows.h>
    #include <shellapi.h>
    #include <shlobj.h>
    #include <fileapi.h>
    #pragma comment(lib, "shell32.lib")
#else
    #include <unistd.h>
    #include <pwd.h>
    #include <sys/ioctl.h>
    #include <termios.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <fcntl.h>
#endif

namespace fs = std::filesystem;

// ==================== Конструкторы/Деструкторы ====================

FileManager::FileManager() : currentPath(fs::current_path()) {
    loadHistory();
    loadBookmarks();
    
    #ifdef _WIN32
        systemRoot = getenv("SystemRoot");
        userProfile = getenv("USERPROFILE");
    #else
        systemRoot = "/";
        userProfile = getenv("HOME");
        if (userProfile.empty()) {
            struct passwd* pw = getpwuid(getuid());
            if (pw) userProfile = pw->pw_dir;
        }
    #endif
    
    initializeTerminal();
}

FileManager::FileManager(const std::string& startPath) {
    if (fs::exists(startPath) && fs::is_directory(startPath)) {
        currentPath = fs::path(startPath);
    } else {
        currentPath = fs::current_path();
        std::cerr << "Warning: Invalid start path. Using current directory.\n";
    }
    
    loadHistory();
    loadBookmarks();
    initializeTerminal();
}

FileManager::~FileManager() {
    saveHistory();
    saveBookmarks();
    cleanupTerminal();
}

// ==================== Навигация по директориям ====================

bool FileManager::changeDirectory(const std::string& path) {
    try {
        fs::path newPath;
        
        // Обработка специальных символов
        if (path == "~") {
            newPath = userProfile;
        } else if (path == "..") {
            newPath = currentPath.parent_path();
        } else if (path == "/" || path == "\\") {
            newPath = systemRoot;
        } else if (path == "-") {
            // Назад по истории
            if (!directoryHistory.empty() && directoryHistory.size() > 1) {
                directoryHistory.pop_back(); // Удаляем текущий
                newPath = directoryHistory.back();
                directoryHistory.pop_back(); // Удаляем предыдущий для возврата
            } else {
                return false;
            }
        } else {
            // Относительный или абсолютный путь
            newPath = currentPath / path;
        }
        
        // Проверка существования и типа
        if (!fs::exists(newPath)) {
            std::cerr << "Error: Path does not exist: " << newPath << "\n";
            return false;
        }
        
        if (!fs::is_directory(newPath)) {
            std::cerr << "Error: Not a directory: " << newPath << "\n";
            return false;
        }
        
        // Проверка доступа
        #ifdef _WIN32
            DWORD attr = GetFileAttributesW(newPath.wstring().c_str());
            if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_HIDDEN)) {
                std::cerr << "Error: Cannot access directory\n";
                return false;
            }
        #else
            if (access(newPath.c_str(), R_OK) != 0) {
                std::cerr << "Error: Permission denied\n";
                return false;
            }
        #endif
        
        // Сохраняем в историю
        directoryHistory.push_back(currentPath);
        if (directoryHistory.size() > MAX_HISTORY_SIZE) {
            directoryHistory.erase(directoryHistory.begin());
        }
        
        // Меняем директорию
        currentPath = fs::canonical(newPath);
        addToRecentDirectories(currentPath.string());
        
        return true;
        
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << "\n";
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return false;
    }
}

std::string FileManager::getCurrentPath() const {
    return currentPath.string();
}

bool FileManager::goToParent() {
    if (!currentPath.has_parent_path() || currentPath == currentPath.root_path()) {
        return false;
    }
    
    directoryHistory.push_back(currentPath);
    currentPath = currentPath.parent_path();
    return true;
}

bool FileManager::goToRoot() {
    directoryHistory.push_back(currentPath);
    currentPath = currentPath.root_path();
    return true;
}

bool FileManager::goToHome() {
    directoryHistory.push_back(currentPath);
    currentPath = fs::path(userProfile);
    return true;
}

// ==================== Просмотр файлов ====================

std::vector<FileInfo> FileManager::listFiles(bool showHidden, SortBy sortBy, bool reverse) const {
    std::vector<FileInfo> files;
    
    try {
        for (const auto& entry : fs::directory_iterator(currentPath)) {
            FileInfo info;
            
            try {
                info.name = entry.path().filename().string();
                info.path = entry.path().string();
                info.isDirectory = entry.is_directory();
                info.isRegularFile = entry.is_regular_file();
                info.isSymlink = entry.is_symlink();
                
                // Получаем расширение для файлов
                if (info.isRegularFile) {
                    info.extension = entry.path().extension().string();
                }
                
                // Размер файла
                if (info.isRegularFile && !info.isSymlink) {
                    info.size = entry.file_size();
                } else if (info.isDirectory) {
                    info.size = 0; // Для директорий размер 0
                }
                
                // Время модификации
                auto ftime = entry.last_write_time();
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                info.modifiedTime = std::chrono::system_clock::to_time_t(sctp);
                
                // Атрибуты файла
                #ifdef _WIN32
                    DWORD attr = GetFileAttributesW(entry.path().wstring().c_str());
                    info.isHidden = (attr & FILE_ATTRIBUTE_HIDDEN) != 0;
                    info.isReadOnly = (attr & FILE_ATTRIBUTE_READONLY) != 0;
                    info.isSystem = (attr & FILE_ATTRIBUTE_SYSTEM) != 0;
                #else
                    struct stat st;
                    if (stat(entry.path().c_str(), &st) == 0) {
                        info.isHidden = info.name[0] == '.';
                        info.permissions = st.st_mode;
                    }
                #endif
                
                // Пропускаем скрытые файлы если не запрошены
                if (!showHidden && info.isHidden) {
                    continue;
                }
                
                files.push_back(info);
                
            } catch (const fs::filesystem_error& e) {
                // Пропускаем файлы с ошибками доступа
                continue;
            }
        }
        
        // Сортировка
        sortFiles(files, sortBy, reverse);
        
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error listing directory: " << e.what() << "\n";
    }
    
    return files;
}

void FileManager::sortFiles(std::vector<FileInfo>& files, SortBy sortBy, bool reverse) const {
    auto comparator = [sortBy, reverse](const FileInfo& a, const FileInfo& b) {
        bool result = false;
        
        switch (sortBy) {
            case SortBy::NAME:
                result = a.name < b.name;
                break;
            case SortBy::SIZE:
                result = a.size < b.size;
                break;
            case SortBy::DATE:
                result = a.modifiedTime < b.modifiedTime;
                break;
            case SortBy::TYPE:
                if (a.isDirectory != b.isDirectory) {
                    result = a.isDirectory > b.isDirectory; // Директории первыми
                } else {
                    result = a.extension < b.extension;
                }
                break;
        }
        
        return reverse ? !result : result;
    };
    
    std::sort(files.begin(), files.end(), comparator);
}

void FileManager::displayFiles(const std::vector<FileInfo>& files, DisplayMode mode) const {
    if (files.empty()) {
        std::cout << "Directory is empty.\n";
        return;
    }
    
    switch (mode) {
        case DisplayMode::LIST:
            displayList(files);
            break;
        case DisplayMode::DETAILS:
            displayDetails(files);
            break;
        case DisplayMode::GRID:
            displayGrid(files);
            break;
        case DisplayMode::TREE:
            displayTree();
            break;
    }
}

void FileManager::displayList(const std::vector<FileInfo>& files) const {
    std::cout << "\nContents of " << currentPath.string() << ":\n";
    std::cout << std::string(60, '-') << "\n";
    
    for (const auto& file : files) {
        std::cout << (file.isDirectory ? "[DIR]  " : "[FILE] ");
        std::cout << std::left << std::setw(40) << file.name;
        
        if (!file.isDirectory) {
            std::cout << " " << formatSize(file.size);
        }
        
        if (file.isHidden) std::cout << " <H>";
        if (file.isSymlink) std::cout << " <L>";
        
        std::cout << "\n";
    }
    
    std::cout << std::string(60, '-') << "\n";
    std::cout << "Total: " << files.size() << " items\n";
}

void FileManager::displayDetails(const std::vector<FileInfo>& files) const {
    // Определяем ширину колонок
    size_t maxName = 30;
    size_t maxSize = 10;
    
    for (const auto& file : files) {
        maxName = std::max(maxName, file.name.length());
    }
    
    std::cout << "\n" << std::left << std::setw(maxName + 2) << "Name"
              << std::setw(10) << "Type"
              << std::setw(maxSize + 2) << "Size"
              << "Modified" << "\n";
    std::cout << std::string(maxName + maxSize + 30, '-') << "\n";
    
    for (const auto& file : files) {
        // Имя
        std::cout << std::left << std::setw(maxName + 2) 
                  << (file.name.length() > maxName ? 
                      file.name.substr(0, maxName - 3) + "..." : file.name);
        
        // Тип
        std::string type = file.isDirectory ? "DIR" : 
                          file.isSymlink ? "LNK" : "FILE";
        std::cout << std::setw(10) << type;
        
        // Размер
        if (file.isDirectory) {
            std::cout << std::setw(maxSize + 2) << "<DIR>";
        } else {
            std::cout << std::setw(maxSize + 2) << formatSize(file.size);
        }
        
        // Дата модификации
        char timeStr[20];
        std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", 
                     std::localtime(&file.modifiedTime));
        std::cout << timeStr;
        
        // Флаги
        if (file.isHidden) std::cout << " H";
        if (file.isReadOnly) std::cout << " R";
        
        std::cout << "\n";
    }
}

void FileManager::displayGrid(const std::vector<FileInfo>& files) const {
    #ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        int terminalWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    #else
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        int terminalWidth = w.ws_col;
    #endif
    
    const int COLUMN_WIDTH = 25;
    int columns = std::max(1, terminalWidth / COLUMN_WIDTH);
    int rows = (files.size() + columns - 1) / columns;
    
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            int index = row + col * rows;
            if (index < files.size()) {
                const auto& file = files[index];
                std::string displayName = file.name;
                
                if (displayName.length() > COLUMN_WIDTH - 2) {
                    displayName = displayName.substr(0, COLUMN_WIDTH - 5) + "...";
                }
                
                std::cout << std::left << std::setw(COLUMN_WIDTH)
                          << (file.isDirectory ? "/" + displayName : displayName);
            }
        }
        std::cout << "\n";
    }
}

void FileManager::displayTree() const {
    displayTreeRecursive(currentPath, 0, "", true);
}

void FileManager::displayTreeRecursive(const fs::path& path, int depth, 
                                      const std::string& prefix, bool isLast) const {
    // Текущая директория
    std::string connector = (depth == 0) ? "" : 
                           (isLast ? "└── " : "├── ");
    
    std::cout << prefix << connector << path.filename().string();
    
    if (fs::is_directory(path)) {
        std::cout << "/\n";
    } else {
        std::cout << "\n";
        return;
    }
    
    // Содержимое директории
    try {
        std::vector<fs::directory_entry> entries;
        for (const auto& entry : fs::directory_iterator(path)) {
            entries.push_back(entry);
        }
        
        std::sort(entries.begin(), entries.end(), 
                  [](const fs::directory_entry& a, const fs::directory_entry& b) {
                      if (a.is_directory() != b.is_directory()) {
                          return a.is_directory() > b.is_directory();
                      }
                      return a.path().filename() < b.path().filename();
                  });
        
        for (size_t i = 0; i < entries.size(); i++) {
            std::string newPrefix = prefix + (isLast ? "    " : "│   ");
            bool lastItem = (i == entries.size() - 1);
            
            displayTreeRecursive(entries[i].path(), depth + 1, newPrefix, lastItem);
        }
        
    } catch (const fs::filesystem_error& e) {
        std::cout << prefix << "    [Error: " << e.what() << "]\n";
    }
}

// ==================== Информация о файлах ====================

FileInfo FileManager::getFileInfo(const std::string& filename) const {
    FileInfo info;
    fs::path filePath = currentPath / filename;
    
    try {
        if (!fs::exists(filePath)) {
            throw std::runtime_error("File does not exist");
        }
        
        info.name = filePath.filename().string();
        info.path = filePath.string();
        info.isDirectory = fs::is_directory(filePath);
        info.isRegularFile = fs::is_regular_file(filePath);
        info.isSymlink = fs::is_symlink(filePath);
        
        if (info.isRegularFile && !info.isSymlink) {
            info.size = fs::file_size(filePath);
            info.extension = filePath.extension().string();
        }
        
        auto ftime = fs::last_write_time(filePath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        info.modifiedTime = std::chrono::system_clock::to_time_t(sctp);
        info.createdTime = info.modifiedTime; // Для простоты
        
        // Атрибуты
        #ifdef _WIN32
            DWORD attr = GetFileAttributesW(filePath.wstring().c_str());
            info.isHidden = (attr & FILE_ATTRIBUTE_HIDDEN) != 0;
            info.isReadOnly = (attr & FILE_ATTRIBUTE_READONLY) != 0;
            info.isSystem = (attr & FILE_ATTRIBUTE_SYSTEM) != 0;
            info.isArchive = (attr & FILE_ATTRIBUTE_ARCHIVE) != 0;
        #else
            struct stat st;
            if (stat(filePath.c_str(), &st) == 0) {
                info.isHidden = info.name[0] == '.';
                info.permissions = st.st_mode;
                info.ownerId = st.st_uid;
                info.groupId = st.st_gid;
            }
        #endif
        
    } catch (const std::exception& e) {
        std::cerr << "Error getting file info: " << e.what() << "\n";
    }
    
    return info;
}

void FileManager::displayFileInfo(const FileInfo& info) const {
    std::cout << "\n=== File Information ===\n";
    std::cout << "Name: " << info.name << "\n";
    std::cout << "Path: " << info.path << "\n";
    std::cout << "Type: " << (info.isDirectory ? "Directory" : 
                             info.isSymlink ? "Symbolic Link" : "Regular File") << "\n";
    
    if (!info.isDirectory) {
        std::cout << "Size: " << formatSize(info.size) << "\n";
        if (!info.extension.empty()) {
            std::cout << "Extension: " << info.extension << "\n";
        }
    }
    
    char timeStr[20];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", 
                 std::localtime(&info.modifiedTime));
    std::cout << "Modified: " << timeStr << "\n";
    
    // Атрибуты
    std::cout << "Attributes: ";
    if (info.isHidden) std::cout << "HIDDEN ";
    if (info.isReadOnly) std::cout << "READONLY ";
    if (info.isSystem) std::cout << "SYSTEM ";
    if (info.isSymlink) std::cout << "SYMLINK ";
    std::cout << "\n";
    
    #ifndef _WIN32
        std::cout << "Permissions: " << std::oct << info.permissions << std::dec << "\n";
    #endif
    
    std::cout << "=======================\n";
}

// ==================== Утилиты ====================

std::string FileManager::formatSize(uintmax_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    std::ostringstream oss;
    if (unitIndex == 0) {
        oss << bytes << " " << units[unitIndex];
    } else {
        oss << std::fixed << std::setprecision(2) 
            << size << " " << units[unitIndex];
    }
    
    return oss.str();
}

void FileManager::addToRecentDirectories(const std::string& path) {
    // Удаляем если уже существует
    auto it = std::find(recentDirectories.begin(), recentDirectories.end(), path);
    if (it != recentDirectories.end()) {
        recentDirectories.erase(it);
    }
    
    // Добавляем в начало
    recentDirectories.insert(recentDirectories.begin(), path);
    
    // Ограничиваем размер
    if (recentDirectories.size() > MAX_RECENT_DIRECTORIES) {
        recentDirectories.pop_back();
    }
}

// ==================== История и закладки ====================

void FileManager::loadHistory() {
    // В реальной реализации здесь чтение из файла
    directoryHistory.clear();
    directoryHistory.push_back(currentPath);
}

void FileManager::saveHistory() const {
    // В реальной реализации здесь запись в файл
}

void FileManager::loadBookmarks() {
    // Стандартные закладки
    bookmarks["Home"] = userProfile;
    bookmarks["Desktop"] = userProfile + "/Desktop";
    bookmarks["Documents"] = userProfile + "/Documents";
    bookmarks["Downloads"] = userProfile + "/Downloads";
    bookmarks["Root"] = systemRoot;
}

void FileManager::saveBookmarks() const {
    // В реальной реализации здесь запись в файл
}

bool FileManager::addBookmark(const std::string& name, const std::string& path) {
    if (bookmarks.find(name) != bookmarks.end()) {
        return false; // Уже существует
    }
    
    bookmarks[name] = path;
    return true;
}

bool FileManager::goToBookmark(const std::string& name) {
    auto it = bookmarks.find(name);
    if (it == bookmarks.end()) {
        return false;
    }
    
    return changeDirectory(it->second);
}

void FileManager::listBookmarks() const {
    std::cout << "\n=== Bookmarks ===\n";
    for (const auto& [name, path] : bookmarks) {
        std::cout << std::left << std::setw(20) << name 
                  << " -> " << path << "\n";
    }
}

// ==================== Терминал ====================

void FileManager::initializeTerminal() {
    #ifdef _WIN32
        // Устанавливаем кодировку UTF-8
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        
        // Включаем поддержку цветов
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD consoleMode;
        GetConsoleMode(hConsole, &consoleMode);
        consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, consoleMode);
    #endif
}

void FileManager::cleanupTerminal() {
    // Очистка ресурсов терминала
}

void FileManager::clearScreen() const {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// ==================== Статистика ====================

DirectoryStats FileManager::getDirectoryStats() const {
    DirectoryStats stats;
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(currentPath)) {
            try {
                if (fs::is_directory(entry)) {
                    stats.directoryCount++;
                } else if (fs::is_regular_file(entry)) {
                    stats.fileCount++;
                    stats.totalSize += fs::file_size(entry);
                    
                    std::string ext = entry.path().extension().string();
                    if (!ext.empty()) {
                        stats.fileTypes[ext]++;
                    }
                }
            } catch (...) {
                stats.errorCount++;
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error calculating stats: " << e.what() << "\n";
    }
    
    return stats;
}

void FileManager::displayStats() const {
    DirectoryStats stats = getDirectoryStats();
    
    std::cout << "\n=== Directory Statistics ===\n";
    std::cout << "Path: " << currentPath.string() << "\n";
    std::cout << "Directories: " << stats.directoryCount << "\n";
    std::cout << "Files: " << stats.fileCount << "\n";
    std::cout << "Total Size: " << formatSize(stats.totalSize) << "\n";
    
    if (!stats.fileTypes.empty()) {
        std::cout << "\nFile Types:\n";
        for (const auto& [ext, count] : stats.fileTypes) {
            std::cout << "  " << ext << ": " << count << "\n";
        }
    }
    
    if (stats.errorCount > 0) {
        std::cout << "Errors: " << stats.errorCount << "\n";
    }
}