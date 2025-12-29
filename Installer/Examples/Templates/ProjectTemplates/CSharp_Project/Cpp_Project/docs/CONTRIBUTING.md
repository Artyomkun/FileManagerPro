# Contributing to FileManager (C++ Version)

## Building the Project

### Prerequisites

- C++17 compatible compiler (GCC, Clang, or MSVC)
- CMake 3.15+

### Build Instructions

```bash
    mkdir build && cd build
    cmake ..
    cmake --build .
    ./FileManager
```

## Code Style Guidelines

### Naming Conventions

- Classes: `PascalCase` (e.g., `FileManager`, `DirectoryScanner`)
- Methods/functions: `camelCase` (e.g., `listFiles`, `changeDirectory`)
- Variables: `camelCase` (e.g., `currentPath`, `fileCount`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_PATH_LENGTH`)

### File Structure

- Header files: `.h` extension
- Source files: `.cpp` extension
- One class per header/source pair

### Formatting

- 4-space indentation (no tabs)
- 100 character line limit
- Opening braces on same line
- Include guards: `#ifndef FILENAME_H`, `#define FILENAME_H`

### Example

```cpp
// Good
class FileSystem {
public:
    explicit FileSystem(const std::string& root);
    std::vector<FileInfo> listDirectory() const;
    
private:
    std::string currentPath;
    bool isValidPath(const std::string& path) const;
};

// Bad
class filesystem{public:filesystem(string r){root=r;}};
```

## Project Structure

Cpp_Project/
    ├── src/           # Source files
    ├── include/       # Header files
    ├── tests/         # Unit tests
    ├── docs/          # Documentation
    └── CMakeLists.txt # Build configuration

## Development Workflow

1. **Fork** the repository
2. **Create a branch**:

   ```bash
        git checkout -b feature/your-feature-name
   ```

3. **Make changes** with descriptive commit messages
4. **Test your changes**:

   ```bash
        cd build && ctest
   ```

5. **Create a Pull Request**

## Commit Message Format

[type]: Short description

[optional longer description]

Types:
    - feat: New feature
    - fix: Bug fix
    - docs: Documentation
    - style: Code style/formatting
    - refactor: Code restructuring
    - test: Adding/updating tests
    - chore: Maintenance tasks

## Reporting Issues

When reporting bugs, include:

1. Operating system and version
2. Compiler and version
3. Steps to reproduce
4. Expected vs actual behavior

## Code Review Process

1. All PRs require at least one review
2. Ensure code follows style guidelines
3. Update documentation if needed
4. Add tests for new functionality

## License

By contributing, you agree that your contributions will be licensed under the project's MIT License.
