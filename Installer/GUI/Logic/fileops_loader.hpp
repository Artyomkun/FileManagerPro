#ifndef FILEOPS_LOADER_HPP
#define FILEOPS_LOADER_HPP

#include <string>
#include <vector>
#include <functional>

// Структуры для совместимости
struct FileOpsFileInfo {
    char name[256];
    char path[1024];
    char type[16];
    int isDirectory;
    long long size;
    char modified[32];
    char extension[16];
    int isHidden;
    int isReadOnly;
};

struct FileOpsResult {
    int success;
    char message[512];
    int errorCode;
    void* data;
    size_t dataSize;
};

class FileOpsLoader {
private:
    void* handle;
    bool loaded;
    
    // Указатели на функции
    std::function<FileOpsResult(const char*, const char*)> list_files_func;
    std::function<FileOpsResult(const char*, const char*, const char*)> copy_file_func;
    std::function<int(const char*)> file_exists_func;
    std::function<int(const char*)> is_directory_func;
    
    // Запрещаем копирование
    FileOpsLoader(const FileOpsLoader&) = delete;
    FileOpsLoader& operator=(const FileOpsLoader&) = delete;
    
public:
    FileOpsLoader();
    ~FileOpsLoader();
    
    // Перемещение разрешено
    FileOpsLoader(FileOpsLoader&& other) noexcept;
    FileOpsLoader& operator=(FileOpsLoader&& other) noexcept;
    
    bool loadLibrary(const std::string& library_path = "");
    bool isLoaded() const;
    
    // Обертки для функций
    FileOpsResult listFiles(const std::string& path, const std::string& options = "");
    FileOpsResult copyFile(const std::string& src, const std::string& dst, 
                          const std::string& options = "");
    bool fileExists(const std::string& path);
    bool isDirectory(const std::string& path);
    
    // Дополнительные методы
    std::string getLibraryPath() const;
    std::string getError() const;
    
private:
    void* getFunction(const std::string& name);
    std::string last_error;
};

#endif // FILEOPS_LOADER_HPP