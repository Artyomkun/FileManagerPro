@echo off
chcp 65001 >nul
title Создание примеров для File Manager Pro

echo ========================================
echo Создание демонстрационных файлов
echo ========================================

REM Создаем структуру папок
mkdir "SourceCodeExamples\C_Example"
mkdir "SourceCodeExamples\Cpp_Example"
mkdir "SourceCodeExamples\CSharp_Example"
mkdir "FileOperationsDemo\To_Copy"
mkdir "FileOperationsDemo\To_Move"
mkdir "FileOperationsDemo\To_Delete"
mkdir "FileOperationsDemo\To_Rename"
mkdir "SearchDemo\Level1\Level2"
mkdir "SearchDemo\different_extensions"
mkdir "NavigationDemo\DeepStructure\Folder1\Subfolder1"
mkdir "NavigationDemo\DeepStructure\Folder1\Subfolder2"
mkdir "NavigationDemo\DeepStructure\Folder2\ManyFiles"
mkdir "NavigationDemo\DeepStructure\Folder2\empty_folder"
mkdir "FileTypesDemo\TextFiles"
mkdir "FileTypesDemo\Images"
mkdir "FileTypesDemo\Archives"
mkdir "FileTypesDemo\Documents"
mkdir "LargeFilesDemo"
mkdir "ConfigurationExamples"

echo Структура папок создана!

REM Создаем README файлы
echo === Примеры для File Manager Pro === > "README.txt"
echo. >> "README.txt"
echo Этот каталог содержит демонстрационные файлы и папки, >> "README.txt"
echo которые показывают возможности File Manager Pro. >> "README.txt"

echo Создание завершено успешно!
echo.
echo Файлы готовы для демонстрации:
echo 1. Исходный код на C/C++/C#
echo 2. Примеры файлов разных типов
echo 3. Демонстрация поиска
echo 4. Примеры навигации
echo 5. Конфигурационные файлы
echo.
pause