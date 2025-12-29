/*
 * Тесты для FileManager утилит
 * Демонстрация модульного тестирования для File Manager Pro
 */

#include "FileManager.hpp"
#include <cassert>
#include <iostream>

namespace FileManagerTests {

void testIsValidFilename() {
    std::cout << "Тест: testIsValidFilename\n";
    
    // Корректные имена
    assert(FileManager::isValidFilename("file.txt") == true);
    assert(FileManager::isValidFilename("my_file-123.cpp") == true);
    assert(FileManager::isValidFilename("README.md") == true);
    
    // Некорректные имена
    assert(FileManager::isValidFilename("") == false); // пустое имя
    assert(FileManager::isValidFilename("file<bad>.txt") == false); // запрещенный символ
    assert(FileManager::isValidFilename("CON.txt") == false); // зарезервированное имя
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void testGetFileExtension() {
    std::cout << "Тест: testGetFileExtension\n";
    
    assert(FileManager::getFileExtension("file.txt") == "txt");
    assert(FileManager::getFileExtension("archive.tar.gz") == "gz"); // последнее расширение
    assert(FileManager::getFileExtension("file_without_ext") == "");
    assert(FileManager::getFileExtension(".hiddenfile") == "hiddenfile");
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void testGetFileType() {
    std::cout << "Тест: testGetFileType\n";
    
    using FileType = FileManager::FileType;
    
    assert(FileManager::getFileType("main.cpp") == FileType::SOURCE_CODE);
    assert(FileManager::getFileType("utils.hpp") == FileType::SOURCE_CODE);
    assert(FileManager::getFileType("script.py") == FileType::SCRIPT);
    assert(FileManager::getFileType("document.pdf") == FileType::DOCUMENT);
    assert(FileManager::getFileType("data.json") == FileType::DATA);
    assert(FileManager::getFileType("image.png") == FileType::IMAGE);
    assert(FileManager::getFileType("archive.zip") == FileType::ARCHIVE);
    assert(FileManager::getFileType("unknown.xyz") == FileType::UNKNOWN);
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void testFormatFileSize() {
    std::cout << "Тест: testFormatFileSize\n";
    
    assert(FileManager::formatFileSize(0) == "0 Б");
    assert(FileManager::formatFileSize(1023) == "1023 Б");
    assert(FileManager::formatFileSize(1024) == "1.00 КБ");
    assert(FileManager::formatFileSize(1024 * 1024) == "1.00 МБ");
    assert(FileManager::formatFileSize(1024 * 1024 * 1024) == "1.00 ГБ");
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void testCombinePaths() {
    std::cout << "Тест: testCombinePaths\n";
    
    using fs = std::filesystem::path;
    
    fs result = FileManager::combinePaths("/home/user", "projects/app");
    assert(result.string().find("projects/app") != std::string::npos);
    
    result = FileManager::combinePaths("C:\\Projects", "src\\main.cpp");
    assert(result.string().find("main.cpp") != std::string::npos);
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void testIsCppProjectFile() {
    std::cout << "Тест: testIsCppProjectFile\n";
    
    assert(FileManager::isCppProjectFile("main.cpp") == true);
    assert(FileManager::isCppProjectFile("utils.hpp") == true);
    assert(FileManager::isCppProjectFile("header.h") == true);
    assert(FileManager::isCppProjectFile("source.cxx") == true);
    assert(FileManager::isCppProjectFile("data.json") == false);
    assert(FileManager::isCppProjectFile("README.md") == false);
    
    std::cout << "  ✓ Все проверки пройдены\n";
}

void runAllTests() {
    std::cout << "=== Запуск тестов FileManager ===\n\n";
    
    testIsValidFilename();
    testGetFileExtension();
    testGetFileType();
    testFormatFileSize();
    testCombinePaths();
    testIsCppProjectFile();
    
    std::cout << "\n=== Все тесты FileManager пройдены успешно! ===\n";
}

} // namespace FileManagerTests