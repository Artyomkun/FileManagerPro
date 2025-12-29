/**
 * FileManagerPro - Simple Project Template
 * File: main.cpp
 * Description: Minimal file manager for educational purposes
 * Requirements: C++11 or higher
 * Last Modified: 2024-01-15
 */

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <chrono>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <pwd.h>
#endif

namespace fs = std::filesystem;

// ==================== Simple Data Structures ====================

struct FileEntry {
    std::string name;
    std::string type;
    long long size;
    std::string modified;
    bool isDirectory;
};

struct AppConfig {
    bool showHidden = false;
    bool showDetails = false;
    std::string currentPath;
    int sortBy = 0; // 0=name, 1=size, 2=date
    bool sortDesc = false;
};

// ==================== Utility Functions ====================

std::string getCurrentDirectory() {
    return fs::current_path().string();
}

bool changeDirectory(const std::string& path) {
    try {
        if (!fs::exists(path)) {
            std::cout << "Error: Path does not exist\n";
            return false;
        }
        
        if (!fs::is_directory(path)) {
            std::cout << "Error: Not a directory\n";
            return false;
        }
        
        fs::current_path(path);
        return true;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return false;
    }
}

std::string formatSize(long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    std::stringstream ss;
    if (unitIndex == 0) {
        ss << bytes << " " << units[unitIndex];
    } else {
        ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    }
    
    return ss.str();
}

std::string formatTime(time_t time) {
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", std::localtime(&time));
    return std::string(buffer);
}

// ==================== File Operations ====================

std::vector<FileEntry> listFiles(const AppConfig& config) {
    std::vector<FileEntry> files;
    
    try {
        for (const auto& entry : fs::directory_iterator(config.currentPath)) {
            FileEntry file;
            file.name = entry.path().filename().string();
            
            // Skip hidden files if not shown
            if (!config.showHidden && file.name[0] == '.') {
                continue;
            }
            
            file.isDirectory = entry.is_directory();
            file.type = file.isDirectory ? "DIR" : "FILE";
            
            if (!file.isDirectory && entry.is_regular_file()) {
                file.size = entry.file_size();
            } else {
                file.size = 0;
            }
            
            auto ftime = entry.last_write_time();
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
            file.modified = formatTime(cftime);
            
            files.push_back(file);
        }
    } catch (const std::exception& e) {
        std::cout << "Error listing files: " << e.what() << "\n";
    }
    
    // Sort files
    std::sort(files.begin(), files.end(), [&config](const FileEntry& a, const FileEntry& b) {
        // Directories first
        if (a.isDirectory != b.isDirectory) {
            return a.isDirectory > b.isDirectory;
        }
        
        bool result = false;
        switch (config.sortBy) {
            case 0: // Name
                result = a.name < b.name;
                break;
            case 1: // Size
                result = a.size < b.size;
                break;
            case 2: // Date
                result = a.modified < b.modified;
                break;
        }
        
        return config.sortDesc ? !result : result;
    });
    
    return files;
}

void displayFiles(const std::vector<FileEntry>& files, const AppConfig& config) {
    if (files.empty()) {
        std::cout << "Directory is empty.\n";
        return;
    }
    
    if (config.showDetails) {
        // Detailed view
        std::cout << std::left 
                  << std::setw(30) << "Name"
                  << std::setw(8) << "Type"
                  << std::setw(12) << "Size"
                  << "Modified\n";
        std::cout << std::string(70, '-') << "\n";
        
        for (const auto& file : files) {
            std::cout << std::left 
                      << std::setw(30) << (file.name.length() > 28 ? 
                                           file.name.substr(0, 25) + "..." : file.name)
                      << std::setw(8) << file.type
                      << std::setw(12) << (file.isDirectory ? "<DIR>" : formatSize(file.size))
                      << file.modified << "\n";
        }
    } else {
        // Simple view
        std::cout << "Contents of " << config.currentPath << ":\n";
        std::cout << std::string(50, '-') << "\n";
        
        for (const auto& file : files) {
            std::cout << (file.isDirectory ? "[DIR]  " : "[FILE] ")
                      << file.name;
            
            if (!file.isDirectory && file.size > 0) {
                std::cout << " (" << formatSize(file.size) << ")";
            }
            
            std::cout << "\n";
        }
    }
    
    std::cout << "\nTotal: " << files.size() << " items\n";
}

void showFileInfo(const std::string& filename) {
    try {
        fs::path filepath = fs::current_path() / filename;
        
        if (!fs::exists(filepath)) {
            std::cout << "File not found: " << filename << "\n";
            return;
        }
        
        std::cout << "\n=== File Information ===\n";
        std::cout << "Name: " << filepath.filename().string() << "\n";
        std::cout << "Path: " << filepath.string() << "\n";
        std::cout << "Type: " << (fs::is_directory(filepath) ? "Directory" : "File") << "\n";
        
        if (fs::is_regular_file(filepath)) {
            std::cout << "Size: " << formatSize(fs::file_size(filepath)) << "\n";
            std::cout << "Extension: " << filepath.extension().string() << "\n";
        }
        
        auto ftime = fs::last_write_time(filepath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
        std::cout << "Modified: " << formatTime(cftime) << "\n";
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
}

bool createDirectory(const std::string& dirname) {
    try {
        if (fs::exists(dirname)) {
            std::cout << "Error: Directory already exists\n";
            return false;
        }
        
        return fs::create_directory(dirname);
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return false;
    }
}

bool deleteFile(const std::string& filename) {
    try {
        if (!fs::exists(filename)) {
            std::cout << "Error: File not found\n";
            return false;
        }
        
        std::cout << "Delete " << filename << "? (y/n): ";
        char confirm;
        std::cin >> confirm;
        std::cin.ignore();
        
        if (confirm == 'y' || confirm == 'Y') {
            if (fs::is_directory(filename)) {
                return fs::remove_all(filename) > 0;
            } else {
                return fs::remove(filename);
            }
        }
        
        return false;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
        return false;
    }
}

// ==================== Command Processing ====================

void printHelp() {
    std::cout << "\n=== Simple File Manager Commands ===\n\n";
    std::cout << "  cd [path]      - Change directory\n";
    std::cout << "  ls             - List files\n";
    std::cout << "  ls -a          - List all files (including hidden)\n";
    std::cout << "  ls -l          - Detailed listing\n";
    std::cout << "  info [file]    - Show file information\n";
    std::cout << "  mkdir [name]   - Create directory\n";
    std::cout << "  rm [file]      - Delete file/directory\n";
    std::cout << "  pwd            - Show current directory\n";
    std::cout << "  clear          - Clear screen\n";
    std::cout << "  help           - Show this help\n";
    std::cout << "  exit           - Exit program\n\n";
    
    std::cout << "Examples:\n";
    std::cout << "  cd ..                   # Go to parent directory\n";
    std::cout << "  cd /home/user/Documents # Go to specific directory\n";
    std::cout << "  ls -la                  # List all files with details\n";
    std::cout << "  info report.txt         # Show file information\n";
    std::cout << "  mkdir new_folder        # Create new directory\n";
}

std::vector<std::string> parseCommand(const std::string& input) {
    std::vector<std::string> args;
    std::stringstream ss(input);
    std::string token;
    
    while (ss >> token) {
        args.push_back(token);
    }
    
    return args;
}

void processCommand(const std::vector<std::string>& args, AppConfig& config) {
    if (args.empty()) return;
    
    std::string command = args[0];
    
    if (command == "cd") {
        if (args.size() > 1) {
            changeDirectory(args[1]);
        } else {
            // Go to home directory
#ifdef _WIN32
            changeDirectory(getenv("USERPROFILE"));
#else
            changeDirectory(getenv("HOME"));
#endif
        }
        config.currentPath = getCurrentDirectory();
        
    } else if (command == "ls" || command == "dir") {
        // Update config based on flags
        config.showHidden = false;
        config.showDetails = false;
        
        for (size_t i = 1; i < args.size(); i++) {
            if (args[i] == "-a" || args[i] == "--all") {
                config.showHidden = true;
            } else if (args[i] == "-l" || args[i] == "--long") {
                config.showDetails = true;
            } else if (args[i] == "-la" || args[i] == "-al") {
                config.showHidden = true;
                config.showDetails = true;
            }
        }
        
        auto files = listFiles(config);
        displayFiles(files, config);
        
    } else if (command == "info") {
        if (args.size() > 1) {
            showFileInfo(args[1]);
        } else {
            std::cout << "Usage: info <filename>\n";
        }
        
    } else if (command == "mkdir") {
        if (args.size() > 1) {
            if (createDirectory(args[1])) {
                std::cout << "Directory created: " << args[1] << "\n";
            }
        } else {
            std::cout << "Usage: mkdir <dirname>\n";
        }
        
    } else if (command == "rm") {
        if (args.size() > 1) {
            deleteFile(args[1]);
        } else {
            std::cout << "Usage: rm <filename>\n";
        }
        
    } else if (command == "pwd") {
        std::cout << "Current directory: " << config.currentPath << "\n";
        
    } else if (command == "clear" || command == "cls") {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
        
    } else if (command == "help" || command == "?") {
        printHelp();
        
    } else if (command == "exit" || command == "quit") {
        std::cout << "Goodbye!\n";
        exit(0);
        
    } else {
        std::cout << "Unknown command: " << command << "\n";
        std::cout << "Type 'help' for available commands\n";
    }
}

// ==================== Main Function ====================

int main(int argc, char* argv[]) {
    AppConfig config;
    config.currentPath = getCurrentDirectory();
    
    std::cout << "========================================\n";
    std::cout << "  Simple File Manager v1.0\n";
    std::cout << "  Type 'help' for commands\n";
    std::cout << "========================================\n\n";
    
    // Handle command line arguments
    if (argc > 1) {
        std::string startPath = argv[1];
        if (changeDirectory(startPath)) {
            config.currentPath = getCurrentDirectory();
        }
    }
    
    // Main command loop
    while (true) {
        try {
            // Show prompt
            std::cout << "\n[" << config.currentPath << "]$ ";
            
            // Get user input
            std::string input;
            std::getline(std::cin, input);
            
            // Handle EOF (Ctrl+D or Ctrl+Z)
            if (std::cin.eof()) {
                std::cout << "\nGoodbye!\n";
                break;
            }
            
            // Parse and process command
            auto args = parseCommand(input);
            if (!args.empty()) {
                processCommand(args, config);
            }
            
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    
    return 0;
}