cmake_minimum_required(VERSION 3.10)
project(FileOps)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Создаем библиотеку
add_library(fileops SHARED
    file_ops.c
    file_utils.c
)

# Устанавливаем выходное имя
set_target_properties(fileops PROPERTIES
    OUTPUT_NAME "fileops"
    VERSION "1.0.0"
    SOVERSION "1"
)

# Публичный заголовок
target_include_directories(fileops PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Устанавливаем выходной каталог
set_target_properties(fileops PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)