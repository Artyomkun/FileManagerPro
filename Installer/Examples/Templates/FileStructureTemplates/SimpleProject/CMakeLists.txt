cmake_minimum_required(VERSION 3.10)
project(SimpleFileManager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configuration options
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_DOCS "Build documentation" OFF)

# Create executable
add_executable(SimpleFileManager main.cpp)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(SimpleFileManager PRIVATE _WIN32)
else()
    target_compile_definitions(SimpleFileManager PRIVATE UNIX)
endif()

# Set output directory
set_target_properties(SimpleFileManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS SimpleFileManager
    RUNTIME DESTINATION bin
)

# Documentation target (optional)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating documentation"
            VERBATIM
        )
    endif()
endif()

# Testing (optional)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()