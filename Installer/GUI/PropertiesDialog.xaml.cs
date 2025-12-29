using System;
using System.IO;
using System.Globalization;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Media.Imaging;

namespace FileManager
{
    public partial class PropertiesDialog : Window
    {
        // ViewModel для биндинга данных
        public class PropertiesViewModel
        {
            public string Name { get; set; } = string.Empty;
            public string FullPath { get; set; } = string.Empty;
            public string TypeDisplay { get; set; } = string.Empty;
            public string FormattedSize { get; set; } = string.Empty;
            public string LastModifiedDisplay { get; set; } = string.Empty;
            public Bitmap? Icon { get; set; }  
        }

        private readonly PropertiesViewModel _viewModel;
        private readonly string _filePath;

        // ПУБЛИЧНЫЙ конструктор без параметров (ДОБАВЬТЕ ЭТОТ)
        public PropertiesDialog()
        {
            InitializeComponent();
            _viewModel = new PropertiesViewModel();
            _filePath = string.Empty;
            DataContext = _viewModel;
        }

        public PropertiesDialog(string filePath)
        {
            _filePath = filePath;
            _viewModel = new PropertiesViewModel();
            DataContext = _viewModel;

            InitializeComponent();
            LoadProperties();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }

        private void LoadProperties()
        {
            try
            {
                if (File.Exists(_filePath))
                {
                    var fileInfo = new FileInfo(_filePath);
                    _viewModel.Name = fileInfo.Name;
                    _viewModel.FullPath = fileInfo.FullName;
                    _viewModel.TypeDisplay = "Файл";
                    _viewModel.FormattedSize = FormatFileSize(fileInfo.Length);
                    _viewModel.LastModifiedDisplay = fileInfo.LastWriteTime.ToString("g", CultureInfo.CurrentCulture);
                    _viewModel.Icon = LoadIconForFile(_filePath);
                    
                    // Установка атрибутов
                    var chkReadOnly = this.FindControl<CheckBox>("chkReadOnly");
                    var chkHidden = this.FindControl<CheckBox>("chkHidden");
                    var chkArchive = this.FindControl<CheckBox>("chkArchive");
                    var txtStatistics = this.FindControl<TextBlock>("txtStatistics");

                    if (chkReadOnly != null)
                        chkReadOnly.IsChecked = (fileInfo.Attributes & FileAttributes.ReadOnly) != 0;
                    
                    if (chkHidden != null)
                        chkHidden.IsChecked = (fileInfo.Attributes & FileAttributes.Hidden) != 0;
                    
                    if (chkArchive != null)
                        chkArchive.IsChecked = (fileInfo.Attributes & FileAttributes.Archive) != 0;
                    
                    if (txtStatistics != null)
                        txtStatistics.Text = $"Создан: {fileInfo.CreationTime:g}\n" +
                                           $"Последний доступ: {fileInfo.LastAccessTime:g}\n" +
                                           $"Размер: {fileInfo.Length} байт";
                }
                else if (Directory.Exists(_filePath))
                {
                    var dirInfo = new DirectoryInfo(_filePath);
                    _viewModel.Name = dirInfo.Name;
                    _viewModel.FullPath = dirInfo.FullName;
                    _viewModel.TypeDisplay = "Папка";
                    _viewModel.FormattedSize = "—";
                    _viewModel.LastModifiedDisplay = dirInfo.LastWriteTime.ToString("g", CultureInfo.CurrentCulture);
                    _viewModel.Icon = LoadIconForFolder();
                    
                    var txtStatistics = this.FindControl<TextBlock>("txtStatistics");
                    if (txtStatistics != null)
                    {
                        try
                        {
                            var files = Directory.GetFiles(_filePath, "*", SearchOption.AllDirectories);
                            var dirs = Directory.GetDirectories(_filePath, "*", SearchOption.AllDirectories);
                            txtStatistics.Text = $"Папок: {dirs.Length}\n" +
                                               $"Файлов: {files.Length}\n" +
                                               $"Последнее изменение: {dirInfo.LastWriteTime:g}";
                        }
                        catch (UnauthorizedAccessException)
                        {
                            txtStatistics.Text = "Нет доступа к статистике папки";
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                _viewModel.Name = "Ошибка";
                _viewModel.TypeDisplay = $"Ошибка: {ex.Message}";
            }
        }

        private string FormatFileSize(long bytes)
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

        private Bitmap? LoadIconForFile(string filePath) 
        {
            try
            {
                // Здесь можно добавить логику загрузки иконок
                // Пока возвращаем null или стандартную иконку
                return null;
            }
            catch
            {
                return null;
            }
        }

        private Bitmap? LoadIconForFolder() 
        {
            try
            {
                // Стандартная иконка папки
                return null;
            }
            catch
            {
                return null;
            }
        }

        private void BtnOK_Click(object? sender, RoutedEventArgs e)
        {
            try
            {
                // Применяем изменения атрибутов
                if (File.Exists(_filePath))
                {
                    var attributes = File.GetAttributes(_filePath);
                    
                    var chkReadOnly = this.FindControl<CheckBox>("chkReadOnly");
                    var chkHidden = this.FindControl<CheckBox>("chkHidden");
                    var chkArchive = this.FindControl<CheckBox>("chkArchive");
                    
                    if (chkReadOnly?.IsChecked == true)
                        attributes |= FileAttributes.ReadOnly;
                    else
                        attributes &= ~FileAttributes.ReadOnly;
                    
                    if (chkHidden?.IsChecked == true)
                        attributes |= FileAttributes.Hidden;
                    else
                        attributes &= ~FileAttributes.Hidden;
                    
                    if (chkArchive?.IsChecked == true)
                        attributes |= FileAttributes.Archive;
                    else
                        attributes &= ~FileAttributes.Archive;
                    
                    File.SetAttributes(_filePath, attributes);
                }
            }
            catch (Exception ex)
            {
                // Можно показать сообщение об ошибке
                Console.WriteLine($"Ошибка сохранения атрибутов: {ex.Message}");
            }
            
            Close(true);
        }

        private void BtnCancel_Click(object? sender, RoutedEventArgs e)
        {
            Close(false);
        }
    }
}