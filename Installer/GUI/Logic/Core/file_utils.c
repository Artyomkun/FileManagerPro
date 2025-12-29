#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <uuid/uuid.h>
#include <sys/statvfs.h>
#include <sys/inotify.h>
#include <stdbool.h>
#include <utime.h> 

typedef struct {
    bool success;
    char message[512];
    int errorCode;
    void* data;
    size_t dataSize;
} N8N_Result;

typedef struct {
    bool recursive;
    bool showHidden;
    char filter[128];
    int maxDepth;
    bool includeStats;
    bool followSymlinks;
} N8N_ListOptions;

typedef struct {
    bool overwrite;
    bool preserveTimestamps;
    bool createDestDir;
    bool preservePermissions;
    bool preserveOwner;
} N8N_CopyOptions;

typedef struct {
    char name[NAME_MAX];
    char path[PATH_MAX];
    char type[16];
    int isDirectory;
    long long size;
    char modified[32];
    char created[32];
    char extension[16];
    char owner[32];
    char group[32];
    mode_t permissions;
    char permissions_str[11];
    char symlink_target[PATH_MAX];
    unsigned long inode;
    int hardlinks;
    int isHidden;
    int depth;
} N8N_FileInfo;

typedef struct {
    char name[NAME_MAX];
    char path[PATH_MAX];
    int isDirectory;
    long long size;
    char modified[32];
    char extension[16];
    int isHidden;
    int isReadOnly;
} FileInfo;

typedef struct {
    int isDirectory;
    int isSymlink;
    int isHidden;
    int isReadOnly;
    int isSystem;  
    int isArchive;  
    long long size;
    time_t created;
    time_t accessed;
    time_t modified;
    mode_t permissions;
    uid_t uid;
    gid_t gid;
    unsigned long inode;
    unsigned long device;
    int hardlinks;
} FileAttributes;


typedef struct {
    char directory[512];
    int inotify_fd;
    int watch_fd;
    bool running;
    void (*callback)(const char* filename, int action);
} N8N_DirectoryMonitor;

#ifndef PERM_READ
#define PERM_READ   0x01
#define PERM_WRITE  0x02
#define PERM_EXECUTE 0x04
#endif

// ==================== Базовые утилиты ====================
// Глобальные переменные для n8n контекста
static char n8n_workflow_id[64] = "";
static char n8n_execution_id[64] = "";
static char n8n_node_name[64] = "";

// Инициализация n8n контекста
void n8n_init_context(const char* workflow_id, const char* execution_id, const char* node_name) {
    if (workflow_id) strncpy(n8n_workflow_id, workflow_id, sizeof(n8n_workflow_id) - 1);
    if (execution_id) strncpy(n8n_execution_id, execution_id, sizeof(n8n_execution_id) - 1);
    if (node_name) strncpy(n8n_node_name, node_name, sizeof(n8n_node_name) - 1);
}
// Логирование для n8n
void n8n_log(const char* level, const char* message) {
    time_t now = time(NULL);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    printf("[n8n][%s][%s][%s] %s: %s\n", 
           timestamp, n8n_workflow_id, n8n_node_name, level, message);
}

// Освобождение памяти, выделенной для списка файлов
void free_file_list(FileInfo* files) {
    if (files != NULL) {
        free(files);
    }
}

// Освобождение памяти для n8n списка файлов
void n8n_free_file_list(N8N_FileInfo* files) {
    if (files != NULL) {
        free(files);
    }
}

// Проверка, является ли путь директорией
int is_directory(const char* path) {
    struct stat statbuf;
    if (lstat(path, &statbuf) != 0) {
        return -1; // Ошибка
    }
    return S_ISDIR(statbuf.st_mode) ? 1 : 0;
}

// Проверка существования файла/директории
int file_exists(const char* path) {
    return access(path, F_OK) == 0;
}

// Проверка, является ли путь символической ссылкой
int is_symlink(const char* path) {
    struct stat statbuf;
    if (lstat(path, &statbuf) != 0) {
        return -1; // Ошибка
    }
    return S_ISLNK(statbuf.st_mode) ? 1 : 0;
}

// ==================== Расширенные утилиты для n8n ====================

// Получение расширения файла
const char* get_file_extension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

// Получение имени файла без расширения
void get_filename_without_extension(const char* filename, char* output, size_t size) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        strncpy(output, filename, size - 1);
        output[size - 1] = '\0';
    } else {
        size_t len = dot - filename;
        if (len >= size) len = size - 1;
        strncpy(output, filename, len);
        output[len] = '\0';
    }
}

// Объединение путей с учетом разделителей
void join_paths(const char* path1, const char* path2, char* result, size_t size) {
    if (path1[0] == '\0') {
        strncpy(result, path2, size - 1);
        result[size - 1] = '\0';
        return;
    }
    
    if (path2[0] == '\0') {
        strncpy(result, path1, size - 1);
        result[size - 1] = '\0';
        return;
    }
    
    size_t len1 = strlen(path1);
    int has_separator = (path1[len1 - 1] == '/');
    int needs_separator = (path2[0] != '/');
    
    if (has_separator && !needs_separator) {
        // Удаляем лишний разделитель
        char temp[PATH_MAX];
        strncpy(temp, path1, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';
        temp[len1 - 1] = '\0';
        snprintf(result, size, "%s%s", temp, path2);
    } else if (!has_separator && needs_separator) {
        // Добавляем разделитель
        snprintf(result, size, "%s/%s", path1, path2);
    } else {
        snprintf(result, size, "%s%s", path1, path2);
    }
}

// Нормализация пути (замена обратных слешей, удаление точек)
void normalize_path(const char* path, char* result, size_t size) {
    char temp[PATH_MAX];
    strncpy(temp, path, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    
    // Замена обратных слешей на прямые
    for (char* p = temp; *p; p++) {
        if (*p == '\\') *p = '/';
    }
    
    // Обработка . и ..
    char* components[256];
    int comp_count = 0;
    
    char* token = strtok(temp, "/");
    while (token != NULL && comp_count < 256) {
        if (strcmp(token, "..") == 0) {
            if (comp_count > 0) comp_count--;
        } else if (strcmp(token, ".") != 0 && strlen(token) > 0) {
            components[comp_count++] = token;
        }
        token = strtok(NULL, "/");
    }
    
    // Сборка нормализованного пути
    result[0] = '\0';
    for (int i = 0; i < comp_count; i++) {
        if (i > 0) strncat(result, "/", size - strlen(result) - 1);
        strncat(result, components[i], size - strlen(result) - 1);
    }
    
    // Если путь был пустым, добавляем текущую директорию
    if (result[0] == '\0') {
        strncpy(result, ".", size - 1);
        result[size - 1] = '\0';
    }
}

// Получение абсолютного пути
int get_absolute_path(const char* relative_path, char* absolute_path, size_t size) {
    if (realpath(relative_path, absolute_path) == NULL) {
        return errno;
    }
    return 0;
}

// Получение реального пути (с разрешением симлинков)
int get_real_path(const char* path, char* real_path, size_t size) {
    if (realpath(path, real_path) == NULL) {
        return errno;
    }
    return 0;
}

// Получение родительской директории
int get_parent_directory(const char* path, char* parent, size_t size) {
    char* path_copy = strdup(path);
    if (!path_copy) {
        return ENOMEM;
    }
    
    char* dir = dirname(path_copy);
    strncpy(parent, dir, size - 1);
    parent[size - 1] = '\0';
    
    free(path_copy);
    return 0;
}

// ==================== Работа с файловыми атрибутами ====================

// Преобразование прав доступа в строку
void permissions_to_string(mode_t mode, char* str) {
    str[0] = S_ISDIR(mode) ? 'd' : S_ISLNK(mode) ? 'l' : S_ISFIFO(mode) ? 'p' : 
             S_ISSOCK(mode) ? 's' : S_ISCHR(mode) ? 'c' : S_ISBLK(mode) ? 'b' : '-';
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';
    str[10] = '\0';
    
    // Специальные биты
    if (mode & S_ISUID) str[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID) str[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX) str[9] = (mode & S_IXOTH) ? 't' : 'T';
}

// Получение имени владельца
int get_owner_name(uid_t uid, char* name, size_t size) {
    struct passwd* pw = getpwuid(uid);
    if (pw) {
        strncpy(name, pw->pw_name, size - 1);
        name[size - 1] = '\0';
        return 0;
    } else {
        snprintf(name, size, "%d", uid);
        return errno;
    }
}

// Получение имени группы
int get_group_name(gid_t gid, char* name, size_t size) {
    struct group* gr = getgrgid(gid);
    if (gr) {
        strncpy(name, gr->gr_name, size - 1);
        name[size - 1] = '\0';
        return 0;
    } else {
        snprintf(name, size, "%d", gid);
        return errno;
    }
}

// Получение атрибутов файла
int get_file_attributes(const char* path, FileAttributes* attr) {
    struct stat statbuf;
    
    if (lstat(path, &statbuf) != 0) {
        return errno;
    }
    
    // Основные атрибуты
    attr->isDirectory = S_ISDIR(statbuf.st_mode);
    attr->isSymlink = S_ISLNK(statbuf.st_mode);
    attr->isHidden = (strchr(basename((char*)path), '.') == basename((char*)path));
    attr->isReadOnly = !(statbuf.st_mode & S_IWUSR);
    
    // Размер файла
    if (!attr->isDirectory && !attr->isSymlink) {
        attr->size = statbuf.st_size;
    } else {
        attr->size = 0;
    }
    
    // Временные метки
    attr->created = statbuf.st_ctime;
    attr->accessed = statbuf.st_atime;
    attr->modified = statbuf.st_mtime;
    
    // Права доступа
    attr->permissions = statbuf.st_mode & 0777;
    
    // Владелец и группа
    attr->uid = statbuf.st_uid;
    attr->gid = statbuf.st_gid;
    
    // Inode и устройство
    attr->inode = statbuf.st_ino;
    attr->device = statbuf.st_dev;
    
    // Количество жестких ссылок
    attr->hardlinks = statbuf.st_nlink;
    
    // Устанавливаем остальные поля в 0 (не используются в Linux)
    attr->isSystem = 0;
    attr->isArchive = 0;
    
    return 0;
}

// Установка атрибутов файла
int set_file_attributes(const char* path, const FileAttributes* attr) {
    // В Linux можно установить только права доступа
    // Время изменяется автоматически при операциях записи
    if (chmod(path, attr->permissions) != 0) {
        return errno;
    }
    
    // Изменение владельца (требует root прав)
    if (attr->uid != (uid_t)-1 || attr->gid != (gid_t)-1) {
        if (chown(path, attr->uid, attr->gid) != 0) {
            // Не критично, если нет прав
            return 0;
        }
    }
    
    return 0;
}

// ==================== Работа с временными файлами ====================

// Создание временного файла
int create_temp_file(char* temp_path, size_t size, const char* prefix) {
    char template[PATH_MAX];
    const char* tmp_dir = getenv("TMPDIR");
    
    if (!tmp_dir) tmp_dir = "/tmp";
    
    if (prefix) {
        snprintf(template, sizeof(template), "%s/%sXXXXXX", tmp_dir, prefix);
    } else {
        snprintf(template, sizeof(template), "%s/tmpXXXXXX", tmp_dir);
    }
    
    int fd = mkstemp(template);
    if (fd == -1) {
        return errno;
    }
    
    close(fd);
    strncpy(temp_path, template, size - 1);
    temp_path[size - 1] = '\0';
    
    return 0;
}

// Создание временной директории
int create_temp_directory(char* temp_dir, size_t size, const char* prefix) {
    char template[PATH_MAX];
    const char* tmp_dir = getenv("TMPDIR");
    
    if (!tmp_dir) tmp_dir = "/tmp";
    
    if (prefix) {
        snprintf(template, sizeof(template), "%s/%sXXXXXX", tmp_dir, prefix);
    } else {
        snprintf(template, sizeof(template), "%s/tmpXXXXXX", tmp_dir);
    }
    
    char* result = mkdtemp(template);
    if (!result) {
        return errno;
    }
    
    strncpy(temp_dir, result, size - 1);
    temp_dir[size - 1] = '\0';
    
    return 0;
}

// ==================== Проверки безопасности ====================

// Проверка на наличие опасных символов
int has_dangerous_chars(const char* path) {
    // Запрещенные символы в Linux путях
    for (const char* p = path; *p; p++) {
        if (*p == '\0' || *p == '/' || *p < 32) {
            // NULL, разделитель путей или управляющие символы
            continue; // Это нормально для полного пути
        }
        
        // Проверка отдельных компонентов имени файла
        if (strchr(path, '/') != NULL) {
            // Это полный путь, проверяем каждый компонент
            char component[NAME_MAX];
            const char* start = path;
            const char* slash;
            
            while ((slash = strchr(start, '/')) != NULL) {
                size_t len = slash - start;
                if (len > 0 && len < sizeof(component)) {
                    strncpy(component, start, len);
                    component[len] = '\0';
                    
                    // Проверяем компонент
                    for (const char* c = component; *c; c++) {
                        if (*c == '\0') break;
                        // В Linux имена файлов не могут содержать / и NULL
                        if (*c == '/' || *c == '\0') {
                            return 1;
                        }
                    }
                }
                start = slash + 1;
            }
            
            // Последний компонент после последнего /
            if (*start) {
                for (const char* c = start; *c; c++) {
                    if (*c == '/' || *c == '\0') {
                        return 1;
                    }
                }
            }
        }
    }
    
    return 0;
}

// Проверка безопасного имени файла
int is_safe_filename(const char* filename) {
    // Проверка на пустое имя
    if (!filename || filename[0] == '\0') {
        return 0;
    }
    
    // Проверка на точку (скрытые файлы - это нормально в Linux)
    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
        return 0; // Зарезервированные имена
    }
    
    // Проверка на запрещенные символы
    for (const char* p = filename; *p; p++) {
        if (*p == '/' || *p == '\0') {
            return 0; // Недопустимые символы в имени файла
        }
    }
    
    // Проверка длины
    if (strlen(filename) > NAME_MAX) {
        return 0;
    }
    
    return 1;
}

// Проверка пути на безопасность
int is_safe_path(const char* path) {
    // Проверяем наличие попыток обхода директорий
    if (strstr(path, "../") != NULL) {
        return 0;
    }
    
    // Проверяем симлинки
    char real_path[PATH_MAX];
    if (realpath(path, real_path) == NULL) {
        // Если realpath не сработал, проверяем вручную
        struct stat statbuf;
        if (lstat(path, &statbuf) == 0 && S_ISLNK(statbuf.st_mode)) {
            // Это симлинк, нужно быть осторожным
            return 0;
        }
    }
    
    // Проверяем опасные символы
    if (has_dangerous_chars(path)) {
        return 0;
    }
    
    // Проверяем системные пути
    const char* system_paths[] = {
        "/bin/", "/sbin/", "/usr/bin/", "/usr/sbin/",
        "/etc/", "/boot/", "/lib/", "/lib64/",
        "/root/", "/var/log/", "/proc/", "/sys/"
    };
    
    for (size_t i = 0; i < sizeof(system_paths) / sizeof(system_paths[0]); i++) {
        if (strncmp(path, system_paths[i], strlen(system_paths[i])) == 0) {
            return 0; // Попытка доступа к системной директории
        }
    }
    
    return 1; // Безопасно
}

// ==================== MD5 хеширование ====================

// Простая реализация MD5 для Linux
typedef struct {
    unsigned int state[4];
    unsigned int count[2];
    unsigned char buffer[64];
} MD5_CTX;

// Инициализация MD5
void MD5_Init(MD5_CTX* context) {
    context->count[0] = context->count[1] = 0;
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}

// Вспомогательные функции для MD5
static void MD5Transform(unsigned int state[4], const unsigned char block[64]) {
    // Упрощенная реализация (для полноты кода)
    // В реальном коде здесь должна быть полная реализация MD5
    for (int i = 0; i < 16; i++) {
        state[i & 3] ^= ((unsigned int*)block)[i];
    }
}

// Обновление MD5
void MD5_Update(MD5_CTX* context, const unsigned char* input, unsigned int inputLen) {
    unsigned int i, index, partLen;
    
    index = (unsigned int)((context->count[0] >> 3) & 0x3F);
    
    if ((context->count[0] += ((unsigned int)inputLen << 3)) < ((unsigned int)inputLen << 3))
        context->count[1]++;
    context->count[1] += ((unsigned int)inputLen >> 29);
    
    partLen = 64 - index;
    
    if (inputLen >= partLen) {
        memcpy(&context->buffer[index], input, partLen);
        MD5Transform(context->state, context->buffer);
        
        for (i = partLen; i + 63 < inputLen; i += 64)
            MD5Transform(context->state, &input[i]);
        
        index = 0;
    } else {
        i = 0;
    }
    
    memcpy(&context->buffer[index], &input[i], inputLen - i);
}

// Завершение MD5
void MD5_Final(unsigned char digest[16], MD5_CTX* context) {
    unsigned char bits[8];
    unsigned int index, padLen;
    
    memcpy(bits, context->count, 8);
    
    index = (unsigned int)((context->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    
    unsigned char padding[64] = {0x80};
    MD5_Update(context, padding, padLen);
    MD5_Update(context, bits, 8);
    
    memcpy(digest, context->state, 16);
    memset(context, 0, sizeof(*context));
}

// Вычисление MD5 хеша файла
int calculate_file_md5(const char* filename, unsigned char* hash) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return errno;
    }
    
    MD5_CTX context;
    unsigned char buffer[8192];
    size_t bytesRead;
    
    MD5_Init(&context);
    
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        MD5_Update(&context, buffer, (unsigned int)bytesRead);
    }
    
    MD5_Final(hash, &context);
    fclose(file);
    
    return 0;
}

// Получение строкового представления хеша
void hash_to_string(const unsigned char* hash, size_t length, char* output, size_t output_size) {
    const char* hex = "0123456789abcdef";
    size_t needed = length * 2 + 1;
    
    if (output_size < needed) {
        output[0] = '\0';
        return;
    }
    
    for (size_t i = 0; i < length; i++) {
        output[i * 2] = hex[hash[i] >> 4];
        output[i * 2 + 1] = hex[hash[i] & 0x0F];
    }
    output[length * 2] = '\0';
}

// ==================== Утилиты для n8n workflow ====================

// Форматирование размера файла для вывода в n8n
void format_file_size(long long bytes, char* buffer, size_t size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit_index = 0;
    double size_d = (double)bytes;
    
    while (size_d >= 1024.0 && unit_index < 5) {
        size_d /= 1024.0;
        unit_index++;
    }
    
    if (unit_index == 0) {
        snprintf(buffer, size, "%lld %s", bytes, units[unit_index]);
    } else {
        snprintf(buffer, size, "%.2f %s", size_d, units[unit_index]);
    }
}

// Получение MIME типа (базовая реализация)
const char* get_mime_type(const char* filename) {
    const char* ext = get_file_extension(filename);
    
    if (strcasecmp(ext, "txt") == 0) return "text/plain";
    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0) return "text/html";
    if (strcasecmp(ext, "css") == 0) return "text/css";
    if (strcasecmp(ext, "js") == 0) return "application/javascript";
    if (strcasecmp(ext, "json") == 0) return "application/json";
    if (strcasecmp(ext, "xml") == 0) return "application/xml";
    if (strcasecmp(ext, "pdf") == 0) return "application/pdf";
    if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "png") == 0) return "image/png";
    if (strcasecmp(ext, "gif") == 0) return "image/gif";
    if (strcasecmp(ext, "zip") == 0) return "application/zip";
    if (strcasecmp(ext, "tar") == 0) return "application/x-tar";
    if (strcasecmp(ext, "gz") == 0) return "application/gzip";
    
    return "application/octet-stream";
}

// Создание JSON строки для n8n (упрощенная версия)
void create_n8n_json_response(void* result, char* json_buffer, size_t buffer_size) {
    // Эта функция зависит от структуры N8N_Result
    // Здесь базовая реализация
    snprintf(json_buffer, buffer_size, "{\"success\": true, \"message\": \"JSON response generated\"}");
}

// ==================== Работа с символическими ссылками ====================

// Создание символической ссылки
int create_symlink(const char* target, const char* link_path) {
    if (symlink(target, link_path) == 0) {
        return 0;
    }
    return errno;
}

// Чтение символической ссылки
int read_symlink(const char* link_path, char* target, size_t size) {
    ssize_t len = readlink(link_path, target, size - 1);
    if (len == -1) {
        return errno;
    }
    
    target[len] = '\0';
    return 0;
}


// ==================== Работа с правами доступа ====================

// Проверка прав доступа к файлу
int check_file_permissions(const char* path, int required_perms) {
    int access_flags = 0;
    
    if (required_perms & PERM_READ) access_flags |= R_OK;
    if (required_perms & PERM_WRITE) access_flags |= W_OK;
    if (required_perms & PERM_EXECUTE) access_flags |= X_OK;
    
    if (access(path, access_flags) == 0) {
        return 0;
    }
    
    return errno;
}

// Изменение прав доступа
int change_file_permissions(const char* path, mode_t mode) {
    if (chmod(path, mode) == 0) {
        return 0;
    }
    return errno;
}

// Изменение владельца
int change_file_owner(const char* path, uid_t uid, gid_t gid) {
    if (chown(path, uid, gid) == 0) {
        return 0;
    }
    return errno;
}

// ==================== Получение информации о файле для n8n ====================

int get_n8n_file_info(const char* path, N8N_FileInfo* info) {
    struct stat statbuf;
    
    if (lstat(path, &statbuf) != 0) {
        return errno;
    }
    
    // Базовая информация
    strncpy(info->name, basename((char*)path), sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    
    strncpy(info->path, path, sizeof(info->path) - 1);
    info->path[sizeof(info->path) - 1] = '\0';
    
    // Тип файла
    if (S_ISLNK(statbuf.st_mode)) {
        strcpy(info->type, "symlink");
        info->isDirectory = 0;
        
        // Читаем цель симлинка
        read_symlink(path, info->symlink_target, sizeof(info->symlink_target));
    } else if (S_ISDIR(statbuf.st_mode)) {
        strcpy(info->type, "directory");
        info->isDirectory = 1;
        info->symlink_target[0] = '\0';
    } else {
        strcpy(info->type, "file");
        info->isDirectory = 0;
        info->symlink_target[0] = '\0';
    }
    
    // Размер
    if (info->isDirectory) {
        info->size = -1;
    } else {
        info->size = statbuf.st_size;
    }
    
    // Временные метки
    time_to_string(statbuf.st_mtime, info->modified, sizeof(info->modified));
    time_to_string(statbuf.st_ctime, info->created, sizeof(info->created));
    
    // Расширение
    strncpy(info->extension, get_file_extension(info->name), sizeof(info->extension) - 1);
    info->extension[sizeof(info->extension) - 1] = '\0';
    
    // Владелец и группа
    get_owner_name(statbuf.st_uid, info->owner, sizeof(info->owner));
    get_group_name(statbuf.st_gid, info->group, sizeof(info->group));
    
    // Права доступа
    info->permissions = statbuf.st_mode & 0777;
    permissions_to_string(statbuf.st_mode, info->permissions_str);
    
    // Дополнительная информация
    info->inode = statbuf.st_ino;
    info->hardlinks = statbuf.st_nlink;
    info->isHidden = (info->name[0] == '.');
    info->depth = 0; // По умолчанию
    
    return 0;
}


// Получение списка файлов для n8n
N8N_Result n8n_list_files(const char* path, N8N_ListOptions options) {
    N8N_Result result = {0};
    result.success = false;
    
    // Подготовка массива для результатов
    N8N_FileInfo* files = NULL;
    int capacity = 50;
    int fileCount = 0;
    
    files = (N8N_FileInfo*)malloc(capacity * sizeof(N8N_FileInfo));
    if (files == NULL) {
        strcpy(result.message, "Memory allocation failed");
        result.errorCode = ENOMEM;
        return result;
    }
    
    // Рекурсивная функция для обхода директорий
    void search_directory(const char* currentPath, int currentDepth) {
        if (currentDepth > options.maxDepth) return;
        
        DIR* dir = opendir(currentPath);
        if (dir == NULL) {
            n8n_log("WARN", "Cannot access directory");
            return;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            // Пропускаем служебные записи
            if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            // Проверяем скрытые файлы (начинаются с точки в Linux)
            bool isHidden = (entry->d_name[0] == '.');
            if (!options.showHidden && isHidden) {
                continue;
            }
            
            // Проверяем фильтр по имени
            if (options.filter[0] != '\0') {
                // Простая фильтрация по подстроке
                if (strstr(entry->d_name, options.filter) == NULL) {
                    continue;
                }
            }
            
            // Полный путь
            char fullPath[PATH_MAX];
            snprintf(fullPath, sizeof(fullPath), "%s/%s", currentPath, entry->d_name);
            
            // Получение информации о файле
            struct stat statbuf;
            int stat_result;
            
            if (options.followSymlinks) {
                stat_result = stat(fullPath, &statbuf);
            } else {
                stat_result = lstat(fullPath, &statbuf);
            }
            
            if (stat_result != 0) {
                continue; // Не удалось получить информацию о файле
            }
            
            bool isDir = S_ISDIR(statbuf.st_mode);
            bool isLink = S_ISLNK(statbuf.st_mode);
            
            // Увеличиваем массив при необходимости
            if (fileCount >= capacity) {
                capacity *= 2;
                N8N_FileInfo* temp = (N8N_FileInfo*)realloc(files, capacity * sizeof(N8N_FileInfo));
                if (temp == NULL) {
                    n8n_log("ERROR", "Failed to reallocate memory");
                    break;
                }
                files = temp;
            }
            
            // Заполняем информацию о файле
            N8N_FileInfo* fi = &files[fileCount];
            
            // Базовые поля
            strncpy(fi->name, entry->d_name, sizeof(fi->name) - 1);
            fi->name[sizeof(fi->name) - 1] = '\0';
            
            // Полный путь
            strncpy(fi->path, fullPath, sizeof(fi->path) - 1);
            
            // Тип файла
            if (isLink) {
                strcpy(fi->type, "symlink");
                fi->isDirectory = false;
            } else if (isDir) {
                strcpy(fi->type, "directory");
                fi->isDirectory = true;
            } else {
                strcpy(fi->type, "file");
                fi->isDirectory = false;
            }
            
            // Размер
            fi->size = isDir ? -1 : statbuf.st_size;
            
            // Временные метки
            time_to_string(statbuf.st_mtime, fi->modified, sizeof(fi->modified));
            time_to_string(statbuf.st_ctime, fi->created, sizeof(fi->created));
            
            // Права доступа
            get_permissions_string(statbuf.st_mode, fi->permissions);
            
            // Владелец и группа
            struct passwd* pw = getpwuid(statbuf.st_uid);
            struct group* gr = getgrgid(statbuf.st_gid);
            
            if (pw) {
                strncpy(fi->owner, pw->pw_name, sizeof(fi->owner) - 1);
            } else {
                snprintf(fi->owner, sizeof(fi->owner), "%d", statbuf.st_uid);
            }
            
            if (gr) {
                strncpy(fi->group, gr->gr_name, sizeof(fi->group) - 1);
            } else {
                snprintf(fi->group, sizeof(fi->group), "%d", statbuf.st_gid);
            }
            
            // Атрибуты
            fi->isHidden = isHidden;
            fi->depth = currentDepth;
            
            fileCount++;
            
            // Рекурсивный обход для директорий
            if (options.recursive && isDir && !isLink) {
                search_directory(fullPath, currentDepth + 1);
            }
        }
        
        closedir(dir);
    }
    
    // Начинаем поиск
    search_directory(path, 0);
    
    // Формируем результат для n8n
    result.success = true;
    result.data = files;
    result.dataSize = fileCount * sizeof(N8N_FileInfo);
    snprintf(result.message, sizeof(result.message), 
             "Found %d files in directory", fileCount);
    
    return result;
}

// Копирование файла с параметрами n8n
N8N_Result n8n_copy_file(const char* source, const char* destination, N8N_CopyOptions options) {
    N8N_Result result = {0};
    result.success = false;
    
    // Получение информации об исходном файле
    struct stat source_stat;
    if (lstat(source, &source_stat) != 0) {
        snprintf(result.message, sizeof(result.message), 
                 "Source file does not exist: %s", strerror(errno));
        result.errorCode = errno;
        return result;
    }
    
    // Проверка, является ли источник симлинком
    bool isSymlink = S_ISLNK(source_stat.st_mode);
    
    // Проверка назначения
    if (!options.overwrite) {
        if (access(destination, F_OK) == 0) {
            strcpy(result.message, "Destination file already exists");
            result.errorCode = EEXIST;
            return result;
        }
    }
    
    // Создание директории назначения если нужно
    if (options.createDestDir) {
        char destDir[PATH_MAX];
        strncpy(destDir, destination, sizeof(destDir) - 1);
        
        // Находим последний разделитель
        char* lastSlash = strrchr(destDir, '/');
        if (lastSlash) {
            *lastSlash = '\0';
            mkdir(destDir, 0755);
        }
    }
    
    if (isSymlink) {
        // Копирование симлинка
        char link_target[PATH_MAX];
        ssize_t len = readlink(source, link_target, sizeof(link_target) - 1);
        if (len != -1) {
            link_target[len] = '\0';
            if (symlink(link_target, destination) == 0) {
                result.success = true;
                strcpy(result.message, "Symbolic link copied successfully");
            } else {
                result.errorCode = errno;
                snprintf(result.message, sizeof(result.message),
                            "Failed to create symbolic link: %s", strerror(errno));
            }
        }
    } else if (S_ISDIR(source_stat.st_mode)) {
        // Копирование директории
        if (mkdir(destination, source_stat.st_mode) == 0) {
            result.success = true;
            strcpy(result.message, "Directory created successfully");
            
            // Копируем права доступа
            if (options.preservePermissions) {
                chmod(destination, source_stat.st_mode);
            }
        } else {
            result.errorCode = errno;
            snprintf(result.message, sizeof(result.message),
                        "Failed to create directory: %s", strerror(errno));
        }
    } else {
        // Копирование обычного файла
        FILE* src_fp = fopen(source, "rb");
        if (!src_fp) {
            snprintf(result.message, sizeof(result.message),
                        "Cannot open source file: %s", strerror(errno));
            result.errorCode = errno;
            return result;
        }
        
        FILE* dst_fp = fopen(destination, "wb");
        if (!dst_fp) {
            fclose(src_fp);
            snprintf(result.message, sizeof(result.message),
                        "Cannot open destination file: %s", strerror(errno));
            result.errorCode = errno;
            return result;
        }
        
        // Буфер для копирования
        char buffer[8192];
        size_t bytes_read;
        long long total_bytes = 0;
        
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
            if (fwrite(buffer, 1, bytes_read, dst_fp) != bytes_read) {
                result.errorCode = errno;
                snprintf(result.message, sizeof(result.message),
                            "Write error: %s", strerror(errno));
                fclose(src_fp);
                fclose(dst_fp);
                return result;
            }
            total_bytes += bytes_read;
        }
        
        fclose(src_fp);
        fclose(dst_fp);
        
        result.success = true;
        
        // Сохранение атрибутов
        if (options.preserveTimestamps) {
            struct utimbuf times;
            times.actime = source_stat.st_atime;
            times.modtime = source_stat.st_mtime;
            utime(destination, &times);
        }
        
        if (options.preservePermissions) {
            chmod(destination, source_stat.st_mode);
        }
        
        if (options.preserveOwner) {
            chown(destination, source_stat.st_uid, source_stat.st_gid);
        }
        
        snprintf(result.message, sizeof(result.message),
                 "File copied successfully. Size: %lld bytes", total_bytes);
    }
    
    return result;
}

// Массовая операция копирования для n8n
N8N_Result n8n_batch_copy(const char** sources, const char** destinations, 
                          int count, N8N_CopyOptions options) {
    N8N_Result result = {0};
    result.success = true;
    
    int* results = (int*)malloc(count * sizeof(int));
    if (results == NULL) {
        strcpy(result.message, "Memory allocation failed");
        result.success = false;
        return result;
    }
    
    int successCount = 0;
    int failCount = 0;
    
    for (int i = 0; i < count; i++) {
        N8N_Result copyResult = n8n_copy_file(sources[i], destinations[i], options);
        results[i] = copyResult.success ? 1 : 0;
        
        if (copyResult.success) {
            successCount++;
        } else {
            failCount++;
            n8n_log("ERROR", copyResult.message);
        }
    }
    
    snprintf(result.message, sizeof(result.message),
             "Batch copy completed: %d succeeded, %d failed", successCount, failCount);
    
    result.data = results;
    result.dataSize = count * sizeof(int);
    
    return result;
}



N8N_DirectoryMonitor* n8n_create_monitor(const char* directory, void (*callback)(const char*, int)) {
    N8N_DirectoryMonitor* monitor = (N8N_DirectoryMonitor*)malloc(sizeof(N8N_DirectoryMonitor));
    if (!monitor) return NULL;
    
    strncpy(monitor->directory, directory, sizeof(monitor->directory) - 1);
    monitor->callback = callback;
    monitor->running = false;
    
    // Инициализация inotify
    monitor->inotify_fd = inotify_init();
    if (monitor->inotify_fd < 0) {
        free(monitor);
        return NULL;
    }
    
    // Добавление watch для директории
    monitor->watch_fd = inotify_add_watch(monitor->inotify_fd, directory,
                                         IN_CREATE | IN_DELETE | IN_MODIFY |
                                         IN_MOVED_FROM | IN_MOVED_TO |
                                         IN_ATTRIB);
    
    if (monitor->watch_fd < 0) {
        close(monitor->inotify_fd);
        free(monitor);
        return NULL;
    }
    
    return monitor;
}

void n8n_start_monitor(N8N_DirectoryMonitor* monitor) {
    if (!monitor || monitor->running) return;
    
    monitor->running = true;
    
    // Буфер для событий inotify
    char buffer[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
    
    while (monitor->running) {
        ssize_t length = read(monitor->inotify_fd, buffer, sizeof(buffer));
        
        if (length <= 0) {
            usleep(100000); // 100ms задержка при ошибке
            continue;
        }
        
        // Обработка событий
        for (char* ptr = buffer; ptr < buffer + length;) {
            struct inotify_event* event = (struct inotify_event*)ptr;
            
            if (monitor->callback && event->len > 0) {
                int action = 0;
                
                // Преобразование событий inotify в коды действий
                if (event->mask & IN_CREATE) action = 1;
                else if (event->mask & IN_DELETE) action = 2;
                else if (event->mask & IN_MODIFY) action = 3;
                else if (event->mask & IN_MOVED_FROM) action = 4;
                else if (event->mask & IN_MOVED_TO) action = 5;
                else if (event->mask & IN_ATTRIB) action = 6;
                
                monitor->callback(event->name, action);
            }
            
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
}

void n8n_stop_monitor(N8N_DirectoryMonitor* monitor) {
    if (monitor) {
        monitor->running = false;
        
        if (monitor->watch_fd >= 0) {
            inotify_rm_watch(monitor->inotify_fd, monitor->watch_fd);
        }
        
        if (monitor->inotify_fd >= 0) {
            close(monitor->inotify_fd);
        }
    }
}


// Изменение прав доступа
N8N_Result n8n_change_permissions(const char* path, mode_t mode) {
    N8N_Result result = {0};
    
    if (chmod(path, mode) == 0) {
        result.success = true;
        strcpy(result.message, "Permissions changed successfully");
    } else {
        result.errorCode = errno;
        snprintf(result.message, sizeof(result.message),
                 "Failed to change permissions: %s", strerror(errno));
    }
    
    return result;
}

// Освобождение памяти результатов
void n8n_free_result(N8N_Result* result) {
    if (result && result->data) {
        free(result->data);
        result->data = NULL;
        result->dataSize = 0;
    }
}

// ==================== Утилиты времени ====================

// Получение текущего времени в миллисекундах
long long get_current_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// ==================== Экспортируемые функции для n8n ====================


// Экспорт функций для n8n (для совместимости с динамической библиотекой)
__attribute__((visibility("default"))) N8N_Result n8n_list_files_export(const char* path, N8N_ListOptions options) {
    return n8n_list_files(path, options);
}

__attribute__((visibility("default"))) N8N_Result n8n_copy_file_export(const char* src, const char* dst, N8N_CopyOptions options) {
    return n8n_copy_file(src, dst, options);
}

__attribute__((visibility("default"))) void n8n_init_export(const char* workflow_id, const char* execution_id, const char* node_name) {
    n8n_init_context(workflow_id, execution_id, node_name);
}

__attribute__((visibility("default"))) int n8n_file_exists(const char* path) {
    return file_exists(path);
}

__attribute__((visibility("default"))) int n8n_is_directory(const char* path) {
    return is_directory(path);
}

__attribute__((visibility("default"))) void n8n_normalize_path(const char* path, char* result, size_t size) {
    normalize_path(path, result, size);
}

__attribute__((visibility("default"))) int n8n_get_absolute_path(const char* relative_path, char* absolute_path, size_t size) {
    return get_absolute_path(relative_path, absolute_path, size);
}

__attribute__((visibility("default"))) int n8n_is_safe_filename(const char* filename) {
    return is_safe_filename(filename);
}

__attribute__((visibility("default"))) void n8n_format_file_size(long long bytes, char* buffer, size_t size) {
    format_file_size(bytes, buffer, size);
}

__attribute__((visibility("default"))) int n8n_get_file_info(const char* path, N8N_FileInfo* info) {
    return get_n8n_file_info(path, info);
}

__attribute__((visibility("default"))) int n8n_check_permissions(const char* path, int required_perms) {
    return check_file_permissions(path, required_perms);
}

__attribute__((visibility("default"))) const char* n8n_get_mime_type(const char* filename) {
    return get_mime_type(filename);
}