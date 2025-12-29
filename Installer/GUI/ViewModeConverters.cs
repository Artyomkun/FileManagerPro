using Avalonia.Data.Converters;
using System;
using System.Globalization;

namespace FileManager
{
    /// <summary>
    /// Статический класс с конвертерами для ViewMode
    /// </summary>
    public static class ViewModeConverters
    {
        public static readonly IValueConverter Details = new DetailsConverter();
        public static readonly IValueConverter Icons = new IconsConverter();
        public static readonly IValueConverter List = new ListConverter();
        public static readonly IValueConverter NotDetails = new NotDetailsConverter();
        public static readonly IValueConverter NotIcons = new NotIconsConverter();
        public static readonly IValueConverter EqualTo = new EqualToConverter();
        
        private class DetailsConverter : IValueConverter
        {
            public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                return value is ViewMode mode && mode == ViewMode.Details;
            }
            
            public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }
        
        private class IconsConverter : IValueConverter
        {
            public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                if (value is ViewMode mode)
                {
                    return mode == ViewMode.LargeIcons || 
                           mode == ViewMode.ExtraLargeIcons || 
                           mode == ViewMode.MediumIcons || 
                           mode == ViewMode.SmallIcons;
                }
                return false;
            }
            
            public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }
        
        private class ListConverter : IValueConverter
        {
            public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                return value is ViewMode mode && mode == ViewMode.List;
            }
            
            public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }
        
        private class NotDetailsConverter : IValueConverter
        {
            public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                return value is ViewMode mode && mode != ViewMode.Details;
            }
            
            public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }
        
        private class NotIconsConverter : IValueConverter
        {
            public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                if (value is ViewMode mode)
                {
                    return mode != ViewMode.LargeIcons && 
                           mode != ViewMode.ExtraLargeIcons && 
                           mode != ViewMode.MediumIcons && 
                           mode != ViewMode.SmallIcons;
                }
                return false;
            }
            
            public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }
        
        private class EqualToConverter : IValueConverter
        {
            public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                if (value is ViewMode currentMode && parameter is string modeString)
                {
                    if (Enum.TryParse<ViewMode>(modeString, out var targetMode))
                    {
                        return currentMode == targetMode;
                    }
                }
                return false;
            }
            
            public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
            {
                throw new NotImplementedException();
            }
        }
    }
    
    /// <summary>
    /// Конвертер для преобразования объекта в bool
    /// </summary>
    public class ObjectToBoolConverter : IValueConverter
    {
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            bool invert = false;
            if (parameter is string paramStr && paramStr.Equals("Invert", StringComparison.OrdinalIgnoreCase))
            {
                invert = true;
            }
            
            bool result = value != null;
            return invert ? !result : result;
        }
        
        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            return Avalonia.Data.BindingOperations.DoNothing;
        }
    }
    
    /// <summary>
    /// Конвертер для инвертирования bool значения
    /// </summary>
    public class InverseBoolConverter : IValueConverter
    {
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            if (value is bool boolValue)
            {
                return !boolValue;
            }
            return true;
        }
        
        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            if (value is bool boolValue)
            {
                return !boolValue;
            }
            return false;
        }
    }
    
    /// <summary>
    /// Конвертер для проверки на null
    /// </summary>
    public class IsNullConverter : IValueConverter
    {
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            bool invert = false;
            if (parameter is string paramStr && paramStr.Equals("Invert", StringComparison.OrdinalIgnoreCase))
            {
                invert = true;
            }
            
            bool isNull = value == null;
            return invert ? !isNull : isNull;
        }
        
        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            return Avalonia.Data.BindingOperations.DoNothing;
        }
    }
    
    /// <summary>
    /// Конвертер для форматирования даты
    /// </summary>
    public class DateFormatConverter : IValueConverter
    {
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            if (value is DateTime date)
            {
                string format = parameter as string ?? "dd.MM.yyyy HH:mm";
                return date.ToString(format, culture);
            }
            return string.Empty;
        }
        
        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            return Avalonia.Data.BindingOperations.DoNothing;
        }
    }
    
    /// <summary>
    /// Конвертер для отображения размера файлов
    /// </summary>
    public class FileSizeConverter : IValueConverter
    {
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            if (value is long bytes)
            {
                if (bytes == 0) return "0 Б";
                
                string[] sizes = { "Б", "КБ", "МБ", "ГБ", "ТБ" };
                double len = bytes;
                int order = 0;
                
                while (len >= 1024 && order < sizes.Length - 1)
                {
                    order++;
                    len /= 1024;
                }
                
                return $"{len:0.##} {sizes[order]}";
            }
            return string.Empty;
        }
        
        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            return Avalonia.Data.BindingOperations.DoNothing;
        }
    }
    
    /// <summary>
    /// Конвертер для определения иконки файла
    /// </summary>
    public class FileIconConverter : IValueConverter
    {
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            if (value is FileItem fileItem)
            {
                return fileItem.Icon;
            }
            else if (value is bool isDirectory)
            {
                return isDirectory ? "📁" : "📄";
            }
            else if (value is string extension)
            {
                return GetIconForExtension(extension);
            }
            return "📄";
        }
        
        private string GetIconForExtension(string extension)
        {
            return extension.ToLower() switch
            {
                ".txt" or ".md" => "📄",
                ".pdf" => "📕",
                ".doc" or ".docx" => "📝",
                ".xls" or ".xlsx" => "📊",
                ".jpg" or ".jpeg" or ".png" or ".gif" or ".bmp" => "🖼️",
                ".mp3" or ".wav" or ".flac" => "🎵",
                ".mp4" or ".avi" or ".mkv" or ".mov" => "🎬",
                ".zip" or ".rar" or ".7z" or ".tar" or ".gz" => "🗜️",
                ".exe" or ".msi" => "⚙️",
                ".cs" or ".java" or ".cpp" or ".py" => "💻",
                ".html" or ".htm" or ".css" or ".js" => "🌐",
                _ => "📄"
            };
        }
        
        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            return Avalonia.Data.BindingOperations.DoNothing;
        }
    }

    /// <summary>
    /// Конвертер для проверки содержит ли текст подстроку
    /// </summary>
    public class TextContainsConverter : IValueConverter
    {
        public static readonly TextContainsConverter Instance = new TextContainsConverter();

        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            if (value is string text && parameter is string search)
            {
                return text.Contains(search, StringComparison.OrdinalIgnoreCase);
            }
            return false;
        }

        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    /// <summary>
    /// Конвертер для проверки не содержит ли текст подстроку
    /// </summary>
    public class TextNotContainsConverter : IValueConverter
    {
        public static readonly TextNotContainsConverter Instance = new TextNotContainsConverter();

        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            if (value is string text && parameter is string search)
            {
                return !text.Contains(search, StringComparison.OrdinalIgnoreCase);
            }
            return true;
        }

        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}