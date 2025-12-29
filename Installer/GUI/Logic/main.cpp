#include <iostream>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include "navigator.hpp"
#include "fileops_loader.hpp"

// Глобальный загрузчик библиотеки
std::unique_ptr<FileOpsLoader> g_fileops_loader;
bool g_library_available = false;

// Инициализация библиотеки fileops
bool init_fileops_library() {
    std::cout << "[DEBUG] Initializing fileops library..." << std::endl;
    
    try {
        g_fileops_loader = std::make_unique<FileOpsLoader>();
        
        // 1. Пробуем переменную окружения
        const char* lib_path = std::getenv("FILEOPS_LIB_PATH");
        if (lib_path && lib_path[0] != '\0') {
            std::cout << "[DEBUG] Trying FILEOPS_LIB_PATH: " << lib_path << std::endl;
            if (g_fileops_loader->loadLibrary(lib_path)) {
                g_library_available = true;
                std::cout << "[DEBUG] Library loaded successfully from FILEOPS_LIB_PATH" << std::endl;
                return true;
            }
        }
        
        // 2. Пробуем стандартный путь
        const char* default_path = "/Core/build/bin/libfileops.so";
        std::cout << "[DEBUG] Trying default path: " << default_path << std::endl;
        if (g_fileops_loader->loadLibrary(default_path)) {
            g_library_available = true;
            std::cout << "[DEBUG] Library loaded successfully from default path" << std::endl;
            return true;
        }
        
        // 3. Пробуем относительный путь
        const char* relative_path = "./libfileops.so";
        std::cout << "[DEBUG] Trying relative path: " << relative_path << std::endl;
        if (g_fileops_loader->loadLibrary(relative_path)) {
            g_library_available = true;
            std::cout << "[DEBUG] Library loaded successfully from relative path" << std::endl;
            return true;
        }
        
        // 4. Пробуем в /usr/local/lib
        const char* system_path = "/usr/local/lib/libfileops.so";
        std::cout << "[DEBUG] Trying system path: " << system_path << std::endl;
        if (g_fileops_loader->loadLibrary(system_path)) {
            g_library_available = true;
            std::cout << "[DEBUG] Library loaded successfully from system path" << std::endl;
            return true;
        }
        
        std::cout << "[WARNING] libfileops.so not found. Using native implementation." << std::endl;
        std::cout << "[INFO] Set FILEOPS_LIB_PATH environment variable to use library" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during library initialization: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception during library initialization" << std::endl;
    }
    
    g_library_available = false;
    return false;
}

// Проверяем, доступна ли библиотека
bool is_library_available() {
    return g_library_available && g_fileops_loader && g_fileops_loader->isLoaded();
}

// Вывести справку
void print_help() {
    std::cout << "File Navigator for n8n - Version 2.0" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: navigator <command> [arguments...]" << std::endl;
    std::cout << std::endl;
    std::cout << "Available commands:" << std::endl;
    std::cout << "  list [path]              List files in directory" << std::endl;
    std::cout << "  cd <path>                Change directory" << std::endl;
    std::cout << "  search <pattern> [-r]    Search files (use -r for recursive)" << std::endl;
    std::cout << "  info <file>              Get file information" << std::endl;
    std::cout << "  mkdir <directory>        Create directory" << std::endl;
    std::cout << "  delete <path>            Delete file or directory" << std::endl;
    std::cout << "  copy <src> <dst>         Copy file or directory" << std::endl;
    std::cout << "  move <src> <dst>         Move/rename file or directory" << std::endl;
    std::cout << "  rename <old> <new>       Rename file or directory" << std::endl;
    std::cout << "  pwd                      Print current directory" << std::endl;
    std::cout << "  diskinfo [path]          Get disk information" << std::endl;
    std::cout << std::endl;
    std::cout << "Environment variables:" << std::endl;
    std::cout << "  FILEOPS_LIB_PATH         Path to libfileops.so" << std::endl;
    std::cout << "  N8N_LOG_PATH             Path for log files" << std::endl;
    std::cout << std::endl;
    std::cout << "Library status: " << (is_library_available() ? "LOADED" : "NOT LOADED") << std::endl;
    if (is_library_available() && g_fileops_loader) {
        std::cout << "Library path: " << g_fileops_loader->getLibraryPath() << std::endl;
    }
}

// Точка входа для n8n
int main(int argc, char* argv[]) {
    // Инициализируем библиотеку
    init_fileops_library();
    
    // Проверка аргументов
    if (argc < 2) {
        print_help();
        return 1;
    }
    
    // Специальные флаги
    std::string command = argv[1];
    
    if (command == "--help" || command == "-h" || command == "help") {
        print_help();
        return 0;
    }
    
    if (command == "--version" || command == "-v") {
        std::cout << "File Navigator for n8n - Version 2.0" << std::endl;
        std::cout << "Library support: " << (is_library_available() ? "ENABLED" : "DISABLED") << std::endl;
        return 0;
    }
    
    if (command == "--lib-status") {
        if (is_library_available()) {
            std::cout << "{\"status\":\"loaded\",\"path\":\"" 
                      << g_fileops_loader->getLibraryPath() << "\"}" << std::endl;
        } else {
            std::cout << "{\"status\":\"not_loaded\"";
            if (g_fileops_loader) {
                std::cout << ",\"error\":\"" << g_fileops_loader->getError() << "\"";
            }
            std::cout << "}" << std::endl;
        }
        return 0;
    }
    
    // Обычные команды
    FileNavigator navigator;
    std::vector<std::string> args;
    
    for (int i = 2; i < argc; i++) {
        args.push_back(argv[i]);
    }
    
    try {
        // Выполняем команду
        std::string result = navigator.execute_command(command, args);
        std::cout << result << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "{\"error\":\"Exception: " << e.what() << "\"}" << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "{\"error\":\"Unknown exception\"}" << std::endl;
        return 1;
    }
}