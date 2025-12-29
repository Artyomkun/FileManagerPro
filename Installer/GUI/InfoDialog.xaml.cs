using System;
using System.IO;
using System.Diagnostics;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Avalonia.Threading;
using Avalonia.Input; 
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text;
using System.Threading.Tasks;

namespace n8n.FileManager.Dialogs
{
    public partial class FileSystemInfoDialog : Window
    {
        private readonly string _path;
        
        // Initialize with null-forgiving operator
        private TextBlock _txtPath = null!;
        private TextBlock _txtFileSystem = null!;
        private TextBlock _txtPermissions = null!;
        private TextBlock _txtOwner = null!;
        private TextBlock _txtGroup = null!;
        private TextBlock _txtTotalFiles = null!;
        private TextBlock _txtTotalSize = null!;
        private TextBlock _txtInodeInfo = null!;
        private TextBlock _txtFileCount = null!;
        private TextBlock _txtDirCount = null!;
        private TextBlock _txtError = null!;
        private Button _btnRefresh = null!;
        private Button _btnClose = null!;
        private Button _btnExportJson = null!;
        private ProgressBar _progressBar = null!;
        
        public FileSystemInfoDialog() : this(Environment.CurrentDirectory)
        {
            // Этот конструктор используется XAML loader-ом
        }
        
        // Основной конструктор (ваша существующая логика)
        public FileSystemInfoDialog(string path)
        {
            _path = path ?? throw new ArgumentNullException(nameof(path));
            InitializeComponent();
            InitializeComponents();
            LoadFileSystemInfo();
            
#if DEBUG
            this.AttachDevTools();
#endif
        }
        
        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
        
        private void InitializeComponents()
        {
            try
            {
                _txtPath = this.FindControl<TextBlock>("txtPath") 
                    ?? throw new InvalidOperationException("Control 'txtPath' not found");
                _txtFileSystem = this.FindControl<TextBlock>("txtFileSystem")
                    ?? throw new InvalidOperationException("Control 'txtFileSystem' not found");
                _txtPermissions = this.FindControl<TextBlock>("txtPermissions")
                    ?? throw new InvalidOperationException("Control 'txtPermissions' not found");
                _txtOwner = this.FindControl<TextBlock>("txtOwner")
                    ?? throw new InvalidOperationException("Control 'txtOwner' not found");
                _txtGroup = this.FindControl<TextBlock>("txtGroup")
                    ?? throw new InvalidOperationException("Control 'txtGroup' not found");
                _txtTotalFiles = this.FindControl<TextBlock>("txtTotalFiles")
                    ?? throw new InvalidOperationException("Control 'txtTotalFiles' not found");
                _txtTotalSize = this.FindControl<TextBlock>("txtTotalSize")
                    ?? throw new InvalidOperationException("Control 'txtTotalSize' not found");
                _txtInodeInfo = this.FindControl<TextBlock>("txtInodeInfo")
                    ?? throw new InvalidOperationException("Control 'txtInodeInfo' not found");
                _txtFileCount = this.FindControl<TextBlock>("txtFileCount")
                    ?? throw new InvalidOperationException("Control 'txtFileCount' not found");
                _txtDirCount = this.FindControl<TextBlock>("txtDirCount")
                    ?? throw new InvalidOperationException("Control 'txtDirCount' not found");
                _txtError = this.FindControl<TextBlock>("txtError")
                    ?? throw new InvalidOperationException("Control 'txtError' not found");
                _btnRefresh = this.FindControl<Button>("btnRefresh")
                    ?? throw new InvalidOperationException("Control 'btnRefresh' not found");
                _btnClose = this.FindControl<Button>("btnClose")
                    ?? throw new InvalidOperationException("Control 'btnClose' not found");
                _btnExportJson = this.FindControl<Button>("btnExportJson")
                    ?? throw new InvalidOperationException("Control 'btnExportJson' not found");
                _progressBar = this.FindControl<ProgressBar>("progressBar")
                    ?? new ProgressBar(); // Optional control
                
                // Обработчики событий
                _btnRefresh.Click += async (s, e) => await LoadFileSystemInfoAsync();
                _btnClose.Click += (s, e) => Close();
                _btnExportJson.Click += async (s, e) => await ExportToN8nAsync();
                
                // Стилизация для Linux
                if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    this.Background = Brushes.WhiteSmoke;
                    this.FontFamily = new FontFamily("Ubuntu, Noto Sans, Sans-Serif");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error initializing components: {ex.Message}");
                throw;
            }
        }
        
        private async Task LoadFileSystemInfoAsync()
        {
            try
            {
                _txtError.IsVisible = false;
                _btnRefresh.IsEnabled = false;
                
                if (string.IsNullOrEmpty(_path) || !Directory.Exists(_path))
                {
                    ShowError("Invalid or non-existent directory");
                    return;
                }
                
                // Показываем прогресс
                _progressBar.IsIndeterminate = true;
                _progressBar.IsVisible = true;
                
                // Получаем информацию о файловой системе для пути
                var fsInfo = await Task.Run(() => GetFileSystemInfoLinux(_path));
                
                // Получаем статистику по файлам и директориям
                var stats = await Task.Run(() => GetDirectoryStats(_path));
                
                // Обновляем UI
                UpdateUI(fsInfo, stats);
            }
            catch (Exception ex)
            {
                ShowError($"Error loading file system info: {ex.Message}");
            }
            finally
            {
                _btnRefresh.IsEnabled = true;
                _progressBar.IsIndeterminate = false;
                _progressBar.IsVisible = false;
            }
        }
        
        private FileSystemInfoLinux GetFileSystemInfoLinux(string path)
        {
            var info = new FileSystemInfoLinux();
            
            try
            {
                // Получаем информацию через stat
                var startInfo = new ProcessStartInfo
                {
                    FileName = "stat",
                    Arguments = $"-f -c \"%T %b %f %a %c %d %S\" \"{EscapePath(path)}\"",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    StandardOutputEncoding = Encoding.UTF8,
                    StandardErrorEncoding = Encoding.UTF8
                };
                
                using var process = Process.Start(startInfo);
                if (process == null)
                {
                    return GetFileSystemInfoViaDF(path);
                }
                
                string output = process.StandardOutput.ReadToEnd().Trim();
                string error = process.StandardError.ReadToEnd().Trim();
                process.WaitForExit();
                
                if (process.ExitCode == 0 && !string.IsNullOrEmpty(output))
                {
                    var data = output.Split(' ', StringSplitOptions.RemoveEmptyEntries);
                    if (data.Length >= 7)
                    {
                        info.FileSystemType = GetFSTypeName(data[0]);
                        info.TotalBlocks = long.Parse(data[1]);
                        info.FreeBlocks = long.Parse(data[2]);
                        info.AvailableBlocks = long.Parse(data[3]);
                        info.BlockSize = long.Parse(data[6]);
                        
                        info.TotalBytes = info.TotalBlocks * info.BlockSize;
                        info.FreeBytes = info.FreeBlocks * info.BlockSize;
                        info.AvailableBytes = info.AvailableBlocks * info.BlockSize;
                        info.UsedBytes = info.TotalBytes - info.FreeBytes;
                        
                        if (info.TotalBytes > 0)
                        {
                            info.UsagePercent = Math.Round((double)info.UsedBytes / info.TotalBytes * 100, 2);
                        }
                    }
                }
                else
                {
                    Console.WriteLine($"stat command failed: {error}");
                    return GetFileSystemInfoViaDF(path);
                }
                
                // Получаем информацию о inodes
                info.Inodes = GetInodeInfo(path);
                
                // Получаем информацию о владельце и правах
                info.Permissions = GetPermissionsString(path);
                info.Owner = GetFileOwner(path);
                info.Group = GetFileGroup(path);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Primary method failed: {ex.Message}. Trying fallback.");
                
                // Альтернативный метод через df
                info = GetFileSystemInfoViaDF(path);
            }
            
            return info;
        }
        
        private string EscapePath(string path)
        {
            // Экранируем кавычки для командной строки
            return path.Replace("\"", "\\\"");
        }
        
        private FileSystemInfoLinux GetFileSystemInfoViaDF(string path)
        {
            var info = new FileSystemInfoLinux();
            
            try
            {
                var startInfo = new ProcessStartInfo
                {
                    FileName = "df",
                    Arguments = $"-B1 -P \"{EscapePath(path)}\"",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    StandardOutputEncoding = Encoding.UTF8,
                    StandardErrorEncoding = Encoding.UTF8
                };
                
                using var process = Process.Start(startInfo);
                if (process == null)
                {
                    return info;
                }
                
                string output = process.StandardOutput.ReadToEnd();
                string error = process.StandardError.ReadToEnd();
                process.WaitForExit();
                
                if (process.ExitCode != 0)
                {
                    Console.WriteLine($"df command failed: {error}");
                    return GetDriveInfoFallback(path);
                }
                
                var lines = output.Split('\n', StringSplitOptions.RemoveEmptyEntries);
                if (lines.Length > 1)
                {
                    var data = lines[1].Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                    if (data.Length >= 6)
                    {
                        info.TotalBytes = long.Parse(data[1]);
                        info.UsedBytes = long.Parse(data[2]);
                        info.AvailableBytes = long.Parse(data[3]);
                        info.FileSystemType = data[0];
                        info.UsagePercent = double.Parse(data[4].TrimEnd('%'));
                        info.FreeBytes = info.TotalBytes - info.UsedBytes;
                    }
                }
                
                // Получаем inodes
                info.Inodes = GetInodeInfo(path);
                
            }
            catch
            {
                return GetDriveInfoFallback(path);
            }
            
            return info;
        }
        
        private FileSystemInfoLinux GetDriveInfoFallback(string path)
        {
            var info = new FileSystemInfoLinux();
            
            try
            {
                var drive = new DriveInfo(Path.GetPathRoot(path) ?? "/");
                if (drive.IsReady)
                {
                    info.TotalBytes = drive.TotalSize;
                    info.FreeBytes = drive.TotalFreeSpace;
                    info.AvailableBytes = drive.AvailableFreeSpace;
                    info.UsedBytes = drive.TotalSize - drive.TotalFreeSpace;
                    info.FileSystemType = drive.DriveFormat;
                    info.UsagePercent = Math.Round((double)info.UsedBytes / info.TotalBytes * 100, 2);
                    info.Permissions = "unknown";
                    info.Owner = Environment.UserName;
                    info.Group = "unknown";
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"DriveInfo fallback failed: {ex.Message}");
            }
            
            return info;
        }
        
        private InodeInfo? GetInodeInfo(string path)
        {
            try
            {
                var startInfo = new ProcessStartInfo
                {
                    FileName = "df",
                    Arguments = $"-i -P \"{EscapePath(path)}\"",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    StandardOutputEncoding = Encoding.UTF8,
                    StandardErrorEncoding = Encoding.UTF8
                };
                
                using var process = Process.Start(startInfo);
                if (process == null)
                {
                    return null;
                }
                
                string output = process.StandardOutput.ReadToEnd();
                string error = process.StandardError.ReadToEnd();
                process.WaitForExit();
                
                if (process.ExitCode != 0)
                {
                    Console.WriteLine($"df -i command failed: {error}");
                    return null;
                }
                
                var lines = output.Split('\n', StringSplitOptions.RemoveEmptyEntries);
                if (lines.Length > 1)
                {
                    var data = lines[1].Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                    if (data.Length >= 6)
                    {
                        return new InodeInfo
                        {
                            Total = long.Parse(data[1]),
                            Used = long.Parse(data[2]),
                            Free = long.Parse(data[3]),
                            UsagePercent = double.Parse(data[4].TrimEnd('%'))
                        };
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error getting inode info: {ex.Message}");
            }
            
            return null;
        }
        
        private DirectoryStats GetDirectoryStats(string path)
        {
            var stats = new DirectoryStats();
            
            try
            {
                // Используем find для быстрого подсчета
                string escapedPath = path.Replace("'", "'\"'\"'");
                
                // Подсчет файлов
                var startInfo = new ProcessStartInfo
                {
                    FileName = "/bin/bash",
                    Arguments = $"-c \"find '{escapedPath}' -type f 2>/dev/null | wc -l\"",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    StandardOutputEncoding = Encoding.UTF8,
                    StandardErrorEncoding = Encoding.UTF8
                };
                
                using var process = Process.Start(startInfo);
                if (process != null)
                {
                    string fileCount = process.StandardOutput.ReadToEnd().Trim();
                    string error = process.StandardError.ReadToEnd();
                    process.WaitForExit();
                    
                    if (process.ExitCode == 0 && long.TryParse(fileCount, out long count))
                    {
                        stats.FileCount = count;
                    }
                    else
                    {
                        Console.WriteLine($"find files command failed: {error}");
                    }
                }
                
                // Считаем директории
                startInfo.Arguments = $"-c \"find '{escapedPath}' -type d 2>/dev/null | wc -l\"";
                using var dirProcess = Process.Start(startInfo);
                if (dirProcess != null)
                {
                    string dirCount = dirProcess.StandardOutput.ReadToEnd().Trim();
                    string error = dirProcess.StandardError.ReadToEnd();
                    dirProcess.WaitForExit();
                    
                    if (dirProcess.ExitCode == 0 && long.TryParse(dirCount, out long dCount))
                    {
                        stats.DirectoryCount = dCount - 1; // Минус сама директория
                    }
                    else
                    {
                        Console.WriteLine($"find directories command failed: {error}");
                    }
                }
                
                stats.TotalItems = stats.FileCount + stats.DirectoryCount;
                
                // Получаем общий размер через du
                startInfo.FileName = "du";
                startInfo.Arguments = $"-sb \"{EscapePath(path)}\"";
                using var sizeProcess = Process.Start(startInfo);
                if (sizeProcess != null)
                {
                    string sizeOutput = sizeProcess.StandardOutput.ReadToEnd().Trim();
                    string error = sizeProcess.StandardError.ReadToEnd();
                    sizeProcess.WaitForExit();
                    
                    if (sizeProcess.ExitCode == 0)
                    {
                        var sizeParts = sizeOutput.Split('\t');
                        if (sizeParts.Length > 0 && long.TryParse(sizeParts[0], out long size))
                        {
                            stats.TotalSizeBytes = size;
                        }
                    }
                    else
                    {
                        Console.WriteLine($"du command failed: {error}");
                        // Fallback: рекурсивный подсчет размера
                        stats.TotalSizeBytes = CalculateDirectorySizeRecursive(path);
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error using find/du commands: {ex.Message}");
                // Fallback: рекурсивный обход на C#
                CountFilesAndDirsRecursive(path, ref stats);
            }
            
            return stats;
        }
        
        private long CalculateDirectorySizeRecursive(string path)
        {
            long size = 0;
            try
            {
                foreach (var file in Directory.GetFiles(path))
                {
                    try
                    {
                        var fileInfo = new FileInfo(file);
                        size += fileInfo.Length;
                    }
                    catch { }
                }
                
                foreach (var dir in Directory.GetDirectories(path))
                {
                    size += CalculateDirectorySizeRecursive(dir);
                }
            }
            catch { }
            
            return size;
        }
        
        private void CountFilesAndDirsRecursive(string path, ref DirectoryStats stats)
        {
            try
            {
                foreach (var file in Directory.GetFiles(path))
                {
                    stats.FileCount++;
                    try
                    {
                        var fileInfo = new FileInfo(file);
                        stats.TotalSizeBytes += fileInfo.Length;
                    }
                    catch { }
                }
                
                foreach (var dir in Directory.GetDirectories(path))
                {
                    stats.DirectoryCount++;
                    CountFilesAndDirsRecursive(dir, ref stats);
                }
                
                stats.TotalItems = stats.FileCount + stats.DirectoryCount;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error counting files recursively: {ex.Message}");
            }
        }
        
        private void UpdateUI(FileSystemInfoLinux fsInfo, DirectoryStats stats)
        {
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                _txtPath.Text = $"Path: {_path}";
                _txtFileSystem.Text = $"File System: {fsInfo.FileSystemType}";
                _txtPermissions.Text = $"Permissions: {fsInfo.Permissions}";
                _txtOwner.Text = $"Owner: {fsInfo.Owner}";
                _txtGroup.Text = $"Group: {fsInfo.Group}";
                
                _txtTotalSize.Text = $"Total Size: {FormatBytes(stats.TotalSizeBytes)}";
                _txtTotalFiles.Text = $"Total Items: {stats.TotalItems:N0}";
                _txtFileCount.Text = $"Files: {stats.FileCount:N0}";
                _txtDirCount.Text = $"Directories: {stats.DirectoryCount:N0}";
                
                if (fsInfo.Inodes != null)
                {
                    _txtInodeInfo.Text = $"Inodes: {fsInfo.Inodes.Used:N0}/{fsInfo.Inodes.Total:N0} ({fsInfo.Inodes.UsagePercent:F1}%)";
                }
                else
                {
                    _txtInodeInfo.Text = "Inode information not available";
                }
                
                // Добавляем информацию о использовании диска
                if (fsInfo.TotalBytes > 0)
                {
                    _txtTotalFiles.Text += $"\nDisk Usage: {FormatBytes(fsInfo.UsedBytes)} / {FormatBytes(fsInfo.TotalBytes)} ({fsInfo.UsagePercent:F1}%)";
                }
            });
        }
        
        private string GetFSTypeName(string typeCode)
        {
            return typeCode switch
            {
                "ext2" => "EXT2",
                "ext3" => "EXT3",
                "ext4" => "EXT4",
                "xfs" => "XFS",
                "btrfs" => "BTRFS",
                "zfs" => "ZFS",
                "ntfs" => "NTFS",
                "vfat" => "VFAT/FAT32",
                "tmpfs" => "Temporary File System",
                "proc" => "Proc File System",
                "sysfs" => "Sys File System",
                "devtmpfs" => "Device File System",
                "fuseblk" => "FUSE Block Device",
                "overlay" => "Overlay File System",
                _ => typeCode
            };
        }
        
        private string GetPermissionsString(string path)
        {
            try
            {
                var startInfo = new ProcessStartInfo
                {
                    FileName = "stat",
                    Arguments = $"-c \"%A\" \"{EscapePath(path)}\"",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    StandardOutputEncoding = Encoding.UTF8
                };
                
                using var process = Process.Start(startInfo);
                if (process == null)
                {
                    return "unknown";
                }
                
                string output = process.StandardOutput.ReadToEnd().Trim();
                string error = process.StandardError.ReadToEnd();
                process.WaitForExit();
                
                if (process.ExitCode == 0 && !string.IsNullOrEmpty(output))
                {
                    return output;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error getting permissions: {ex.Message}");
            }
            
            return "unknown";
        }
        
        private string GetFileOwner(string path)
        {
            try
            {
                var startInfo = new ProcessStartInfo
                {
                    FileName = "stat",
                    Arguments = $"-c \"%U\" \"{EscapePath(path)}\"",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    StandardOutputEncoding = Encoding.UTF8
                };
                
                using var process = Process.Start(startInfo);
                if (process == null)
                {
                    return Environment.UserName;
                }
                
                string output = process.StandardOutput.ReadToEnd().Trim();
                process.WaitForExit();
                
                if (!string.IsNullOrEmpty(output))
                {
                    return output;
                }
            }
            catch { }
            
            return Environment.UserName;
        }
        
        private string GetFileGroup(string path)
        {
            try
            {
                var startInfo = new ProcessStartInfo
                {
                    FileName = "stat",
                    Arguments = $"-c \"%G\" \"{EscapePath(path)}\"",
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    StandardOutputEncoding = Encoding.UTF8
                };
                
                using var process = Process.Start(startInfo);
                if (process == null)
                {
                    return "unknown";
                }
                
                string output = process.StandardOutput.ReadToEnd().Trim();
                process.WaitForExit();
                
                if (!string.IsNullOrEmpty(output))
                {
                    return output;
                }
            }
            catch { }
            
            return "unknown";
        }
        
        private string FormatBytes(long bytes)
        {
            string[] sizes = { "B", "KB", "MB", "GB", "TB", "PB" };
            if (bytes == 0) return "0 B";
            
            int place = Convert.ToInt32(Math.Floor(Math.Log(bytes, 1024)));
            double num = Math.Round(bytes / Math.Pow(1024, place), 2);
            
            return $"{num:0.##} {sizes[place]}";
        }
        
        private void ShowError(string message)
        {
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                _txtError.Text = message;
                _txtError.IsVisible = true;
            });
        }
        
        private async Task ExportToN8nAsync()
        {
            try
            {
                _btnExportJson.IsEnabled = false;
                _txtError.IsVisible = false;
                
                var json = GetFileSystemInfoJson();
                
                bool success = false;
                
                // Пробуем использовать Avalonia Clipboard через TopLevel
                try
                {
                    var topLevel = TopLevel.GetTopLevel(this);
                    if (topLevel?.Clipboard != null)
                    {
                        await topLevel.Clipboard.SetTextAsync(json);
                        success = true;
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"Avalonia clipboard failed: {ex.Message}");
                }
                
                // Fallback для Linux через xclip
                if (!success && RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    success = await TryCopyToClipboardLinux(json, "xclip");
                }
                
                // Fallback через xsel
                if (!success && RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
                {
                    success = await TryCopyToClipboardLinux(json, "xsel");
                }
                
                if (success)
                {
                    ShowError("✓ JSON copied to clipboard");
                }
                else
                {
                    // Сохраняем в файл
                    var tempFile = Path.Combine(Path.GetTempPath(), 
                        $"n8n_filesystem_info_{DateTime.Now:yyyyMMdd_HHmmss}.json");
                    await File.WriteAllTextAsync(tempFile, json);
                    ShowError($"JSON saved to: {tempFile}");
                    
                    // Пробуем открыть файл
                    try
                    {
                        Process.Start(new ProcessStartInfo
                        {
                            FileName = "xdg-open",
                            Arguments = $"\"{tempFile}\"",
                            UseShellExecute = false,
                            CreateNoWindow = true
                        });
                    }
                    catch
                    {
                        // Альтернативный способ открытия
                        try
                        {
                            Process.Start(new ProcessStartInfo
                            {
                                FileName = tempFile,
                                UseShellExecute = true
                            });
                        }
                        catch { }
                    }
                }
            }
            catch (Exception ex)
            {
                ShowError($"Export failed: {ex.Message}");
            }
            finally
            {
                _btnExportJson.IsEnabled = true;
            }
        }

        private async Task<bool> TryCopyToClipboardLinux(string text, string tool)
        {
            try
            {
                var startInfo = new ProcessStartInfo
                {
                    FileName = "/bin/bash",
                    RedirectStandardInput = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    StandardInputEncoding = Encoding.UTF8
                };

                if (tool == "xclip")
                {
                    startInfo.Arguments = "-c \"xclip -selection clipboard\"";
                }
                else if (tool == "xsel")
                {
                    startInfo.Arguments = "-c \"xsel --clipboard --input\"";
                }
                else
                {
                    return false;
                }

                using var process = Process.Start(startInfo);
                if (process == null || process.HasExited)
                {
                    return false;
                }

                // Записываем текст в stdin процесса
                await process.StandardInput.WriteAsync(text);
                await process.StandardInput.FlushAsync();
                process.StandardInput.Close();
                
                // Ждем завершения с таймаутом
                var completed = process.WaitForExit(3000);
                if (!completed)
                {
                    process.Kill();
                    return false;
                }
                
                return process.ExitCode == 0;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"{tool} failed: {ex.Message}");
                return false;
            }
        }
        
        private string EscapeForBash(string text)
        {
            // Экранируем одинарные кавычки для bash
            return text.Replace("'", "'\"'\"'");
        }
        
        // Основной метод для n8n интеграции
        public string GetFileSystemInfoJson()
        {
            try
            {
                var fsInfo = GetFileSystemInfoLinux(_path);
                var stats = GetDirectoryStats(_path);
                
                var response = new
                {
                    success = true,
                    timestamp = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ"),
                    path = _path,
                    filesystem = new
                    {
                        type = fsInfo.FileSystemType,
                        total_bytes = fsInfo.TotalBytes,
                        used_bytes = fsInfo.UsedBytes,
                        free_bytes = fsInfo.FreeBytes,
                        available_bytes = fsInfo.AvailableBytes,
                        usage_percent = fsInfo.UsagePercent,
                        permissions = fsInfo.Permissions,
                        owner = fsInfo.Owner,
                        group = fsInfo.Group
                    },
                    statistics = new
                    {
                        total_items = stats.TotalItems,
                        file_count = stats.FileCount,
                        directory_count = stats.DirectoryCount,
                        total_size_bytes = stats.TotalSizeBytes,
                        total_size_formatted = FormatBytes(stats.TotalSizeBytes)
                    },
                    inodes = fsInfo.Inodes != null ? new
                    {
                        total = fsInfo.Inodes.Total,
                        used = fsInfo.Inodes.Used,
                        free = fsInfo.Inodes.Free,
                        usage_percent = fsInfo.Inodes.UsagePercent
                    } : null,
                    platform = "linux"
                };
                
                return JsonSerializer.Serialize(response, new JsonSerializerOptions
                {
                    WriteIndented = true,
                    PropertyNamingPolicy = JsonNamingPolicy.CamelCase
                });
            }
            catch (Exception ex)
            {
                return JsonSerializer.Serialize(new
                {
                    success = false,
                    error = ex.Message,
                    path = _path,
                    platform = "linux"
                });
            }
        }
        
        private void LoadFileSystemInfo()
        {
            // Синхронная версия для обратной совместимости
            LoadFileSystemInfoAsync().ConfigureAwait(false);
        }
    }
    
    public class FileSystemInfoLinux
    {
        public string FileSystemType { get; set; } = "unknown";
        public long TotalBytes { get; set; }
        public long UsedBytes { get; set; }
        public long FreeBytes { get; set; }
        public long AvailableBytes { get; set; }
        public double UsagePercent { get; set; }
        public string Permissions { get; set; } = "unknown";
        public string Owner { get; set; } = "unknown";
        public string Group { get; set; } = "unknown";
        public InodeInfo? Inodes { get; set; }
        
        // Для stat команды
        public long TotalBlocks { get; set; }
        public long FreeBlocks { get; set; }
        public long AvailableBlocks { get; set; }
        public long BlockSize { get; set; }
    }
    
    public class InodeInfo
    {
        public long Total { get; set; }
        public long Used { get; set; }
        public long Free { get; set; }
        public double UsagePercent { get; set; }
    }
    
    public class DirectoryStats
    {
        public long TotalItems { get; set; }
        public long FileCount { get; set; }
        public long DirectoryCount { get; set; }
        public long TotalSizeBytes { get; set; }
    }
}