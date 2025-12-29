#include "FileManager.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <memory>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <termios.h>
    #include <unistd.h>
    #include <readline/readline.h>
    #include <readline/history.h>
#endif

// ==================== Константы и утилиты ====================

const std::string VERSION = "1.0.0";
const std::string APP_NAME = "FileManagerPro";

// ANSI цветовые коды
namespace Colors {
    #ifdef _WIN32
        // Windows цветовые коды
        const std::string RESET = "";
        const std::string RED = "";
        const std::string GREEN = "";
        const std::string YELLOW = "";
        const std::string BLUE = "";
        const std::string MAGENTA = "";
        const std::string CYAN = "";
        const std::string BOLD = "";
    #else
        // ANSI escape codes
        const std::string RESET = "\033[0m";
        const std::string RED = "\033[31m";
        const std::string GREEN = "\033[32m";
        const std::string YELLOW = "\033[33m";
        const std::string BLUE = "\033[34m";
        const std::string MAGENTA = "\033[35m";
        const std::string CYAN = "\033[36m";
        const std::string BOLD = "\033[1m";
    #endif
}

// ==================== Прототипы функций ====================

void printWelcome();
void printHelp();
void printPrompt(const FileManager& fm);
std::vector<std::string> parseCommand(const std::string& input);
void executeCommand(FileManager& fm, const std::vector<std::string>& args);
void showDirectoryContents(FileManager& fm, const std::vector<std::string>& args);
void handleCdCommand(FileManager& fm, const std::vector<std::string>& args);
void handleFileInfoCommand(FileManager& fm, const std::vector<std::string>& args);
void handleFileOperationCommand(FileManager& fm, const std::vector<std::string>& args, const std::string& operation);
void handleSearchCommand(FileManager& fm, const std::vector<std::string>& args);
void handleBookmarkCommand(FileManager& fm, const std::vector<std::string>& args);
void handleConfigCommand(FileManager& fm, const std::vector<std::string>& args);
void handleTreeCommand(FileManager& fm, const std::vector<std::string>& args);
void handleStatsCommand(FileManager& fm, const std::vector<std::string>& args);
void showInteractiveMenu();
void clearScreen();
char getKeyPress();
bool confirmAction(const std::string& message);

// ==================== Главная функция ====================

int main(int argc, char* argv[]) {
    // Инициализация
    std::unique_ptr<FileManager> fm;
    
    try {
        // Обработка аргументов командной строки
        if (argc > 1) {
            std::string startPath = argv[1];
            fm = std::make_unique<FileManager>(startPath);
            std::cout << "Started in: " << startPath << "\n";
        } else {
            fm = std::make_unique<FileManager>();
        }
        
        // Устанавливаем callback-и
        fm->setDirectoryChangeCallback([](const std::string& oldPath, const std::string& newPath) {
            std::cout << Colors::CYAN << "Changed directory: " 
                      << oldPath << " -> " << newPath << Colors::RESET << "\n";
        });
        
        fm->setFileOperationCallback([](const std::string& operation, 
                                       const std::string& filePath, 
                                       bool success) {
            std::string color = success ? Colors::GREEN : Colors::RED;
            std::cout << color << operation << ": " << filePath 
                      << " - " << (success ? "SUCCESS" : "FAILED") 
                      << Colors::RESET << "\n";
        });
        
        // Печатаем приветствие
        printWelcome();
        
        // Основной цикл
        bool running = true;
        std::string input;
        
        while (running) {
            try {
                // Показываем подсказку
                printPrompt(*fm);
                
                // Получаем ввод пользователя
                #ifdef _WIN32
                    std::cout << Colors::GREEN << ">> " << Colors::RESET;
                    std::getline(std::cin, input);
                    
                    if (std::cin.eof()) {
                        running = false;
                        break;
                    }
                #else
                    char* line = readline((Colors::GREEN + ">> " + Colors::RESET).c_str());
                    if (!line) {
                        running = false;
                        break;
                    }
                    input = line;
                    free(line);
                    
                    if (!input.empty()) {
                        add_history(input.c_str());
                    }
                #endif
                
                // Парсим команду
                auto args = parseCommand(input);
                if (args.empty()) {
                    continue;
                }
                
                // Выполняем команду
                std::string command = args[0];
                std::transform(command.begin(), command.end(), command.begin(), ::tolower);
                
                if (command == "exit" || command == "quit") {
                    if (confirmAction("Are you sure you want to exit?")) {
                        running = false;
                    }
                } else if (command == "help" || command == "?") {
                    printHelp();
                } else if (command == "clear" || command == "cls") {
                    clearScreen();
                } else if (command == "menu") {
                    showInteractiveMenu();
                } else {
                    executeCommand(*fm, args);
                }
                
            } catch (const std::exception& e) {
                std::cerr << Colors::RED << "Error: " << e.what() 
                          << Colors::RESET << "\n";
            }
        }
        
        std::cout << Colors::YELLOW << "\nGoodbye! Thanks for using FileManagerPro.\n" 
                  << Colors::RESET;
        
    } catch (const std::exception& e) {
        std::cerr << Colors::RED << "Fatal error: " << e.what() 
                  << Colors::RESET << "\n";
        return 1;
    }
    
    return 0;
}

// ==================== Функции интерфейса ====================

void printWelcome() {
    clearScreen();
    
    std::cout << Colors::CYAN << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║" << std::setw(53) << "║\n";
    std::cout << "║      " << Colors::BOLD << Colors::YELLOW << "FileManagerPro v" << VERSION 
              << Colors::RESET << Colors::CYAN << " - C++ Edition      ║\n";
    std::cout << "║" << std::setw(53) << "║\n";
    std::cout << "║     Advanced File System Management Tool     ║\n";
    std::cout << "║" << std::setw(53) << "║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";
    
    std::cout << Colors::RESET << "Type 'help' for available commands\n";
    std::cout << "Type 'menu' for interactive menu\n";
    std::cout << "Type 'exit' to quit\n\n";
}

void printHelp() {
    std::cout << Colors::BOLD << Colors::YELLOW << "\n=== Available Commands ===\n\n" << Colors::RESET;
    
    // Navigation
    std::cout << Colors::CYAN << "Navigation:\n" << Colors::RESET;
    std::cout << "  cd [path]        - Change directory\n";
    std::cout << "  cd ..            - Go to parent directory\n";
    std::cout << "  cd ~             - Go to home directory\n";
    std::cout << "  cd /             - Go to root directory\n";
    std::cout << "  pwd              - Print working directory\n";
    std::cout << "  back             - Go back in history\n";
    std::cout << "  history          - Show navigation history\n\n";
    
    // File listing
    std::cout << Colors::CYAN << "File Listing:\n" << Colors::RESET;
    std::cout << "  ls               - List files (simple)\n";
    std::cout << "  ls -a            - List all files (including hidden)\n";
    std::cout << "  ls -l            - Detailed listing\n";
    std::cout << "  ls -la           - Detailed listing with hidden\n";
    std::cout << "  tree             - Show directory tree\n";
    std::cout << "  grid             - Show files in grid view\n\n";
    
    // File operations
    std::cout << Colors::CYAN << "File Operations:\n" << Colors::RESET;
    std::cout << "  mkdir [name]     - Create directory\n";
    std::cout << "  touch [name]     - Create empty file\n";
    std::cout << "  rm [file]        - Delete file\n";
    std::cout << "  rmdir [dir]      - Delete directory\n";
    std::cout << "  cp [src] [dst]   - Copy file\n";
    std::cout << "  mv [src] [dst]   - Move/rename file\n";
    std::cout << "  rename [old] [new] - Rename file\n";
    std::cout << "  info [file]      - Show file information\n\n";
    
    // Search
    std::cout << Colors::CYAN << "Search:\n" << Colors::RESET;
    std::cout << "  find [pattern]   - Search files by name\n";
    std::cout << "  find -r [pattern]- Recursive search\n";
    std::cout << "  grep [text]      - Search text in files\n\n";
    
    // Bookmarks
    std::cout << Colors::CYAN << "Bookmarks:\n" << Colors::RESET;
    std::cout << "  bookmark add [name] [path] - Add bookmark\n";
    std::cout << "  bookmark list              - List bookmarks\n";
    std::cout << "  bookmark goto [name]       - Go to bookmark\n";
    std::cout << "  bookmark rm [name]         - Remove bookmark\n\n";
    
    // Statistics
    std::cout << Colors::CYAN << "Statistics:\n" << Colors::RESET;
    std::cout << "  stats            - Show directory statistics\n";
    std::cout << "  size [path]      - Calculate directory size\n";
    std::cout << "  du               - Disk usage\n\n";
    
    // System
    std::cout << Colors::CYAN << "System:\n" << Colors::RESET;
    std::cout << "  clear/cls        - Clear screen\n";
    std::cout << "  help/?           - Show this help\n";
    std::cout << "  menu             - Interactive menu\n";
    std::cout << "  version          - Show version\n";
    std::cout << "  exit/quit        - Exit program\n";
    
    std::cout << Colors::BOLD << Colors::YELLOW << "\n=== Examples ===\n" << Colors::RESET;
    std::cout << "  ls -la                         # List all files with details\n";
    std::cout << "  cp report.txt backup/          # Copy file to backup directory\n";
    std::cout << "  find *.txt                     # Find all text files\n";
    std::cout << "  bookmark add docs ~/Documents  # Bookmark Documents folder\n";
    std::cout << "  tree                           # Show directory tree\n\n";
}

void printPrompt(const FileManager& fm) {
    std::string currentPath = fm.getCurrentPath();
    std::string homePath = fm.getUserHome();
    
    // Сокращаем путь для красоты
    if (currentPath.find(homePath) == 0) {
        currentPath = "~" + currentPath.substr(homePath.length());
    }
    
    std::cout << Colors::BOLD << Colors::GREEN << "\n[" << currentPath << "]" 
              << Colors::RESET;
}

std::vector<std::string> parseCommand(const std::string& input) {
    std::vector<std::string> args;
    std::stringstream ss(input);
    std::string token;
    
    while (ss >> std::quoted(token)) {
        args.push_back(token);
    }
    
    return args;
}

void executeCommand(FileManager& fm, const std::vector<std::string>& args) {
    if (args.empty()) return;
    
    std::string command = args[0];
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);
    
    // Навигация
    if (command == "cd") {
        handleCdCommand(fm, args);
    } else if (command == "pwd") {
        std::cout << Colors::CYAN << "Current directory: " 
                  << fm.getCurrentPath() << Colors::RESET << "\n";
    } else if (command == "back") {
        // Навигация назад по истории
        auto history = fm.getHistory();
        if (history.size() > 1) {
            std::string prevDir = history[history.size() - 2];
            fm.changeDirectory(prevDir);
        }
        
    // Просмотр файлов
    } else if (command == "ls" || command == "dir") {
        showDirectoryContents(fm, args);
    } else if (command == "tree") {
        handleTreeCommand(fm, args);
    } else if (command == "grid") {
        auto files = fm.listFiles(false, SortBy::NAME, false);
        fm.displayFiles(files, DisplayMode::GRID);
        
    // Информация о файлах
    } else if (command == "info") {
        handleFileInfoCommand(fm, args);
        
    // Операции с файлами
    } else if (command == "mkdir") {
        handleFileOperationCommand(fm, args, "mkdir");
    } else if (command == "touch") {
        handleFileOperationCommand(fm, args, "touch");
    } else if (command == "rm" || command == "del") {
        handleFileOperationCommand(fm, args, "rm");
    } else if (command == "rmdir") {
        handleFileOperationCommand(fm, args, "rmdir");
    } else if (command == "cp" || command == "copy") {
        handleFileOperationCommand(fm, args, "cp");
    } else if (command == "mv" || command == "move") {
        handleFileOperationCommand(fm, args, "mv");
    } else if (command == "rename") {
        handleFileOperationCommand(fm, args, "rename");
        
    // Поиск
    } else if (command == "find" || command == "search") {
        handleSearchCommand(fm, args);
    } else if (command == "grep") {
        if (args.size() > 1) {
            std::string text = args[1];
            std::cout << "Searching for text: " << text << "\n";
            // Реализация поиска по содержимому файлов
        }
        
    // Закладки
    } else if (command == "bookmark") {
        handleBookmarkCommand(fm, args);
        
    // Статистика
    } else if (command == "stats") {
        handleStatsCommand(fm, args);
    } else if (command == "size" || command == "du") {
        if (args.size() > 1) {
            uintmax_t size = fm.calculateDirectorySize(args[1]);
            std::cout << "Size: " << FileManager::formatSize(size) << "\n";
        } else {
            uintmax_t size = fm.calculateDirectorySize(fm.getCurrentPath());
            std::cout << "Current directory size: " 
                      << FileManager::formatSize(size) << "\n";
        }
        
    // Системные команды
    } else if (command == "version") {
        std::cout << Colors::YELLOW << APP_NAME << " v" << VERSION 
                  << Colors::RESET << "\n";
    } else if (command == "history") {
        auto history = fm.getHistory();
        std::cout << Colors::CYAN << "\n=== Navigation History ===\n" 
                  << Colors::RESET;
        for (size_t i = 0; i < history.size(); i++) {
            std::cout << i << ": " << history[i] << "\n";
        }
    } else {
        std::cout << Colors::RED << "Unknown command: " << command 
                  << Colors::RESET << "\n";
        std::cout << "Type 'help' for available commands\n";
    }
}

// ==================== Обработчики команд ====================

void showDirectoryContents(FileManager& fm, const std::vector<std::string>& args) {
    bool showHidden = false;
    bool detailed = false;
    SortBy sortBy = SortBy::NAME;
    bool reverse = false;
    
    // Парсим флаги
    for (size_t i = 1; i < args.size(); i++) {
        std::string arg = args[i];
        if (arg == "-a" || arg == "--all") {
            showHidden = true;
        } else if (arg == "-l" || arg == "--long") {
            detailed = true;
        } else if (arg == "-r" || arg == "--reverse") {
            reverse = true;
        } else if (arg == "-s" || arg == "--size") {
            sortBy = SortBy::SIZE;
        } else if (arg == "-t" || arg == "--time") {
            sortBy = SortBy::DATE;
        } else if (arg == "-e" || arg == "--extension") {
            sortBy = SortBy::TYPE;
        }
    }
    
    // Получаем файлы
    auto files = fm.listFiles(showHidden, sortBy, reverse);
    
    // Отображаем в нужном формате
    if (detailed) {
        fm.displayFiles(files, DisplayMode::DETAILS);
    } else {
        fm.displayFiles(files, DisplayMode::LIST);
    }
}

void handleCdCommand(FileManager& fm, const std::vector<std::string>& args) {
    if (args.size() > 1) {
        std::string path = args[1];
        if (!fm.changeDirectory(path)) {
            std::cout << Colors::RED << "Failed to change directory to: " 
                      << path << Colors::RESET << "\n";
        }
    } else {
        // Без аргументов - идем домой
        fm.goToHome();
    }
}

void handleFileInfoCommand(FileManager& fm, const std::vector<std::string>& args) {
    if (args.size() > 1) {
        std::string filename = args[1];
        auto info = fm.getFileInfo(filename);
        fm.displayFileInfo(info);
    } else {
        std::cout << Colors::RED << "Usage: info <filename>" << Colors::RESET << "\n";
    }
}

void handleFileOperationCommand(FileManager& fm, const std::vector<std::string>& args, 
                               const std::string& operation) {
    if (args.size() < 2) {
        std::cout << Colors::RED << "Usage: " << operation << " <target>" 
                  << Colors::RESET << "\n";
        return;
    }
    
    bool success = false;
    
    if (operation == "mkdir") {
        success = fm.createDirectory(args[1]);
    } else if (operation == "touch") {
        success = fm.createFile(args[1]);
    } else if (operation == "rm" || operation == "del") {
        std::string filename = args[1];
        if (confirmAction("Delete file '" + filename + "'?")) {
            success = fm.deleteFile(filename, false);
        }
    } else if (operation == "rmdir") {
        std::string dirname = args[1];
        if (confirmAction("Delete directory '" + dirname + "'?")) {
            // Для директорий нужна рекурсивная функция (реализуется в FileManager)
            std::cout << "Directory deletion not implemented yet\n";
            success = false;
        }
    } else if (operation == "cp" || operation == "copy") {
        if (args.size() > 2) {
            success = fm.copyFile(args[1], args[2], true);
        } else {
            std::cout << Colors::RED << "Usage: cp <source> <destination>" 
                      << Colors::RESET << "\n";
        }
    } else if (operation == "mv" || operation == "move") {
        if (args.size() > 2) {
            success = fm.moveFile(args[1], args[2]);
        } else {
            std::cout << Colors::RED << "Usage: mv <source> <destination>" 
                      << Colors::RESET << "\n";
        }
    } else if (operation == "rename") {
        if (args.size() > 2) {
            success = fm.rename(args[1], args[2]);
        } else {
            std::cout << Colors::RED << "Usage: rename <oldname> <newname>" 
                      << Colors::RESET << "\n";
        }
    }
    
    if (success) {
        std::cout << Colors::GREEN << operation << " completed successfully" 
                  << Colors::RESET << "\n";
    }
}

void handleSearchCommand(FileManager& fm, const std::vector<std::string>& args) {
    if (args.size() > 1) {
        std::string pattern = args[1];
        bool recursive = false;
        
        // Проверяем флаги
        for (size_t i = 1; i < args.size(); i++) {
            if (args[i] == "-r" || args[i] == "--recursive") {
                recursive = true;
                if (i + 1 < args.size()) {
                    pattern = args[i + 1];
                }
                break;
            }
        }
        
        std::cout << Colors::CYAN << "Searching for: " << pattern 
                  << (recursive ? " (recursive)" : "") << Colors::RESET << "\n";
        
        auto results = fm.searchFiles(pattern, recursive);
        
        if (results.empty()) {
            std::cout << Colors::YELLOW << "No files found." << Colors::RESET << "\n";
        } else {
            std::cout << Colors::GREEN << "Found " << results.size() 
                      << " files:" << Colors::RESET << "\n";
            for (const auto& result : results) {
                std::cout << "  " << result << "\n";
            }
        }
    } else {
        std::cout << Colors::RED << "Usage: find <pattern>" << Colors::RESET << "\n";
    }
}

void handleBookmarkCommand(FileManager& fm, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << Colors::RED << "Usage: bookmark <command> [args]" << Colors::RESET << "\n";
        return;
    }
    
    std::string subcommand = args[1];
    
    if (subcommand == "add" && args.size() > 3) {
        std::string name = args[2];
        std::string path = args[3];
        if (fm.addBookmark(name, path)) {
            std::cout << Colors::GREEN << "Bookmark added: " << name 
                      << " -> " << path << Colors::RESET << "\n";
        } else {
            std::cout << Colors::RED << "Failed to add bookmark" << Colors::RESET << "\n";
        }
    } else if (subcommand == "list") {
        fm.listBookmarks();
    } else if (subcommand == "goto" && args.size() > 2) {
        std::string name = args[2];
        if (!fm.goToBookmark(name)) {
            std::cout << Colors::RED << "Bookmark not found: " << name 
                      << Colors::RESET << "\n";
        }
    } else if (subcommand == "rm" && args.size() > 2) {
        // Удаление закладки (нужно добавить метод в FileManager)
        std::cout << "Bookmark removal not implemented yet\n";
    } else {
        std::cout << Colors::RED << "Invalid bookmark command" << Colors::RESET << "\n";
    }
}

void handleTreeCommand(FileManager& fm, const std::vector<std::string>& args) {
    int depth = 3; // Глубина по умолчанию
    if (args.size() > 1) {
        try {
            depth = std::stoi(args[1]);
        } catch (...) {
            std::cout << Colors::RED << "Invalid depth value" << Colors::RESET << "\n";
            return;
        }
    }
    
    fm.displayTree();
}

void handleStatsCommand(FileManager& fm, const std::vector<std::string>& args) {
    fm.displayStats();
}

// ==================== Интерактивное меню ====================

void showInteractiveMenu() {
    clearScreen();
    
    std::cout << Colors::CYAN << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║" << std::setw(53) << "║\n";
    std::cout << "║        " << Colors::BOLD << Colors::YELLOW << "Interactive Menu" 
              << Colors::RESET << Colors::CYAN << "        ║\n";
    std::cout << "║" << std::setw(53) << "║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";
    
    std::cout << Colors::RESET << "Select an option:\n\n";
    std::cout << Colors::GREEN << "1." << Colors::RESET << " Navigate File System\n";
    std::cout << Colors::GREEN << "2." << Colors::RESET << " View Files\n";
    std::cout << Colors::GREEN << "3." << Colors::RESET << " File Operations\n";
    std::cout << Colors::GREEN << "4." << Colors::RESET << " Search Files\n";
    std::cout << Colors::GREEN << "5." << Colors::RESET << " Manage Bookmarks\n";
    std::cout << Colors::GREEN << "6." << Colors::RESET << " Statistics\n";
    std::cout << Colors::GREEN << "7." << Colors::RESET << " Settings\n";
    std::cout << Colors::GREEN << "8." << Colors::RESET << " Help\n";
    std::cout << Colors::GREEN << "9." << Colors::RESET << " Return to Command Line\n";
    std::cout << Colors::GREEN << "0." << Colors::RESET << " Exit\n\n";
    
    std::cout << Colors::CYAN << "Enter choice (0-9): " << Colors::RESET;
    
    char choice = getKeyPress();
    std::cout << choice << "\n";
    
    switch (choice) {
        case '1':
            std::cout << "\nNavigation features:\n";
            std::cout << "- Change directory (cd)\n";
            std::cout << "- Navigation history\n";
            std::cout << "- Quick access to common directories\n";
            break;
        case '2':
            std::cout << "\nFile viewing options:\n";
            std::cout << "- List view (ls)\n";
            std::cout << "- Detailed view (ls -l)\n";
            std::cout << "- Grid view (grid)\n";
            std::cout << "- Tree view (tree)\n";
            break;
        case '3':
            std::cout << "\nFile operations:\n";
            std::cout << "- Create files/directories\n";
            std::cout << "- Copy/Move/Rename\n";
            std::cout << "- Delete files\n";
            std::cout << "- File properties\n";
            break;
        case '4':
            std::cout << "\nSearch capabilities:\n";
            std::cout << "- Search by name (find)\n";
            std::cout << "- Search by content (grep)\n";
            std::cout << "- Advanced search patterns\n";
            break;
        case '5':
            std::cout << "\nBookmark management:\n";
            std::cout << "- Add bookmarks\n";
            std::cout << "- List bookmarks\n";
            std::cout << "- Quick navigation\n";
            break;
        case '6':
            std::cout << "\nStatistics:\n";
            std::cout << "- Directory size\n";
            std::cout << "- File counts\n";
            std::cout << "- Disk usage\n";
            break;
        case '7':
            std::cout << "\nSettings:\n";
            std::cout << "- Display options\n";
            std::cout << "- Behavior settings\n";
            std::cout << "- Configuration\n";
            break;
        case '8':
            printHelp();
            break;
        case '9':
            std::cout << "\nReturning to command line...\n";
            return;
        case '0':
            std::cout << "\nExiting...\n";
            exit(0);
            break;
        default:
            std::cout << Colors::RED << "Invalid choice!" << Colors::RESET << "\n";
    }
    
    std::cout << "\nPress any key to continue...";
    getKeyPress();
    showInteractiveMenu();
}

// ==================== Утилиты ====================

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

char getKeyPress() {
    #ifdef _WIN32
        return _getch();
    #else
        struct termios oldt, newt;
        char ch;
        
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        
        ch = getchar();
        
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
    #endif
}

bool confirmAction(const std::string& message) {
    std::cout << Colors::YELLOW << message << " (y/n): " << Colors::RESET;
    
    char response = getKeyPress();
    std::cout << response << "\n";
    
    return (response == 'y' || response == 'Y');
}