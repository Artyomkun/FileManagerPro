#ifndef UTILS_H
#define UTILS_H

#include <time.h>

/* String Utilities */
char* trim_string(char* str);
int strcasecmp_custom(const char* s1, const char* s2);
int ends_with(const char* str, const char* suffix);
char* replace_substring(const char* original, const char* pattern, 
                        const char* replacement);

/* File System Utilities */
const char* get_file_extension(const char* filename);
char* get_filename_without_extension(const char* filename);
char* join_paths(const char* path1, const char* path2);
char* get_human_readable_size(long long bytes);
int is_directory(const char* path);
int is_regular_file(const char* path);
int create_directory_recursive(const char* path);

/* Date/Time Utilities */
char* get_current_timestamp();
char* format_file_time(time_t modtime);

/* Memory Utilities */
char* safe_strdup(const char* str);
void free_string_array(char** array, int count);

/* Validation Utilities */
int is_valid_filename(const char* filename);

/* Platform Utilities */
char* get_home_directory();

#endif /* UTILS_H */