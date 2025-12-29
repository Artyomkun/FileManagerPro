/*
 * File Manager Pro - Шаблон C++ проекта
 * Чистый код для демонстрации возможностей File Manager Pro
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

// Пространство имен для утилит проекта
namespace FileManagerProUtils {
    
    /**
     * Класс для демонстрации ООП в C++
     */
    class FileSystemItem {
    protected:
        std::string name;
        size_t size;
        
    public:
        FileSystemItem(const std::string& itemName, size_t itemSize = 0)
            : name(itemName), size(itemSize) {}
        
        virtual ~FileSystemItem() = default;
        
        virtual void display() const {
            std::cout << "Имя: " << name 
                        << ", Размер: " << size << " байт" << std::endl;
        }
        
        const std::string& getName() const { return name; }
        size_t getSize() const { return size; }
    };
    
    /**
     * Класс файла
     */
    class File : public FileSystemItem {
    private:
        std::string extension;
        
    public:
        File(const std::string& fileName, size_t fileSize, 
                const std::string& fileExt)
                : FileSystemItem(fileName, fileSize), extension(fileExt) {}
        
        void display() const override {
            std::cout << "[ФАЙЛ] " << name << "." << extension 
                        << " (" << size << " байт)" << std::endl;
        }
        
        const std::string& getExtension() const { return extension; }
    };
    
    /**
     * Класс папки
     */
    class Folder : public FileSystemItem {
    private:
        std::vector<std::shared_ptr<FileSystemItem>> contents;
        
    public:
        Folder(const std::string& folderName)
            : FileSystemItem(folderName) {}
        
        void addItem(std::shared_ptr<FileSystemItem> item) {
            contents.push_back(item);
            size += item->getSize(); // Обновляем общий размер
        }
        
        void display() const override {
            std::cout << "[ПАПКА] " << name << "/" 
                        << " (элементов: " << contents.size() 
                        << ", общий размер: " << size << " байт)" << std::endl;
            
            for (const auto& item : contents) {
                std::cout << "  ";
                item->display();
            }
        }
        
        size_t getItemCount() const { return contents.size(); }
    };
    
    /**
     * Функция для демонстрации работы с контейнерами STL
     */
    void demonstrateSTLContainers() {
        std::cout << "\n=== Демонстрация STL контейнеров ===" << std::endl;
        
        // Вектор файлов
        std::vector<File> files;
        files.emplace_back("document", 1024, "txt");
        files.emplace_back("image", 2048, "jpg");
        files.emplace_back("archive", 5120, "zip");
        
        std::cout << "Файлы в векторе:" << std::endl;
        for (const auto& file : files) {
            file.display();
        }
        
        // Сортировка по размеру
        std::sort(files.begin(), files.end(), 
                    [](const File& a, const File& b) {
                        return a.getSize() < b.getSize();
                    });
        
        std::cout << "\nОтсортировано по размеру:" << std::endl;
        for (const auto& file : files) {
            file.display();
        }
    }
    
    /**
     * Функция для демонстрации умных указателей
     */
    void demonstrateSmartPointers() {
        std::cout << "\n=== Демонстрация умных указателей ===" << std::endl;
        
        // Создание папки с использованием shared_ptr
        auto rootFolder = std::make_shared<Folder>("FileManagerPro_Project");
        
        // Добавление файлов в папку
        rootFolder->addItem(std::make_shared<File>("main", 2048, "cpp"));
        rootFolder->addItem(std::make_shared<File>("utils", 1024, "h"));
        rootFolder->addItem(std::make_shared<File>("config", 512, "json"));
        
        // Создание вложенной папки
        auto srcFolder = std::make_shared<Folder>("src");
        srcFolder->addItem(std::make_shared<File>("algorithm", 4096, "cpp"));
        
        rootFolder->addItem(srcFolder);
        
        // Отображение структуры
        rootFolder->display();
    }
    
    /**
     * Шаблонная функция для работы с разными типами файлов
     */
    template<typename T>
    void processFileItem(const T& item, const std::string& operation) {
        std::cout << "Выполняется операция '" << operation 
                    << "' над: " << item.getName() << std::endl;
    }
    
} // namespace FileManagerProUtils

/**
 * Демонстрация возможностей современного C++
 */
void demonstrateModernCpp() {
    using namespace FileManagerProUtils;
    
    std::cout << "\n=== Современный C++ в действии ===" << std::endl;
    
    // Лямбда-выражения
    auto fileFilter = [](const File& file, size_t minSize) {
        return file.getSize() > minSize;
    };
    
    std::vector<File> allFiles = {
        File("small", 512, "txt"),
        File("medium", 2048, "jpg"),
        File("large", 8192, "zip")
    };
    
    std::cout << "Файлы больше 1000 байт:" << std::endl;
    for (const auto& file : allFiles) {
        if (fileFilter(file, 1000)) {
            file.display();
        }
    }
    
    // auto и decltype
    auto importantFile = File("important", 4096, "docx");
    decltype(importantFile) backupFile("backup", 2048, "bak");
    
    std::cout << "\nТипы определены автоматически:" << std::endl;
    importantFile.display();
    backupFile.display();
}

/**
 * Главная функция программы
 */
int main() {
    using namespace FileManagerProUtils;
    
    std::cout << "==========================================" << std::endl;
    std::cout << "File Manager Pro - Демонстрация C++ кода" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // Демонстрация ООП
    std::cout << "\n=== Объектно-ориентированное программирование ===" << std::endl;
    
    File mainFile("main", 2048, "cpp");
    File headerFile("utils", 1024, "h");
    
    Folder projectFolder("CppProject");
    projectFolder.addItem(std::make_shared<File>(mainFile));
    projectFolder.addItem(std::make_shared<File>(headerFile));
    
    projectFolder.display();
    
    // Демонстрация STL
    demonstrateSTLContainers();
    
    // Демонстрация умных указателей
    demonstrateSmartPointers();
    
    // Демонстрация современного C++
    demonstrateModernCpp();
    
    // Демонстрация шаблонов
    std::cout << "\n=== Демонстрация шаблонов ===" << std::endl;
    
    File configFile("settings", 512, "json");
    processFileItem(configFile, "валидация");
    
    Folder dataFolder("data");
    processFileItem(dataFolder, "сканирование");
    
    // Итог
    std::cout << "\n==========================================" << std::endl;
    std::cout << "Демонстрация завершена успешно!" << std::endl;
    std::cout << "\nFile Manager Pro поддерживает:" << std::endl;
    std::cout << "• Подсветку синтаксиса C++17" << std::endl;
    std::cout << "• Навигацию по классам и функциям" << std::endl;
    std::cout << "• Поиск по всему проекту" << std::endl;
    std::cout << "• Интеграцию с системами сборки" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    return 0;
}