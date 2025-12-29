using System;
using System.IO;
using System.Linq;

namespace FileManager
{
    public class PropertiesDialogViewModel
    {
        private readonly FileItem _fileItem;
        
        public string Name => _fileItem.Name;
        public string FullPath => _fileItem.Path;
        public bool IsDirectory => _fileItem.IsDirectory;
        public long Size => _fileItem.Size;
        public DateTime LastModified => _fileItem.Modified;
        
        public string Type => IsDirectory ? "Папка" : "Файл";
        public string FormattedSize => IsDirectory ? GetDirectorySizeInfo() : FormatSize(_fileItem.Size);
        public string LastModifiedDisplay => _fileItem.Modified.ToString("dd.MM.yyyy HH:mm");
        
        public bool IsReadOnly { get; set; }
        public bool IsHidden { get; set; }
        public bool IsArchive { get; set; }
        
        public string Statistics => GetStatistics();
        
        public bool CanApply { get; set; } = true;
        public string Icon => IsDirectory ? "📁" : "📄";
        
        public PropertiesDialogViewModel(FileItem fileItem)
        {
            _fileItem = fileItem;
            LoadFileAttributes();
        }
        
        private void LoadFileAttributes()
        {
            try
            {
                if (File.Exists(FullPath) || Directory.Exists(FullPath))
                {
                    var attributes = File.GetAttributes(FullPath);
                    IsReadOnly = (attributes & FileAttributes.ReadOnly) == FileAttributes.ReadOnly;
                    IsHidden = (attributes & FileAttributes.Hidden) == FileAttributes.Hidden;
                    IsArchive = (attributes & FileAttributes.Archive) == FileAttributes.Archive;
                }
            }
            catch
            {
            }
        }
        
        private string GetDirectorySizeInfo()
        {
            try
            {
                if (Directory.Exists(FullPath))
                {
                    var files = Directory.GetFiles(FullPath, "*", SearchOption.AllDirectories);
                    var fileCount = files.Length;
                    var dirCount = Directory.GetDirectories(FullPath, "*", SearchOption.AllDirectories).Length;
                    
                    return $"Папка, {fileCount} файлов, {dirCount} подпапок";
                }
            }
            catch (UnauthorizedAccessException)
            {
                return "Папка (нет доступа для подсчета)";
            }
            catch
            {
            }
            
            return "Папка";
        }
        
        private string GetStatistics()
        {
            if (IsDirectory)
            {
                try
                {
                    var files = Directory.GetFiles(FullPath);
                    var directories = Directory.GetDirectories(FullPath);
                    var totalSize = files.Sum(f => new FileInfo(f).Length);
                    
                    return $"Содержимое папки:\n" +
                           $"• Файлов: {files.Length}\n" +
                           $"• Подпапок: {directories.Length}\n" +
                           $"• Общий размер: {FormatSize(totalSize)}\n" +
                           $"• Создана: {Directory.GetCreationTime(FullPath):dd.MM.yyyy HH:mm}";
                }
                catch (UnauthorizedAccessException)
                {
                    return "Нет доступа к содержимому папки";
                }
                catch
                {
                    return "Не удалось получить статистику папки";
                }
            }
            else
            {
                try
                {
                    var info = new FileInfo(FullPath);
                    return $"Информация о файле:\n" +
                           $"• Расширение: {info.Extension}\n" +
                           $"• Создан: {info.CreationTime:dd.MM.yyyy HH:mm}\n" +
                           $"• Последний доступ: {info.LastAccessTime:dd.MM.yyyy HH:mm}\n" +
                           $"• Размер на диске: {FormatSize(info.Length)}";
                }
                catch
                {
                    return "Не удалось получить статистику файла";
                }
            }
        }
        
        private static string FormatSize(long bytes)
        {
            string[] sizes = { "B", "KB", "MB", "GB", "TB" };
            double len = bytes;
            int order = 0;
            
            while (len >= 1024 && order < sizes.Length - 1)
            {
                order++;
                len /= 1024;
            }
            
            return $"{len:0.##} {sizes[order]}";
        }
        
        public void ApplyChanges()
        {
            try
            {
                var attributes = File.GetAttributes(FullPath);
                
                if (IsReadOnly)
                    attributes |= FileAttributes.ReadOnly;
                else
                    attributes &= ~FileAttributes.ReadOnly;
                
                if (IsHidden)
                    attributes |= FileAttributes.Hidden;
                else
                    attributes &= ~FileAttributes.Hidden;
                
                if (IsArchive)
                    attributes |= FileAttributes.Archive;
                else
                    attributes &= ~FileAttributes.Archive;
                
                File.SetAttributes(FullPath, attributes);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Ошибка при сохранении атрибутов: {ex.Message}");
            }
        }
    }
}