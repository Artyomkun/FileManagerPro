/*
 * FileManager.cs
 * Утилиты для работы с файловой системой на C#
 * Демонстрация для File Manager Pro
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace FileManagerPro.Examples.CSharpProject
{
    /// <summary>
    /// Типы файлов для классификации
    /// </summary>
    public enum FileType
    {
        Unknown,
        SourceCode,
        Script,
        Document,
        Data,
        Image,
        Archive,
        Executable
    }

    /// <summary>
    /// Информация о файле
    /// </summary>
    public class FileInfo
    {
        public string Name { get; set; }
        public long Size { get; set; }
        public string Extension { get; set; }
        public DateTime ModifiedDate { get; set; }
        public FileType Type { get; set; }

        public FileInfo(string name = "", long size = 0, string extension = "", DateTime? modifiedDate = null)
        {
            Name = name;
            Size = size;
            Extension = extension;
            ModifiedDate = modifiedDate ?? DateTime.Now;
            Type = DetermineFileType(extension);
        }

        private FileType DetermineFileType(string extension)
        {
            if (string.IsNullOrEmpty(extension))
                return FileType.Unknown;

            string ext = extension.ToLower().TrimStart('.');

            return ext switch
            {
                // Код
                "cs" or "cpp" or "c" or "h" or "hpp" or "java" => FileType.SourceCode,
                
                // Скрипты
                "py" or "js" or "ts" or "php" or "rb" or "sh" => FileType.Script,
                
                // Документы
                "txt" or "md" or "rtf" or "doc" or "docx" or "pdf" => FileType.Document,
                
                // Данные
                "json" or "xml" or "yaml" or "yml" or "csv" or "ini" => FileType.Data,
                
                // Изображения
                "jpg" or "jpeg" or "png" or "gif" or "bmp" or "svg" => FileType.Image,
                
                // Архивы
                "zip" or "rar" or "7z" or "tar" or "gz" => FileType.Archive,
                
                // Исполняемые файлы
                "exe" or "dll" or "so" or "dylib" => FileType.Executable,
                
                _ => FileType.Unknown
            };
        }
    }

    /// <summary>
    /// Утилиты для работы с файловой системой
    /// </summary>
    public static class FileManager
    {
        /// <summary>
        /// Проверяет валидность имени файла
        /// </summary>
        public static bool IsValidFilename(string filename)
        {
            if (string.IsNullOrWhiteSpace(filename) || filename.Length > 255)
                return false;

            // Запрещенные символы в Windows
            char[] invalidChars = Path.GetInvalidFileNameChars();
            
            if (filename.Any(c => invalidChars.Contains(c)))
                return false;

            // Запрещенные имена в Windows
            string[] reservedNames = {
                "CON", "PRN", "AUX", "NUL",
                "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
            };

            string nameWithoutExt = Path.GetFileNameWithoutExtension(filename).ToUpper();
            return !reservedNames.Contains(nameWithoutExt);
        }

        /// <summary>
        /// Получает расширение файла без точки
        /// </summary>
        public static string GetFileExtension(string filepath)
        {
            string ext = Path.GetExtension(filepath);
            return string.IsNullOrEmpty(ext) ? "" : ext.TrimStart('.');
        }

        /// <summary>
        /// Форматирует размер файла в читаемом виде
        /// </summary>
        public static string FormatFileSize(long bytes)
        {
            if (bytes == 0) return "0 Б";

            string[] units = { "Б", "КБ", "МБ", "ГБ", "ТБ" };
            int unitIndex = 0;
            double size = bytes;

            while (size >= 1024 && unitIndex < units.Length - 1)
            {
                size /= 1024;
                unitIndex++;
            }

            return unitIndex == 0 
                ? $"{size:0} {units[unitIndex]}" 
                : $"{size:0.##} {units[unitIndex]}";
        }

        /// <summary>
        /// Проверяет, является ли файл частью C# проекта
        /// </summary>
        public static bool IsCSharpProjectFile(string filepath)
        {
            string ext = GetFileExtension(filepath).ToLower();
            string[] csharpExtensions = { "cs", "csproj", "sln", "xaml", "config" };
            return csharpExtensions.Contains(ext);
        }

        /// <summary>
        /// Получает относительный путь
        /// </summary>
        public static string GetRelativePath(string fullPath, string basePath)
        {
            try
            {
                Uri fullUri = new Uri(fullPath);
                Uri baseUri = new Uri(basePath);

                return Uri.UnescapeDataString(
                    baseUri.MakeRelativeUri(fullUri).ToString()
                ).Replace('/', Path.DirectorySeparatorChar);
            }
            catch
            {
                return fullPath;
            }
        }

        /// <summary>
        /// Сканирует виртуальную структуру проекта (без реальных файлов)
        /// </summary>
        public static List<FileInfo> ScanVirtualProjectStructure()
        {
            return new List<FileInfo>
            {
                new FileInfo("Program.cs", 2048, ".cs", DateTime.Now.AddDays(-1)),
                new FileInfo("FileManager.cs", 4096, ".cs", DateTime.Now),
                new FileInfo("Utils.cs", 1024, ".cs", DateTime.Now.AddDays(-2)),
                new FileInfo("appsettings.json", 512, ".json", DateTime.Now.AddDays(-3)),
                new FileInfo("README.md", 256, ".md", DateTime.Now.AddDays(-4)),
                new FileInfo("project.csproj", 1024, ".csproj", DateTime.Now.AddDays(-5)),
                new FileInfo("config.xml", 768, ".xml", DateTime.Now.AddDays(-6))
            };
        }

        /// <summary>
        /// Анализирует структуру проекта
        /// </summary>
        public static string AnalyzeProjectStructure()
        {
            var files = ScanVirtualProjectStructure();
            
            var sb = new StringBuilder();
            sb.AppendLine("=== Анализ проекта C# ===");
            sb.AppendLine($"Всего файлов: {files.Count}");
            sb.AppendLine($"Общий размер: {FormatFileSize(files.Sum(f => f.Size))}");
            
            sb.AppendLine("\nПо типам файлов:");
            var byType = files.GroupBy(f => f.Type)
                             .OrderByDescending(g => g.Count());
            
            foreach (var group in byType)
            {
                sb.AppendLine($"  {group.Key}: {group.Count()} файлов");
            }
            
            sb.AppendLine("\nСамые большие файлы:");
            var largestFiles = files.OrderByDescending(f => f.Size).Take(3);
            foreach (var file in largestFiles)
            {
                sb.AppendLine($"  {file.Name}: {FormatFileSize(file.Size)}");
            }
            
            sb.AppendLine("========================");
            return sb.ToString();
        }
    }

    /// <summary>
    /// Класс для демонстрации работы с проектами
    /// </summary>
    public class ProjectAnalyzer
    {
        public string ProjectName { get; }
        public List<FileInfo> Files { get; }
        public List<string> Folders { get; }
        
        public ProjectAnalyzer(string projectName)
        {
            ProjectName = projectName;
            Files = new List<FileInfo>();
            Folders = new List<string>();
        }
        
        public void AddFile(FileInfo file)
        {
            Files.Add(file);
        }
        
        public void AddFolder(string folderName)
        {
            Folders.Add(folderName);
        }
        
        public string GetProjectReport()
        {
            var sb = new StringBuilder();
            
            sb.AppendLine($"Отчет по проекту: {ProjectName}");
            sb.AppendLine($"Дата создания: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");
            sb.AppendLine();
            
            sb.AppendLine("Структура проекта:");
            sb.AppendLine($"  Папок: {Folders.Count}");
            sb.AppendLine($"  Файлов: {Files.Count}");
            sb.AppendLine($"  Общий размер: {FileManager.FormatFileSize(Files.Sum(f => f.Size))}");
            
            if (Files.Any())
            {
                sb.AppendLine("\nСтатистика по типам файлов:");
                var typeGroups = Files.GroupBy(f => f.Type)
                                     .OrderByDescending(g => g.Sum(f => f.Size));
                
                foreach (var group in typeGroups)
                {
                    sb.AppendLine($"  {group.Key}: {group.Count()} файлов, " +
                                 $"{FileManager.FormatFileSize(group.Sum(f => f.Size))}");
                }
            }
            
            return sb.ToString();
        }
    }

    /// <summary>
    /// Пример использования File Manager Pro с C#
    /// </summary>
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("========================================");
            Console.WriteLine("File Manager Pro - Демонстрация C#");
            Console.WriteLine("========================================\n");
            
            // Демонстрация валидации имен файлов
            Console.WriteLine("1. Валидация имен файлов:");
            Console.WriteLine($"   'Program.cs': {FileManager.IsValidFilename("Program.cs")}");
            Console.WriteLine($"   'file<bad>.txt': {FileManager.IsValidFilename("file<bad>.txt")}");
            Console.WriteLine($"   'CON.exe': {FileManager.IsValidFilename("CON.exe")}");
            
            // Демонстрация работы с расширениями
            Console.WriteLine("\n2. Работа с расширениями:");
            Console.WriteLine($"   Расширение 'utils.cs': {FileManager.GetFileExtension("utils.cs")}");
            Console.WriteLine($"   Это C# файл: {FileManager.IsCSharpProjectFile("utils.cs")}");
            Console.WriteLine($"   Это C# файл: {FileManager.IsCSharpProjectFile("data.json")}");
            
            // Демонстрация форматирования размеров
            Console.WriteLine("\n3. Форматирование размеров:");
            Console.WriteLine($"   1024 байт: {FileManager.FormatFileSize(1024)}");
            Console.WriteLine($"   1048576 байт: {FileManager.FormatFileSize(1048576)}");
            Console.WriteLine($"   1073741824 байт: {FileManager.FormatFileSize(1073741824)}");
            
            // Анализ виртуального проекта
            Console.WriteLine("\n4. Анализ проекта:");
            Console.WriteLine(FileManager.AnalyzeProjectStructure());
            
            // Создание и анализ проекта
            Console.WriteLine("\n5. Создание проекта:");
            var analyzer = new ProjectAnalyzer("FileManagerPro Demo");
            
            analyzer.AddFolder("src");
            analyzer.AddFolder("docs");
            analyzer.AddFolder("tests");
            
            analyzer.AddFile(new FileInfo("Program.cs", 2048, ".cs"));
            analyzer.AddFile(new FileInfo("Utils.cs", 1024, ".cs"));
            analyzer.AddFile(new FileInfo("README.md", 256, ".md"));
            analyzer.AddFile(new FileInfo("config.json", 512, ".json"));
            
            Console.WriteLine(analyzer.GetProjectReport());
            
            Console.WriteLine("\n========================================");
            Console.WriteLine("File Manager Pro поддерживает C# проекты:");
            Console.WriteLine("• Подсветка синтаксиса C#");
            Console.WriteLine("• Навигация по классам и методам");
            Console.WriteLine("• Поиск по всему проекту");
            Console.WriteLine("• Интеграция с .NET CLI");
            Console.WriteLine("========================================\n");
            
            // Демонстрация относительных путей
            if (args.Length > 0)
            {
                string fullPath = Path.GetFullPath(args[0]);
                string currentDir = Directory.GetCurrentDirectory();
                string relativePath = FileManager.GetRelativePath(fullPath, currentDir);
                
                Console.WriteLine($"Относительный путь: {relativePath}");
            }
        }
    }
}