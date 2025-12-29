using System;
using System.IO;
using System.Text;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Threading;
using Avalonia.Markup.Xaml;
using System.Threading.Tasks;

namespace FileManager.Viewers
{
    public partial class FileViewer : UserControl
    {
        private string _filePath = string.Empty;
        
        public FileViewer()
        {
            InitializeComponent();
            InitializeControls();
        }
        
        private void InitializeControls()
        {
            // Подписываемся на изменение текста для активации кнопки сохранения
            _textContent.TextChanged += (s, e) =>
            {
                _btnSave.IsEnabled = !string.IsNullOrEmpty(_textContent?.Text);
            };
            
            // Настройка кнопок
            _btnSave.Click += BtnSave_Click;
            _btnClose.Click += (s, e) => Close();
            
            // Настройка горячих клавиш
            _textContent.KeyDown += OnPreviewKeyDown;
        }
        
        public void OpenFile(string filePath)
        {
            if (string.IsNullOrEmpty(filePath))
                throw new ArgumentNullException(nameof(filePath));
                
            _filePath = filePath;
            
            // Показываем информацию о файле
            var fileInfo = new FileInfo(filePath);
            _lblInfo.Text = $"Файл: {Path.GetFileName(filePath)} | Размер: {FormatFileSize(fileInfo.Length)}";
            
            // Загружаем содержимое файла асинхронно
            LoadFileContentAsync(filePath);
        }
        
        private async void LoadFileContentAsync(string filePath)
        {
            try
            {
                _progressBar.IsVisible = true;
                _progressBar.IsIndeterminate = true;  // Добавьте эту строку
                _textContent.IsEnabled = false;
                
                await Task.Run(async () =>
                {
                    try
                    {
                        // Проверяем размер файла (не загружаем слишком большие файлы)
                        var fileInfo = new FileInfo(filePath);
                        if (fileInfo.Length > 10 * 1024 * 1024) // 10 MB
                        {
                            await Dispatcher.UIThread.InvokeAsync(() =>
                            {
                                _textContent.Text = $"[Файл слишком большой для просмотра: {FormatFileSize(fileInfo.Length)}]\n" +
                                                    "Используйте другое приложение для просмотра больших файлов.";
                            });
                            return;
                        }
                        
                        // Определяем кодировку и читаем файл
                        var encoding = DetectFileEncoding(filePath);
                        string content;
                        
                        using (var stream = new FileStream(filePath, FileMode.Open, FileAccess.Read, FileShare.Read))
                        using (var reader = new StreamReader(stream, encoding))
                        {
                            content = await reader.ReadToEndAsync();
                        }
                        
                        // Для бинарных файлов показываем hex-представление
                        if (IsBinaryFile(content))
                        {
                            var bytes = await File.ReadAllBytesAsync(filePath);
                            content = ConvertToHexView(bytes, 256 * 1024); // Ограничиваем размер
                        }
                        
                        await Dispatcher.UIThread.InvokeAsync(() =>
                        {
                            _textContent.Text = content;
                        });
                    }
                    catch (Exception ex)
                    {
                        await Dispatcher.UIThread.InvokeAsync(() =>
                        {
                            _textContent.Text = $"[Ошибка загрузки файла: {ex.Message}]";
                        });
                    }
                });
            }
            catch (Exception ex)
            {
                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    _textContent.Text = $"[Ошибка: {ex.Message}]";
                });
            }
            finally
            {
                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    _progressBar.IsVisible = false;
                    _progressBar.IsIndeterminate = false;  // Добавьте эту строку
                    _textContent.IsEnabled = true;
                });
            }
        }
        
        private Encoding DetectFileEncoding(string filePath)
        {
            try
            {
                // Читаем первые несколько байт для определения кодировки
                var buffer = new byte[4096];
                using (var fs = new FileStream(filePath, FileMode.Open, FileAccess.Read))
                {
                    var bytesRead = fs.Read(buffer, 0, buffer.Length);
                    
                    // Проверяем BOM (Byte Order Mark)
                    if (bytesRead >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF)
                        return Encoding.UTF8;
                    
                    if (bytesRead >= 2 && buffer[0] == 0xFE && buffer[1] == 0xFF)
                        return Encoding.BigEndianUnicode;
                    
                    if (bytesRead >= 2 && buffer[0] == 0xFF && buffer[1] == 0xFE)
                        return Encoding.Unicode;
                }
                
                // Пытаемся определить кодировку по содержимому
                try
                {
                    var content = File.ReadAllText(filePath, Encoding.UTF8);
                    if (IsValidUtf8(content))
                        return Encoding.UTF8;
                }
                catch { }
                
                // По умолчанию UTF-8
                return Encoding.UTF8;
            }
            catch
            {
                return Encoding.UTF8;
            }
        }
        
        private bool IsValidUtf8(string text)
        {
            try
            {
                var bytes = Encoding.UTF8.GetBytes(text);
                var decoded = Encoding.UTF8.GetString(bytes);
                return true;
            }
            catch
            {
                return false;
            }
        }
        
        private bool IsBinaryFile(string content)
        {
            // Простая проверка на бинарный файл
            if (string.IsNullOrEmpty(content))
                return false;
                
            int nullCount = 0;
            int controlCount = 0;
            int totalChars = Math.Min(content.Length, 10000); // Проверяем только первые 10000 символов
            
            for (int i = 0; i < totalChars; i++)
            {
                char c = content[i];
                if (c == '\0') nullCount++;
                if (char.IsControl(c) && c != '\r' && c != '\n' && c != '\t') controlCount++;
                
                if (nullCount > 10 || controlCount > totalChars * 0.1)
                    return true;
            }
            
            return false;
        }
        
        private string ConvertToHexView(byte[] bytes, int maxBytes)
        {
            var sb = new StringBuilder();
            sb.AppendLine("HEX-представление бинарного файла:");
            sb.AppendLine("Адрес   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F   Текст");
            sb.AppendLine(new string('-', 70));
            
            int length = Math.Min(bytes.Length, maxBytes);
            for (int i = 0; i < length; i += 16)
            {
                // Адрес
                sb.Append($"{i:X8}  ");
                
                // HEX байты
                for (int j = 0; j < 16; j++)
                {
                    if (i + j < length)
                        sb.Append($"{bytes[i + j]:X2} ");
                    else
                        sb.Append("   ");
                    
                    if (j == 7) sb.Append(" ");
                }
                
                sb.Append(" ");
                
                // Текстовое представление
                for (int j = 0; j < 16; j++)
                {
                    if (i + j < length)
                    {
                        byte b = bytes[i + j];
                        sb.Append(b >= 32 && b <= 126 ? (char)b : '.');
                    }
                    else
                    {
                        sb.Append(' ');
                    }
                }
                
                sb.AppendLine();
                
                // Ограничиваем вывод
                if (i >= maxBytes - 16)
                {
                    sb.AppendLine($"[... выведено {maxBytes} из {bytes.Length} байт ...]");
                    break;
                }
            }
            
            return sb.ToString();
        }
        
        private async void BtnSave_Click(object? sender, RoutedEventArgs e)
        {
            try
            {
                _progressBar.IsVisible = true;
                _progressBar.IsIndeterminate = true;  // Добавьте эту строку
                _textContent.IsEnabled = false;
                
                await Task.Run(async () =>
                {
                    try
                    {
                        // Сохраняем в той же кодировке, в которой прочитали
                        var encoding = DetectFileEncoding(_filePath);
                        await File.WriteAllTextAsync(_filePath, _textContent.Text, encoding);
                        
                        await Dispatcher.UIThread.InvokeAsync(() =>
                        {
                            var fileInfo = new FileInfo(_filePath);
                            _lblInfo.Text = $"Файл: {Path.GetFileName(_filePath)} | Размер: {FormatFileSize(fileInfo.Length)} [Сохранено]";
                        });
                    }
                    catch (Exception ex)
                    {
                        await Dispatcher.UIThread.InvokeAsync(() =>
                        {
                            _textContent.Text = $"[Ошибка сохранения: {ex.Message}]";
                        });
                    }
                });
            }
            catch (Exception ex)
            {
                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    _textContent.Text = $"[Ошибка: {ex.Message}]";
                });
            }
            finally
            {
                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    _progressBar.IsVisible = false;
                    _progressBar.IsIndeterminate = false;  // Добавьте эту строку
                    _textContent.IsEnabled = true;
                });
            }
        }
        
        private void OnPreviewKeyDown(object? sender, KeyEventArgs e)
        {
            // Сохранение по Ctrl+S
            if (e.KeyModifiers == KeyModifiers.Control && e.Key == Key.S)
            {
                if (_btnSave.IsEnabled)
                {
                    BtnSave_Click(null, null!);
                    e.Handled = true;
                }
            }
            // Закрытие по ESC
            else if (e.Key == Key.Escape)
            {
                Close();
                e.Handled = true;
            }
        }
        
        public void Close()
        {
            // Здесь можно добавить проверку на несохраненные изменения
            var parent = this.Parent as ContentControl;
            if (parent != null)
            {
                parent.Content = null;
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
    }
}