#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "utils.h"

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
    #define PATH_SEPARATOR '\\'
#else
    #include <unistd.h>
    #include <pwd.h>
    #define PATH_SEPARATOR '/'
#endif

/* ==================== String Utilities ==================== */

/**
 * Trims whitespace from the beginning and end of a string
 * Modifies the original string
 */
char* trim_string(char* str) {
    if (str == NULL) return NULL;
    
    char* end;
    
    // Trim leading space
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }
    
    if (*str == '\0') {  // All spaces?
        return str;
    }
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    
    // Write new null terminator
    *(end + 1) = '\0';
    
    return str;
}

/**
 * Case-insensitive string comparison
 */
int strcasecmp_custom(const char* s1, const char* s2) {
    if (s1 == NULL || s2 == NULL) {
        return (s1 == s2) ? 0 : ((s1 == NULL) ? -1 : 1);
    }
    
    while (*s1 && *s2) {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
        
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    
    return (*s1 - *s2);
}

/**
 * Checks if string ends with given suffix
 */
int ends_with(const char* str, const char* suffix) {
    if (str == NULL || suffix == NULL) {
        return 0;
    }
    
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    
    if (suffix_len > str_len) {
        return 0;
    }
    
    return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

/**
 * Replaces all occurrences of a substring
 * Returns newly allocated string - caller must free!
 */
char* replace_substring(const char* original, const char* pattern, const char* replacement) {
    if (original == NULL || pattern == NULL || replacement == NULL) {
        return NULL;
    }
    
    size_t orig_len = strlen(original);
    size_t pat_len = strlen(pattern);
    size_t rep_len = strlen(replacement);
    
    // Count occurrences
    int count = 0;
    const char* tmp = original;
    while ((tmp = strstr(tmp, pattern)) != NULL) {
        count++;
        tmp += pat_len;
    }
    
    // Calculate new length
    size_t new_len = orig_len + count * (rep_len - pat_len);
    char* result = malloc(new_len + 1);
    if (result == NULL) {
        return NULL;
    }
    
    // Perform replacement
    char* dest = result;
    const char* src = original;
    
    while (*src) {
        if (strstr(src, pattern) == src) {
            strcpy(dest, replacement);
            dest += rep_len;
            src += pat_len;
        } else {
            *dest++ = *src++;
        }
    }
    
    *dest = '\0';
    return result;
}

/* ==================== File System Utilities ==================== */

/**
 * Gets file extension from filename
 */
const char* get_file_extension(const char* filename) {
    if (filename == NULL) {
        return "";
    }
    
    const char* dot = strrchr(filename, '.');
    if (dot == NULL || dot == filename) {
        return "";
    }
    
    return dot + 1;
}

/**
 * Gets filename without extension
 * Returns newly allocated string - caller must free!
 */
char* get_filename_without_extension(const char* filename) {
    if (filename == NULL) {
        return NULL;
    }
    
    const char* dot = strrchr(filename, '.');
    const char* sep = strrchr(filename, PATH_SEPARATOR);
    
    if (sep != NULL) {
        filename = sep + 1;
    }
    
    size_t len;
    if (dot != NULL && (sep == NULL || dot > sep)) {
        len = dot - filename;
    } else {
        len = strlen(filename);
    }
    
    char* result = malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }
    
    strncpy(result, filename, len);
    result[len] = '\0';
    
    return result;
}

/**
 * Joins two paths correctly
 * Returns newly allocated string - caller must free!
 */
char* join_paths(const char* path1, const char* path2) {
    if (path1 == NULL || path2 == NULL) {
        return NULL;
    }
    
    size_t len1 = strlen(path1);
    size_t len2 = strlen(path2);
    size_t total_len = len1 + len2 + 2;  // +1 for separator, +1 for null terminator
    
    char* result = malloc(total_len);
    if (result == NULL) {
        return NULL;
    }
    
    strcpy(result, path1);
    
    // Add separator if needed
    if (len1 > 0 && result[len1 - 1] != PATH_SEPARATOR && 
        len2 > 0 && path2[0] != PATH_SEPARATOR) {
        result[len1] = PATH_SEPARATOR;
        result[len1 + 1] = '\0';
    } else if (len1 > 0 && result[len1 - 1] == PATH_SEPARATOR && 
               len2 > 0 && path2[0] == PATH_SEPARATOR) {
        // Remove duplicate separator
        result[len1 - 1] = '\0';
    }
    
    strcat(result, path2);
    return result;
}

/**
 * Gets file size in human-readable format
 * Returns newly allocated string - caller must free!
 */
char* get_human_readable_size(long long bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit_index = 0;
    double size = (double)bytes;
    
    while (size >= 1024.0 && unit_index < 5) {
        size /= 1024.0;
        unit_index++;
    }
    
    char* result = malloc(32);
    if (result == NULL) {
        return NULL;
    }
    
    if (unit_index == 0) {
        snprintf(result, 32, "%lld %s", bytes, units[unit_index]);
    } else {
        snprintf(result, 32, "%.2f %s", size, units[unit_index]);
    }
    
    return result;
}

/**
 * Checks if path is a directory
 */
int is_directory(const char* path) {
    struct stat path_stat;
    
    if (stat(path, &path_stat) != 0) {
        return 0;
    }
    
    return S_ISDIR(path_stat.st_mode);
}

/**
 * Checks if path is a regular file
 */
int is_regular_file(const char* path) {
    struct stat path_stat;
    
    if (stat(path, &path_stat) != 0) {
        return 0;
    }
    
    return S_ISREG(path_stat.st_mode);
}

/**
 * Creates directory recursively
 */
int create_directory_recursive(const char* path) {
    if (path == NULL || *path == '\0') {
        return -1;
    }
    
    char* temp = strdup(path);
    if (temp == NULL) {
        return -1;
    }
    
    char* p = temp;
    
    // Skip drive letter on Windows
    #ifdef _WIN32
    if (strlen(p) > 1 && p[1] == ':') {
        p += 2;
    }
    #endif
    
    // Skip leading separators
    while (*p == PATH_SEPARATOR) {
        p++;
    }
    
    while (*p != '\0') {
        // Find next separator
        while (*p != '\0' && *p != PATH_SEPARATOR) {
            p++;
        }
        
        // Save current position
        char saved_char = *p;
        *p = '\0';
        
        // Create directory if it doesn't exist
        struct stat st;
        if (stat(temp, &st) != 0) {
            if (mkdir(temp, 0755) != 0 && errno != EEXIST) {
                free(temp);
                return -1;
            }
        } else if (!S_ISDIR(st.st_mode)) {
            free(temp);
            return -1;
        }
        
        // Restore separator and continue
        if (saved_char != '\0') {
            *p = saved_char;
            p++;
        }
    }
    
    free(temp);
    return 0;
}

/* ==================== Date/Time Utilities ==================== */

/**
 * Gets current timestamp as string
 * Returns newly allocated string - caller must free!
 */
char* get_current_timestamp() {
    time_t rawtime;
    struct tm* timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    char* result = malloc(64);
    if (result == NULL) {
        return NULL;
    }
    
    strftime(result, 64, "%Y-%m-%d %H:%M:%S", timeinfo);
    return result;
}

/**
 * Formats file modification time
 * Returns newly allocated string - caller must free!
 */
char* format_file_time(time_t modtime) {
    time_t now;
    time(&now);
    
    double diff = difftime(now, modtime);
    struct tm* t = localtime(&modtime);
    
    char* result = malloc(64);
    if (result == NULL) {
        return NULL;
    }
    
    if (diff < 60 * 60 * 24) {  // Less than 1 day
        strftime(result, 64, "Today %H:%M", t);
    } else if (diff < 60 * 60 * 24 * 7) {  // Less than 1 week
        strftime(result, 64, "%a %H:%M", t);
    } else {
        strftime(result, 64, "%Y-%m-%d %H:%M", t);
    }
    
    return result;
}

/* ==================== Memory Utilities ==================== */

/**
 * Safe string duplication
 * Returns newly allocated string - caller must free!
 */
char* safe_strdup(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    
    char* new_str = malloc(strlen(str) + 1);
    if (new_str == NULL) {
        return NULL;
    }
    
    strcpy(new_str, str);
    return new_str;
}

/**
 * Frees array of strings and the array itself
 */
void free_string_array(char** array, int count) {
    if (array == NULL) {
        return;
    }
    
    for (int i = 0; i < count; i++) {
        free(array[i]);
    }
    
    free(array);
}

/* ==================== Validation Utilities ==================== */

/**
 * Validates if string is a valid filename
 */
int is_valid_filename(const char* filename) {
    if (filename == NULL || *filename == '\0') {
        return 0;
    }
    
    // Check for invalid characters
    const char* invalid_chars = "<>:\"/\\|?*";
    for (const char* p = filename; *p; p++) {
        for (const char* ic = invalid_chars; *ic; ic++) {
            if (*p == *ic) {
                return 0;
            }
        }
        
        // Check for control characters
        if (*p < 32) {
            return 0;
        }
    }
    
    // Check for reserved names on Windows
    #ifdef _WIN32
    const char* reserved_names[] = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    char upper_name[256];
    size_t len = strlen(filename);
    for (size_t i = 0; i < len; i++) {
        upper_name[i] = (filename[i] >= 'a' && filename[i] <= 'z') ? 
                        filename[i] - 32 : filename[i];
    }
    upper_name[len] = '\0';
    
    for (int i = 0; i < sizeof(reserved_names)/sizeof(reserved_names[0]); i++) {
        if (strcmp(upper_name, reserved_names[i]) == 0) {
            return 0;
        }
    }
    #endif
    
    return 1;
}

/* ==================== Platform Utilities ==================== */

/**
 * Gets home directory path
 * Returns newly allocated string - caller must free!
 */
char* get_home_directory() {
    #ifdef _WIN32
        char* home = getenv("USERPROFILE");
        if (home == NULL) {
            home = getenv("HOMEDRIVE");
            char* homepath = getenv("HOMEPATH");
            if (home != NULL && homepath != NULL) {
                char* result = malloc(strlen(home) + strlen(homepath) + 1);
                if (result != NULL) {
                    strcpy(result, home);
                    strcat(result, homepath);
                }
                return result;
            }
        }
        return safe_strdup(home);
    #else
        char* home = getenv("HOME");
        if (home == NULL) {
            struct passwd* pw = getpwuid(getuid());
            if (pw != NULL) {
                home = pw->pw_dir;
            }
        }
        return safe_strdup(home);
    #endif
}