#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <queue>
#include <stack>
#include <ctime>
#include <fstream>
#include <cstring> 
#include <sys/time.h>  
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/statvfs.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <errno.h>

// Простые структуры для n8n
struct FileItem {
    std::string name;
    std::string path;
    std::string type; // "file", "directory" или "symlink"
    long long size;
    std::string modified;
    std::string extension;
    bool isHidden;
    bool isReadOnly;
    std::string permissions;
    std::string owner;
    std::string group;
    std::string symlinkTarget;
};

struct SearchResult {
    std::string name;
    std::string path;
    std::string type;
    long long size;
    std::string modified;
    std::string symlinkTarget;
};

struct DiskInfo {
    unsigned long long total_space;
    unsigned long long free_space;
    unsigned long long available_space;
    unsigned long long used_space;
    double usage_percentage;
    std::string filesystem;
};

class FileNavigator {
private:
    std::string current_path;
    std::vector<std::string> path_history;
    int history_index;
    
    // Функция для проверки директории
    bool is_directory(const std::string& path) {
        struct stat statbuf;
        if (stat(path.c_str(), &statbuf) != 0)
            return false;
        return S_ISDIR(statbuf.st_mode);
    }
    
    // Проверка символической ссылки
    bool is_symlink(const std::string& path) {
        struct stat statbuf;
        if (lstat(path.c_str(), &statbuf) != 0)
            return false;
        return S_ISLNK(statbuf.st_mode);
    }
    
    // Получение строки прав доступа
    std::string get_permissions_string(mode_t mode) {
        std::string perms(10, '-');
        
        perms[0] = S_ISDIR(mode) ? 'd' : S_ISLNK(mode) ? 'l' : 
                   S_ISFIFO(mode) ? 'p' : S_ISSOCK(mode) ? 's' : 
                   S_ISCHR(mode) ? 'c' : S_ISBLK(mode) ? 'b' : '-';
        
        perms[1] = (mode & S_IRUSR) ? 'r' : '-';
        perms[2] = (mode & S_IWUSR) ? 'w' : '-';
        perms[3] = (mode & S_IXUSR) ? 'x' : '-';
        perms[4] = (mode & S_IRGRP) ? 'r' : '-';
        perms[5] = (mode & S_IWGRP) ? 'w' : '-';
        perms[6] = (mode & S_IXGRP) ? 'x' : '-';
        perms[7] = (mode & S_IROTH) ? 'r' : '-';
        perms[8] = (mode & S_IWOTH) ? 'w' : '-';
        perms[9] = (mode & S_IXOTH) ? 'x' : '-';
        
        // Специальные биты
        if (mode & S_ISUID) perms[3] = (mode & S_IXUSR) ? 's' : 'S';
        if (mode & S_ISGID) perms[6] = (mode & S_IXGRP) ? 's' : 'S';
        if (mode & S_ISVTX) perms[9] = (mode & S_IXOTH) ? 't' : 'T';
        
        return perms;
    }
    
    // Преобразование времени в строку
    std::string time_to_string(time_t t) {
        struct tm* tm_info = localtime(&t);
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
        return std::string(buffer);
    }
    
    // Получение содержимого директории
    std::vector<FileItem> get_directory_contents(const std::string& path) {
        std::vector<FileItem> items;
        DIR* dir;
        struct dirent* entry;
        
        dir = opendir(path.c_str());
        if (dir == nullptr) {
            return items;
        }
        
        while ((entry = readdir(dir)) != nullptr) {
            // Пропускаем . и ..
            if (strcmp(entry->d_name, ".") == 0 || 
                strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            FileItem item;
            item.name = entry->d_name;
            
            // Собираем полный путь
            if (path == "/") {
                item.path = path + item.name;
            } else {
                item.path = path + "/" + item.name;
            }
            
            // Получаем информацию о файле/директории через lstat (для симлинков)
            struct stat statbuf;
            if (lstat(item.path.c_str(), &statbuf) != 0) {
                continue; // Пропускаем, если не удалось получить информацию
            }
            
            // Определяем тип
            if (S_ISLNK(statbuf.st_mode)) {
                item.type = "symlink";
                item.size = 0;
                
                // Читаем цель симлинка
                char target[PATH_MAX];
                ssize_t len = readlink(item.path.c_str(), target, sizeof(target) - 1);
                if (len != -1) {
                    target[len] = '\0';
                    item.symlinkTarget = target;
                }
            } else if (S_ISDIR(statbuf.st_mode)) {
                item.type = "directory";
                item.size = 0;
                item.symlinkTarget = "";
            } else {
                item.type = "file";
                item.size = statbuf.st_size;
                item.symlinkTarget = "";
                
                // Получаем расширение
                std::string name = item.name;
                size_t dot_pos = name.find_last_of('.');
                if (dot_pos != std::string::npos) {
                    item.extension = name.substr(dot_pos + 1);
                }
            }
            
            // Время модификации
            item.modified = time_to_string(statbuf.st_mtime);
            
            // Атрибуты
            item.isHidden = (item.name[0] == '.');
            item.isReadOnly = !(statbuf.st_mode & S_IWUSR);
            
            // Права доступа
            item.permissions = get_permissions_string(statbuf.st_mode);
            
            // Владелец и группа
            struct passwd* pwd = getpwuid(statbuf.st_uid);
            struct group* grp = getgrgid(statbuf.st_gid);
            item.owner = pwd ? pwd->pw_name : std::to_string(statbuf.st_uid);
            item.group = grp ? grp->gr_name : std::to_string(statbuf.st_gid);
            
            items.push_back(item);
        }
        
        closedir(dir);
        
        // Сортировка: директории первыми
        std::sort(items.begin(), items.end(), [](const FileItem& a, const FileItem& b) {
            if (a.type == b.type) {
                return a.name < b.name;
            }
            // Порядок: директории, симлинки, файлы
            if (a.type == "directory" && b.type != "directory") return true;
            if (a.type == "symlink" && b.type == "file") return true;
            return false;
        });
        
        return items;
    }
    
    // Форматирование JSON для n8n
    std::string to_json(const std::vector<FileItem>& items) {
        std::ostringstream oss;
        oss << "{\"files\":[";
        
        bool first = true;
        for (const auto& item : items) {
            if (!first) oss << ",";
            
            oss << "{"
                << "\"name\":\"" << escape_json(item.name) << "\","
                << "\"path\":\"" << escape_json(item.path) << "\","
                << "\"type\":\"" << item.type << "\","
                << "\"size\":" << item.size << ","
                << "\"modified\":\"" << item.modified << "\","
                << "\"extension\":\"" << item.extension << "\","
                << "\"isHidden\":" << (item.isHidden ? "true" : "false") << ","
                << "\"isReadOnly\":" << (item.isReadOnly ? "true" : "false") << ","
                << "\"permissions\":\"" << item.permissions << "\","
                << "\"owner\":\"" << item.owner << "\","
                << "\"group\":\"" << item.group << "\","
                << "\"symlinkTarget\":\"" << escape_json(item.symlinkTarget) << "\""
                << "}";
            
            first = false;
        }
        
        oss << "]}";
        return oss.str();
    }
    
    std::string escape_json(const std::string& s) {
        std::ostringstream oss;
        for (char c : s) {
            switch (c) {
                case '"': oss << "\\\""; break;
                case '\\': oss << "\\\\"; break;
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                default: oss << c; break;
            }
        }
        return oss.str();
    }
    
    // Получение абсолютного пути
    std::string get_absolute_path(const std::string& path) {
        if (path.empty()) {
            return current_path;
        }
        
        if (path[0] == '/') {
            return path; // Уже абсолютный путь
        }
        
        if (path == ".") {
            return current_path;
        }
        
        if (path == "..") {
            // Родительская директория
            size_t pos = current_path.find_last_of('/');
            if (pos == 0) {
                return "/"; // Уже в корне
            }
            return current_path.substr(0, pos);
        }
        
        // Относительный путь
        if (current_path == "/") {
            return current_path + path;
        }
        return current_path + "/" + path;
    }
    
public:
    FileNavigator() {
        char buffer[PATH_MAX];
        if (getcwd(buffer, sizeof(buffer))) {
            current_path = buffer;
        } else {
            current_path = "/";
        }
        path_history.push_back(current_path);
        history_index = 0;
    }
    
    // Основной метод для n8n - возвращает JSON
    std::string execute_command(const std::string& command, const std::vector<std::string>& args) {
        if (command == "list") {
            return list_files(args);
        } else if (command == "cd") {
            return change_directory(args);
        } else if (command == "search") {
            return search_files(args);
        } else if (command == "info") {
            return get_info(args);
        } else if (command == "mkdir") {
            return create_directory(args);
        } else if (command == "delete") {
            return delete_item(args);
        } else if (command == "copy") {
            return copy_item(args);
        } else if (command == "move") {
            return move_item(args);
        } else if (command == "rename") {
            return rename_item(args);
        } else if (command == "pwd") {
            return get_current_path();
        } else if (command == "diskinfo") {
            return get_disk_info(args);
        }
        
        return "{\"error\":\"Unknown command: " + command + "\"}";
    }
    
private:
    std::string list_files(const std::vector<std::string>& args) {
        std::string path = current_path;
        if (!args.empty()) {
            path = get_absolute_path(args[0]);
        }
        
        if (!is_directory(path)) {
            return "{\"error\":\"Not a directory: " + path + "\"}";
        }
        
        auto items = get_directory_contents(path);
        return to_json(items);
    }
    
    std::string change_directory(const std::vector<std::string>& args) {
        if (args.empty()) {
            return "{\"error\":\"No path specified\"}";
        }
        
        std::string new_path = get_absolute_path(args[0]);
        
        if (!is_directory(new_path)) {
            return "{\"error\":\"Not a directory: " + new_path + "\"}";
        }
        
        if (chdir(new_path.c_str()) == 0) {
            char buffer[PATH_MAX];
            if (getcwd(buffer, sizeof(buffer))) {
                current_path = buffer;
            }
            
            return "{\"success\":true,\"path\":\"" + current_path + "\"}";
        }
        
        return "{\"error\":\"Failed to change directory: " + std::string(strerror(errno)) + "\"}";
    }
    
    std::string search_files(const std::vector<std::string>& args) {
        if (args.size() < 1) {
            return "{\"error\":\"No search pattern specified\"}";
        }
        
        std::string pattern = args[0];
        bool recursive = (args.size() > 1 && args[1] == "-r");
        std::string start_path = (args.size() > 2) ? get_absolute_path(args[2]) : current_path;
        
        std::vector<SearchResult> results;
        std::queue<std::string> dirs_queue;
        dirs_queue.push(start_path);
        
        while (!dirs_queue.empty()) {
            std::string current_dir = dirs_queue.front();
            dirs_queue.pop();
            
            auto items = get_directory_contents(current_dir);
            
            for (const auto& item : items) {
                // Поиск по имени (простой contains)
                if (item.name.find(pattern) != std::string::npos) {
                    SearchResult result;
                    result.name = item.name;
                    result.path = item.path;
                    result.type = item.type;
                    result.size = item.size;
                    result.modified = item.modified;
                    result.symlinkTarget = item.symlinkTarget;
                    results.push_back(result);
                }
                
                // Если рекурсивно и это директория - добавляем в очередь
                if (recursive && item.type == "directory") {
                    dirs_queue.push(item.path);
                }
            }
            
            if (!recursive) break;
        }
        
        // Формируем JSON ответ
        std::ostringstream oss;
        oss << "{\"results\":[";
        
        bool first = true;
        for (const auto& result : results) {
            if (!first) oss << ",";
            
            oss << "{"
                << "\"name\":\"" << escape_json(result.name) << "\","
                << "\"path\":\"" << escape_json(result.path) << "\","
                << "\"type\":\"" << result.type << "\","
                << "\"size\":" << result.size << ","
                << "\"modified\":\"" << result.modified << "\","
                << "\"symlinkTarget\":\"" << escape_json(result.symlinkTarget) << "\""
                << "}";
            
            first = false;
        }
        
        oss << "],\"count\":" << results.size() << "}";
        return oss.str();
    }
    
    std::string get_info(const std::vector<std::string>& args) {
        if (args.empty()) {
            // Информация о текущей директории
            auto items = get_directory_contents(current_path);
            
            DiskInfo disk_info;
            struct statvfs vfs;
            if (statvfs(current_path.c_str(), &vfs) == 0) {
                disk_info.total_space = (unsigned long long)vfs.f_blocks * vfs.f_frsize;
                disk_info.free_space = (unsigned long long)vfs.f_bfree * vfs.f_frsize;
                disk_info.available_space = (unsigned long long)vfs.f_bavail * vfs.f_frsize;
                disk_info.used_space = disk_info.total_space - disk_info.free_space;
                disk_info.usage_percentage = (double)disk_info.used_space / disk_info.total_space * 100.0;
                disk_info.filesystem = "linux";
            }
            
            std::ostringstream oss;
            oss << "{"
                << "\"path\":\"" << current_path << "\","
                << "\"itemCount\":" << items.size() << ","
                << "\"diskInfo\":{"
                << "\"total\":\"" << format_size(disk_info.total_space) << "\","
                << "\"free\":\"" << format_size(disk_info.free_space) << "\","
                << "\"available\":\"" << format_size(disk_info.available_space) << "\","
                << "\"used\":\"" << format_size(disk_info.used_space) << "\","
                << "\"filesystem\":\"" << disk_info.filesystem << "\","
                << "\"usagePercent\":" << std::fixed << std::setprecision(1) << disk_info.usage_percentage
                << "}"
                << "}";
            
            return oss.str();
        }
        
        // Информация о конкретном файле
        std::string file_path = get_absolute_path(args[0]);
        struct stat statbuf;
        
        if (lstat(file_path.c_str(), &statbuf) != 0) {
            return "{\"error\":\"File not found: " + std::string(strerror(errno)) + "\"}";
        }
        
        bool is_dir = S_ISDIR(statbuf.st_mode);
        bool is_link = S_ISLNK(statbuf.st_mode);
        long long size = is_dir ? 0 : statbuf.st_size;
        
        // Для симлинков получаем цель
        std::string symlink_target;
        if (is_link) {
            char target[PATH_MAX];
            ssize_t len = readlink(file_path.c_str(), target, sizeof(target) - 1);
            if (len != -1) {
                target[len] = '\0';
                symlink_target = target;
            }
        }
        
        std::string type;
        if (is_link) type = "symlink";
        else if (is_dir) type = "directory";
        else type = "file";
        
        std::string permissions = get_permissions_string(statbuf.st_mode);
        
        // Владелец и группа
        struct passwd* pwd = getpwuid(statbuf.st_uid);
        struct group* grp = getgrgid(statbuf.st_gid);
        
        std::string owner = pwd ? pwd->pw_name : std::to_string(statbuf.st_uid);
        std::string group = grp ? grp->gr_name : std::to_string(statbuf.st_gid);
        
        std::ostringstream oss;
        oss << "{"
            << "\"name\":\"" << escape_json(file_path.substr(file_path.find_last_of("/") + 1)) << "\","
            << "\"path\":\"" << escape_json(file_path) << "\","
            << "\"type\":\"" << type << "\","
            << "\"size\":" << size << ","
            << "\"sizeFormatted\":\"" << format_size(size) << "\","
            << "\"permissions\":\"" << permissions << "\","
            << "\"owner\":\"" << owner << "\","
            << "\"group\":\"" << group << "\","
            << "\"isHidden\":" << (file_path.substr(file_path.find_last_of("/") + 1)[0] == '.' ? "true" : "false") << ","
            << "\"isReadOnly\":" << (!(statbuf.st_mode & S_IWUSR) ? "true" : "false") << ","
            << "\"symlinkTarget\":\"" << escape_json(symlink_target) << "\","
            << "\"modified\":\"" << time_to_string(statbuf.st_mtime) << "\""
            << "}";
        
        return oss.str();
    }
    
    std::string create_directory(const std::vector<std::string>& args) {
        if (args.empty()) {
            return "{\"error\":\"No directory name specified\"}";
        }
        
        std::string dir_name = args[0];
        std::string full_path = get_absolute_path(dir_name);
        
        // Создаем все промежуточные директории, если нужно
        if (args.size() > 1 && args[1] == "-p") {
            if (mkdir_recursive(full_path) == 0) {
                return "{\"success\":true,\"path\":\"" + full_path + "\"}";
            } else {
                return "{\"error\":\"Failed to create directory: " + std::string(strerror(errno)) + "\"}";
            }
        }
        
        if (mkdir(full_path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0) {
            return "{\"success\":true,\"path\":\"" + full_path + "\"}";
        }
        
        return "{\"error\":\"Failed to create directory: " + std::string(strerror(errno)) + "\"}";
    }
    
    int mkdir_recursive(const std::string& path) {
        std::string current;
        size_t pos = 0;
        
        while ((pos = path.find('/', pos + 1)) != std::string::npos) {
            current = path.substr(0, pos);
            if (current.empty()) continue;
            
            if (mkdir(current.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) {
                return -1;
            }
        }
        
        return mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    }
    
    std::string delete_item(const std::vector<std::string>& args) {
        if (args.empty()) {
            return "{\"error\":\"No item specified\"}";
        }
        
        std::string item_path = get_absolute_path(args[0]);
        struct stat statbuf;
        
        if (lstat(item_path.c_str(), &statbuf) != 0) {
            return "{\"error\":\"Item not found: " + std::string(strerror(errno)) + "\"}";
        }
        
        bool success = false;
        if (S_ISDIR(statbuf.st_mode)) {
            // Рекурсивное удаление директории
            if (args.size() > 1 && args[1] == "-r") {
                success = delete_directory_recursive(item_path);
            } else {
                success = (rmdir(item_path.c_str()) == 0);
            }
        } else {
            success = (unlink(item_path.c_str()) == 0);
        }
        
        if (success) {
            return "{\"success\":true,\"item\":\"" + item_path + "\"}";
        }
        
        return "{\"error\":\"Failed to delete item: " + std::string(strerror(errno)) + "\"}";
    }
    
    bool delete_directory_recursive(const std::string& path) {
        DIR* dir = opendir(path.c_str());
        if (!dir) return false;
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            std::string full_path = path + "/" + entry->d_name;
            struct stat statbuf;
            
            if (lstat(full_path.c_str(), &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode)) {
                    if (!delete_directory_recursive(full_path)) {
                        closedir(dir);
                        return false;
                    }
                } else {
                    if (unlink(full_path.c_str()) != 0) {
                        closedir(dir);
                        return false;
                    }
                }
            }
        }
        
        closedir(dir);
        return (rmdir(path.c_str()) == 0);
    }
    
    std::string copy_item(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            return "{\"error\":\"Source and destination required\"}";
        }
        
        std::string src = get_absolute_path(args[0]);
        std::string dst = get_absolute_path(args[1]);
        
        // Проверяем, является ли источник директорией
        struct stat src_stat;
        if (lstat(src.c_str(), &src_stat) != 0) {
            return "{\"error\":\"Source not found: " + std::string(strerror(errno)) + "\"}";
        }
        
        bool recursive = (args.size() > 2 && args[2] == "-r");
        
        if (S_ISDIR(src_stat.st_mode) && !recursive) {
            return "{\"error\":\"Cannot copy directory without -r flag\"}";
        }
        
        if (S_ISDIR(src_stat.st_mode)) {
            return copy_directory(src, dst);
        }
        
        // Копирование обычного файла
        int source_fd = open(src.c_str(), O_RDONLY);
        if (source_fd == -1) {
            return "{\"error\":\"Cannot open source file: " + std::string(strerror(errno)) + "\"}";
        }
        
        int dest_fd = open(dst.c_str(), O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);
        if (dest_fd == -1) {
            close(source_fd);
            return "{\"error\":\"Cannot create destination file: " + std::string(strerror(errno)) + "\"}";
        }
        
        char buffer[4096];
        ssize_t bytes_read;
        bool success = true;
        
        while ((bytes_read = read(source_fd, buffer, sizeof(buffer))) > 0) {
            if (write(dest_fd, buffer, bytes_read) != bytes_read) {
                success = false;
                break;
            }
        }
        
        close(source_fd);
        close(dest_fd);
        
        if (!success) {
            unlink(dst.c_str());
            return "{\"error\":\"Failed to copy file data: " + std::string(strerror(errno)) + "\"}";
        }
        
        // Копируем время модификации
        struct timeval times[2];
        times[0].tv_sec = src_stat.st_atime;
        times[0].tv_usec = 0;
        times[1].tv_sec = src_stat.st_mtime;
        times[1].tv_usec = 0;
        utimes(dst.c_str(), times);
        
        return "{\"success\":true,\"source\":\"" + src + "\",\"destination\":\"" + dst + "\"}";
    }
    
    std::string copy_directory(const std::string& src, const std::string& dst) {
        // Создаем целевую директорию
        if (mkdir(dst.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) {
            return "{\"error\":\"Cannot create destination directory: " + std::string(strerror(errno)) + "\"}";
        }
        
        DIR* dir = opendir(src.c_str());
        if (!dir) {
            return "{\"error\":\"Cannot open source directory: " + std::string(strerror(errno)) + "\"}";
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            std::string src_path = src + "/" + entry->d_name;
            std::string dst_path = dst + "/" + entry->d_name;
            
            struct stat statbuf;
            if (lstat(src_path.c_str(), &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode)) {
                    auto result = copy_directory(src_path, dst_path);
                    if (result.find("\"success\":true") == std::string::npos) {
                        closedir(dir);
                        return result;
                    }
                } else {
                    std::vector<std::string> copy_args = {src_path, dst_path};
                    auto result = copy_item(copy_args);
                    if (result.find("\"success\":true") == std::string::npos) {
                        closedir(dir);
                        return result;
                    }
                }
            }
        }
        
        closedir(dir);
        return "{\"success\":true,\"source\":\"" + src + "\",\"destination\":\"" + dst + "\"}";
    }
    
    std::string move_item(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            return "{\"error\":\"Source and destination required\"}";
        }
        
        std::string src = get_absolute_path(args[0]);
        std::string dst = get_absolute_path(args[1]);
        
        if (rename(src.c_str(), dst.c_str()) == 0) {
            return "{\"success\":true,\"source\":\"" + src + "\",\"destination\":\"" + dst + "\"}";
        }
        
        return "{\"error\":\"Failed to move file: " + std::string(strerror(errno)) + "\"}";
    }
    
    std::string rename_item(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            return "{\"error\":\"Old and new names required\"}";
        }
        
        std::string old_name = get_absolute_path(args[0]);
        std::string new_name = get_absolute_path(args[1]);
        
        if (rename(old_name.c_str(), new_name.c_str()) == 0) {
            return "{\"success\":true,\"oldName\":\"" + old_name + "\",\"newName\":\"" + new_name + "\"}";
        }
        
        return "{\"error\":\"Failed to rename: " + std::string(strerror(errno)) + "\"}";
    }
    
    std::string get_current_path() {
        return "{\"currentPath\":\"" + current_path + "\"}";
    }
    
    std::string get_disk_info(const std::vector<std::string>& args) {
        std::string path = current_path;
        if (!args.empty()) {
            path = get_absolute_path(args[0]);
        }
        
        struct statvfs vfs;
        
        if (statvfs(path.c_str(), &vfs) == 0) {
            unsigned long long total = (unsigned long long)vfs.f_blocks * vfs.f_frsize;
            unsigned long long free = (unsigned long long)vfs.f_bfree * vfs.f_frsize;
            unsigned long long available = (unsigned long long)vfs.f_bavail * vfs.f_frsize;
            unsigned long long used = total - free;
            
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1);
            
            oss << "{"
                << "\"path\":\"" << path << "\","
                << "\"totalBytes\":" << total << ","
                << "\"freeBytes\":" << free << ","
                << "\"availableBytes\":" << available << ","
                << "\"usedBytes\":" << used << ","
                << "\"totalFormatted\":\"" << format_size(total) << "\","
                << "\"freeFormatted\":\"" << format_size(free) << "\","
                << "\"availableFormatted\":\"" << format_size(available) << "\","
                << "\"usedFormatted\":\"" << format_size(used) << "\","
                << "\"usagePercent\":" << ((double)used / total * 100.0)
                << "}";
            
            return oss.str();
        }
        
        return "{\"error\":\"Failed to get disk info: " + std::string(strerror(errno)) + "\"}";
    }
    
    std::string format_size(long long bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024.0 && unit < 4) {
            size /= 1024.0;
            unit++;
        }
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
        return oss.str();
    }
};

// Точка входа для n8n
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: navigator_n8n <command> [args...]" << std::endl;
        std::cout << "Commands: list, cd, search, info, mkdir, delete, copy, move, rename, pwd, diskinfo" << std::endl;
        return 1;
    }
    
    FileNavigator navigator;
    std::vector<std::string> args;
    
    for (int i = 2; i < argc; i++) {
        args.push_back(argv[i]);
    }
    
    std::string result = navigator.execute_command(argv[1], args);
    std::cout << result << std::endl;
    
    return 0;
}