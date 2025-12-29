#ifndef NAVIGATOR_HPP
#define NAVIGATOR_HPP

#include <string>
#include <vector>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <cstring>

// Структуры для n8n (адаптированные для Linux)
struct FileItem {
    std::string name;
    std::string path;
    std::string type; // "file" или "directory"
    long long size;
    std::string modified;
    std::string extension;
    bool isHidden;
    bool isReadOnly;
    
    // Linux-специфичные поля
    std::string permissions;  // права доступа в формате rwxrwxrwx
    std::string owner;        // имя владельца
    std::string group;        // имя группы
    
    // Дополнительные методы для совместимости
    std::string get_formatted_size() const;
};

struct SearchResult {
    std::string name;
    std::string path;
    std::string type;
    long long size;
    std::string modified;
};

struct DiskInfo {
    unsigned long long total_space;
    unsigned long long free_space;
    unsigned long long used_space;
    double usage_percentage;
    
    std::string get_formatted_total() const;
    std::string get_formatted_free() const;
    std::string get_formatted_used() const;
};

class FileNavigator {
private:
    std::string current_path;
    std::vector<std::string> path_history;
    int history_index;
    
    // Вспомогательные методы (адаптированные для Linux)
    bool is_directory_linux(const std::string& path);
    bool is_symlink_linux(const std::string& path);
    bool file_exists_linux(const std::string& path);
    std::string get_absolute_path_linux(const std::string& path);
    std::vector<FileItem> get_directory_contents_linux(const std::string& path);
    std::string to_json(const std::vector<FileItem>& items);
    std::string escape_json(const std::string& s);
    std::string format_size(long long bytes);
    std::string format_permissions(mode_t mode);
    std::string get_owner_name(uid_t uid);
    std::string get_group_name(gid_t gid);
    std::string time_to_string(time_t t);
    std::string get_file_extension(const std::string& filename);
    bool is_hidden_file(const std::string& filename);
    bool is_read_only(mode_t mode);
    
    // Рекурсивные операции
    bool delete_directory_recursive_linux(const std::string& path);
    bool copy_directory_recursive_linux(const std::string& src, const std::string& dst);
    
    // Команды (адаптированные для Linux)
    std::string list_files(const std::vector<std::string>& args);
    std::string change_directory(const std::vector<std::string>& args);
    std::string search_files(const std::vector<std::string>& args);
    std::string get_info(const std::vector<std::string>& args);
    std::string create_directory(const std::vector<std::string>& args);
    std::string delete_item(const std::vector<std::string>& args);
    std::string copy_item(const std::vector<std::string>& args);
    std::string move_item(const std::vector<std::string>& args);
    std::string rename_item(const std::vector<std::string>& args);
    std::string get_current_path();
    std::string get_history();
    std::string go_back();
    std::string go_forward();
    std::string go_to_parent();
    std::string get_disk_info(const std::vector<std::string>& args);
    
    // Вспомогательные методы для копирования файлов
    bool copy_file_contents(const std::string& src, const std::string& dst);
    
    // История навигации
    void add_to_history(const std::string& path);
    void update_current_path(const std::string& new_path);
    
public:
    FileNavigator();
    
    // Основной метод для n8n - возвращает JSON
    std::string execute_command(const std::string& command, const std::vector<std::string>& args);
    
    // Дополнительные методы для совместимости с заголовочным файлом
    std::vector<FileItem> get_directory_contents(bool force_refresh = false);
    bool change_directory_api(const std::string& new_path);
    bool create_directory_api(const std::string& dir_name);
    bool delete_item_api(const std::string& item_path);
    bool copy_item_api(const std::string& src, const std::string& dst);
    bool move_item_api(const std::string& src, const std::string& dst);
    bool rename_item_api(const std::string& old_name, const std::string& new_name);
    bool create_symbolic_link_api(const std::string& target, const std::string& link_path);
    std::string read_symbolic_link_api(const std::string& link_path);
    
    DiskInfo get_disk_info_api(const std::string& path = "");
    std::string get_current_path_api() const;
    std::vector<std::string> get_history_api() const;
    int get_history_index() const;
    
    // Поиск файлов
    bool search_file_api(const std::string& pattern, 
                        std::vector<SearchResult>& results,
                        bool recursive = false,
                        bool case_sensitive = false);
    
    // Linux-специфичные методы
    bool change_permissions_api(const std::string& path, const std::string& permissions);
    bool change_owner_api(const std::string& path, const std::string& owner);
    bool change_group_api(const std::string& path, const std::string& group);
    
    // Информация о файле
    bool get_file_info_api(const std::string& path, FileItem& info);
    
    // Утилиты
    std::string get_parent_directory(const std::string& path);
    bool path_exists(const std::string& path);
    bool is_directory_api(const std::string& path);
    bool is_file_api(const std::string& path);
    bool is_symlink_api(const std::string& path);
};

// Утилитарные функции (inline для заголовочного файла)
namespace FileUtils {
    inline std::string format_bytes(unsigned long long bytes);
    inline bool wildcard_match(const std::string& str, const std::string& pattern, bool case_sensitive = false);
    inline bool is_executable(const std::string& path);
    inline bool is_symbolic_link(const std::string& path);
    inline std::string get_file_type(const std::string& path);
    inline std::string get_mime_type(const std::string& path);
    inline std::string get_file_extension(const std::string& filename);
    inline bool is_hidden_file(const std::string& filename);
    inline std::string join_paths(const std::string& path1, const std::string& path2);
    inline std::string normalize_path(const std::string& path);
    inline std::string get_basename(const std::string& path);
    inline std::string get_dirname(const std::string& path);
    inline std::string get_home_directory();
}

#endif // NAVIGATOR_HPP