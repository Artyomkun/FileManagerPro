cmake_minimum_required(VERSION 3.10)

# Название проекта
project(FileManagerPro_Cpp_Project VERSION 1.0.0)

# Язык и стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Настройки компилятора
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Основной исполняемый файл
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/ProjectUtils.cpp
    src/FileManager.cpp
    src/StringUtils.cpp
)

# Директории с заголовками
target_include_directories(${PROJECT_NAME} 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Настройки для отладки
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zi)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -g -O0)
    endif()
endif()

# Настройки для релиза
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /O2)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O3)
    endif()
endif()

# Информация о проекте для File Manager Pro
message(STATUS "======================================")
message(STATUS "File Manager Pro C++ Project Template")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "======================================")

# Установка
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Документация
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_BINARY_DIR}/README.md"
    COPYONLY
)