#include "fileops_loader.hpp"
#include <dlfcn.h>
#include <iostream>
#include <cstring>
#include <utility>

FileOpsLoader::FileOpsLoader() 
    : handle(nullptr), loaded(false) {
}

FileOpsLoader::~FileOpsLoader() {
    if (handle) {
        dlclose(handle);
        handle = nullptr;
    }
}

// Конструктор перемещения
FileOpsLoader::FileOpsLoader(FileOpsLoader&& other) noexcept
    : handle(other.handle),
      loaded(other.loaded),
      list_files_func(std::move(other.list_files_func)),
      copy_file_func(std::move(other.copy_file_func)),
      file_exists_func(std::move(other.file_exists_func)),
      is_directory_func(std::move(other.is_directory_func)),
      last_error(std::move(other.last_error)) {
    other.handle = nullptr;
    other.loaded = false;
}

// Оператор перемещения
FileOpsLoader& FileOpsLoader::operator=(FileOpsLoader&& other) noexcept {
    if (this != &other) {
        // Освобождаем текущие ресурсы
        if (handle) {
            dlclose(handle);
        }
        
        // Перемещаем ресурсы
        handle = other.handle;
        loaded = other.loaded;
        list_files_func = std::move(other.list_files_func);
        copy_file_func = std::move(other.copy_file_func);
        file_exists_func = std::move(other.file_exists_func);
        is_directory_func = std::move(other.is_directory_func);
        last_error = std::move(other.last_error);
        
        // Обнуляем у источника
        other.handle = nullptr;
        other.loaded = false;
    }
    return *this;
}

bool FileOpsLoader::loadLibrary(const std::string& library_path) {
    // Освобождаем предыдущую библиотеку если загружена
    if (handle) {
        dlclose(handle);
        handle = nullptr;
        loaded = false;
    }
    
    std::string path = library_path;
    if (path.empty()) {
        // Пробуем стандартные пути
        const char* env_path = getenv("FILEOPS_LIB_PATH");
        if (env_path) {
            path = env_path;
        } else {
            path = "/Core/build/bin/libfileops.so";
        }
    }
    
    // Пробуем загрузить
    handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        last_error = dlerror();
        std::cerr << "Failed to load library " << path << ": " << last_error << std::endl;
        
        // Пробуем относительный путь
        path = "./libfileops.so";
        handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
        if (!handle) {
            last_error = dlerror();
            return false;
        }
    }
    
    // Загружаем функции
    auto list_files_ptr = (FileOpsResult(*)(const char*, const char*))getFunction("list_files");
    auto copy_file_ptr = (FileOpsResult(*)(const char*, const char*, const char*))getFunction("copy_file");
    auto file_exists_ptr = (int(*)(const char*))getFunction("file_exists");
    auto is_directory_ptr = (int(*)(const char*))getFunction("is_directory");
    
    if (!list_files_ptr || !copy_file_ptr || !file_exists_ptr || !is_directory_ptr) {
        last_error = "Failed to load required functions";
        dlclose(handle);
        handle = nullptr;
        return false;
    }
    
    // Сохраняем указатели
    list_files_func = list_files_ptr;
    copy_file_func = copy_file_ptr;
    file_exists_func = file_exists_ptr;
    is_directory_func = is_directory_ptr;
    
    loaded = true;
    return true;
}

void* FileOpsLoader::getFunction(const std::string& name) {
    if (!handle) return nullptr;
    
    void* func = dlsym(handle, name.c_str());
    if (!func) {
        // Пробуем с префиксом
        std::string n8n_name = "n8n_" + name;
        func = dlsym(handle, n8n_name.c_str());
        
        if (!func) {
            // Пробуем альтернативные имена
            if (name == "list_files") {
                func = dlsym(handle, "n8n_list_files");
            } else if (name == "copy_file") {
                func = dlsym(handle, "n8n_copy_file");
            } else if (name == "file_exists") {
                func = dlsym(handle, "n8n_file_exists");
            } else if (name == "is_directory") {
                func = dlsym(handle, "n8n_is_directory");
            }
        }
    }
    return func;
}

bool FileOpsLoader::isLoaded() const {
    return loaded;
}

FileOpsResult FileOpsLoader::listFiles(const std::string& path, const std::string& options) {
    if (!loaded) {
        FileOpsResult result = {0};
        strcpy(result.message, "Library not loaded");
        return result;
    }
    return list_files_func(path.c_str(), options.c_str());
}

FileOpsResult FileOpsLoader::copyFile(const std::string& src, const std::string& dst, 
                                     const std::string& options) {
    if (!loaded) {
        FileOpsResult result = {0};
        strcpy(result.message, "Library not loaded");
        return result;
    }
    return copy_file_func(src.c_str(), dst.c_str(), options.c_str());
}

bool FileOpsLoader::fileExists(const std::string& path) {
    if (!loaded) return false;
    return file_exists_func(path.c_str()) != 0;
}

bool FileOpsLoader::isDirectory(const std::string& path) {
    if (!loaded) return false;
    return is_directory_func(path.c_str()) != 0;
}

std::string FileOpsLoader::getLibraryPath() const {
    if (!handle) return "Not loaded";
    
    Dl_info info;
    if (list_files_func.target<void*>() && 
        dladdr((void*)list_files_func.target<void*>(), &info)) {
        return info.dli_fname;
    }
    return "Unknown";
}

std::string FileOpsLoader::getError() const {
    return last_error;
}