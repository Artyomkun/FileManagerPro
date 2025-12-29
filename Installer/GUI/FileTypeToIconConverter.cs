using Avalonia.Data;
using Avalonia.Data.Converters;
using System;
using System.Globalization;

namespace FileManager
{
    public class FileTypeToIconConverter : IValueConverter
    {
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            if (value is bool isDirectory)
            {
                return isDirectory ? "📁" : "📄";
            }
            return "📄";
        }

        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}