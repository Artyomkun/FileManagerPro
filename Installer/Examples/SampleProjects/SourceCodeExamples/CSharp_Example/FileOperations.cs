/**
 * FileManagerPro - File Operations Module
 * File: FileOperations.cs
 * Description: Advanced file system operations for C# implementation
 * Requirements: .NET 6.0+, System.IO, System.Security
 * Last Modified: 2024-01-15
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Security;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace FileManagerPro.FileSystem
{
    /// <summary>
    /// File system item types
    /// </summary>
    public enum ItemType
    {
        File,
        Directory,
        SymbolicLink,
        Other
    }

    /// <summary>
    /// File attributes extended
    /// </summary>
    [Flags]
    public enum ExtendedFileAttributes
    {
        None = 0,
        ReadOnly = 1,
        Hidden = 2,
        System = 4,
        Archive = 32,
        Device = 64,
        Normal = 128,
        Temporary = 256,
        SparseFile = 512,
        ReparsePoint = 1024,
        Compressed = 2048,
        Offline = 4096,
        NotContentIndexed = 8192,
        Encrypted = 16384
    }

    /// <summary>
    /// File information structure
    /// </summary>
    public class FileItemInfo
    {
        public string Name { get; set; } = string.Empty;
        public string FullPath { get; set; } = string.Empty;
        public string Extension { get; set; } = string.Empty;
        public ItemType Type { get; set; }
        public long Size { get; set; }
        public DateTime Created { get; set; }
        public DateTime Modified { get; set; }
        public DateTime Accessed { get; set; }
        public ExtendedFileAttributes Attributes { get; set; }
        public string Owner { get; set; } = string.Empty;
        public string Permissions { get; set; } = string.Empty;
        public bool IsHidden => (Attributes & ExtendedFileAttributes.Hidden) != 0;
        public bool IsReadOnly => (Attributes & ExtendedFileAttributes.ReadOnly) != 0;
        public bool IsSystem => (Attributes & ExtendedFileAttributes.System) != 0;
        public bool IsSymbolicLink => (Attributes & ExtendedFileAttributes.ReparsePoint) != 0;

        public override string ToString()
        {
            return $"{Name} ({Type}, {FormatSize(Size)})";
        }

        private static string FormatSize(long bytes)
        {
            string[] sizes = { "B", "KB", "MB", "GB", "TB" };
            int order = 0;
            double size = bytes;

            while (size >= 1024 && order < sizes.Length - 1)
            {
                order++;
                size /= 1024;
            }

            return $"{size:0.##} {sizes[order]}";
        }
    }

    /// <summary>
    /// Directory statistics
    /// </summary>
    public class DirectoryStatistics
    {
        public long TotalFiles { get; set; }
        public long TotalDirectories { get; set; }
        public long TotalSize { get; set; }
        public Dictionary<string, long> FileTypes { get; set; } = new();
        public DateTime OldestFile { get; set; }
        public DateTime NewestFile { get; set; }
        public int ErrorCount { get; set; }

        public override string ToString()
        {
            return $"Files: {TotalFiles}, Directories: {TotalDirectories}, Size: {FormatSize(TotalSize)}";
        }

        private static string FormatSize(long bytes)
        {
            string[] sizes = { "B", "KB", "MB", "GB", "TB" };
            int order = 0;
            double size = bytes;

            while (size >= 1024 && order < sizes.Length - 1)
            {
                order++;
                size /= 1024;
            }

            return $"{size:0.##} {sizes[order]}";
        }
    }

    /// <summary>
    /// File operation progress report
    /// </summary>
    public class FileOperationProgress
    {
        public string CurrentFile { get; set; } = string.Empty;
        public long BytesProcessed { get; set; }
        public long TotalBytes { get; set; }
        public int FilesProcessed { get; set; }
        public int TotalFiles { get; set; }
        public bool IsIndeterminate { get; set; }
        public string Operation { get; set; } = string.Empty;
        public double Percentage => TotalBytes > 0 ? (BytesProcessed * 100.0) / TotalBytes : 0;
    }

    /// <summary>
    /// Advanced file operations with progress reporting and error handling
    /// </summary>
    public static class FileOperations
    {
        // ==================== Constants ====================
        private const int BUFFER_SIZE = 81920; // 80KB buffer for file operations
        private const int MAX_RETRY_COUNT = 3;
        private const int RETRY_DELAY_MS = 100;

        // ==================== Delegates and Events ====================
        public delegate void ProgressHandler(FileOperationProgress progress);
        public delegate void ErrorHandler(string operation, string path, Exception exception);
        public delegate void OperationCompleteHandler(string operation, bool success);

        public static event ProgressHandler? OnProgress;
        public static event ErrorHandler? OnError;
        public static event OperationCompleteHandler? OnComplete;

        // ==================== File Information Methods ====================

        /// <summary>
        /// Get detailed information about a file or directory
        /// </summary>
        public static FileItemInfo GetFileInfo(string path)
        {
            try
            {
                var info = new FileItemInfo
                {
                    FullPath = Path.GetFullPath(path),
                    Name = Path.GetFileName(path)
                };

                if (File.Exists(path))
                {
                    var fileInfo = new FileInfo(path);
                    info.Type = ItemType.File;
                    info.Size = fileInfo.Length;
                    info.Created = fileInfo.CreationTime;
                    info.Modified = fileInfo.LastWriteTime;
                    info.Accessed = fileInfo.LastAccessTime;
                    info.Extension = fileInfo.Extension;
                    info.Attributes = (ExtendedFileAttributes)fileInfo.Attributes;

                    try
                    {
                        info.Owner = File.GetAccessControl(path)
                            .GetOwner(typeof(NTAccount))
                            .ToString();
                    }
                    catch
                    {
                        info.Owner = "Unknown";
                    }
                }
                else if (Directory.Exists(path))
                {
                    var dirInfo = new DirectoryInfo(path);
                    info.Type = ItemType.Directory;
                    info.Size = 0;
                    info.Created = dirInfo.CreationTime;
                    info.Modified = dirInfo.LastWriteTime;
                    info.Accessed = dirInfo.LastAccessTime;
                    info.Attributes = (ExtendedFileAttributes)dirInfo.Attributes;

                    try
                    {
                        info.Owner = Directory.GetAccessControl(path)
                            .GetOwner(typeof(NTAccount))
                            .ToString();
                    }
                    catch
                    {
                        info.Owner = "Unknown";
                    }
                }
                else
                {
                    throw new FileNotFoundException($"Path not found: {path}");
                }

                // Get permissions string
                info.Permissions = GetPermissionsString(path);

                return info;
            }
            catch (Exception ex)
            {
                OnError?.Invoke("GetFileInfo", path, ex);
                throw;
            }
        }

        /// <summary>
        /// Get file list from directory with various options
        /// </summary>
        public static List<FileItemInfo> GetFileList(string directoryPath, 
            bool includeHidden = false, 
            bool includeSystem = false,
            string searchPattern = "*",
            SearchOption searchOption = SearchOption.TopDirectoryOnly)
        {
            var fileList = new List<FileItemInfo>();

            try
            {
                if (!Directory.Exists(directoryPath))
                    throw new DirectoryNotFoundException($"Directory not found: {directoryPath}");

                var dirInfo = new DirectoryInfo(directoryPath);

                // Get files
                foreach (var file in dirInfo.GetFiles(searchPattern, searchOption))
                {
                    if (ShouldIncludeFile(file, includeHidden, includeSystem))
                    {
                        fileList.Add(ConvertToFileItemInfo(file));
                    }
                }

                // Get directories
                foreach (var dir in dirInfo.GetDirectories(searchPattern, searchOption))
                {
                    if (ShouldIncludeFile(dir, includeHidden, includeSystem))
                    {
                        fileList.Add(ConvertToFileItemInfo(dir));
                    }
                }
            }
            catch (Exception ex)
            {
                OnError?.Invoke("GetFileList", directoryPath, ex);
                throw;
            }

            return fileList;
        }

        /// <summary>
        /// Calculate directory size recursively
        /// </summary>
        public static long CalculateDirectorySize(string path, bool includeHidden = false)
        {
            long totalSize = 0;

            try
            {
                if (!Directory.Exists(path))
                    throw new DirectoryNotFoundException($"Directory not found: {path}");

                var progress = new FileOperationProgress
                {
                    Operation = "Calculating Size",
                    IsIndeterminate = true
                };

                OnProgress?.Invoke(progress);

                totalSize = CalculateSizeRecursive(path, includeHidden);
            }
            catch (Exception ex)
            {
                OnError?.Invoke("CalculateDirectorySize", path, ex);
                throw;
            }

            return totalSize;
        }

        private static long CalculateSizeRecursive(string path, bool includeHidden)
        {
            long size = 0;

            try
            {
                // Add size of files in current directory
                foreach (var file in Directory.GetFiles(path))
                {
                    var fileInfo = new FileInfo(file);
                    if (includeHidden || !fileInfo.Attributes.HasFlag(FileAttributes.Hidden))
                    {
                        size += fileInfo.Length;
                    }
                }

                // Recursively add size of subdirectories
                foreach (var dir in Directory.GetDirectories(path))
                {
                    var dirInfo = new DirectoryInfo(dir);
                    if (includeHidden || !dirInfo.Attributes.HasFlag(FileAttributes.Hidden))
                    {
                        size += CalculateSizeRecursive(dir, includeHidden);
                    }
                }
            }
            catch (UnauthorizedAccessException)
            {
                // Skip directories we can't access
            }

            return size;
        }

        // ==================== File Copy Operations ====================

        /// <summary>
        /// Copy file with progress reporting
        /// </summary>
        public static async Task<bool> CopyFileAsync(string sourcePath, string destinationPath, 
            bool overwrite = false, CancellationToken cancellationToken = default)
        {
            int retryCount = 0;

            while (retryCount < MAX_RETRY_COUNT)
            {
                try
                {
                    var sourceFile = new FileInfo(sourcePath);
                    var destFile = new FileInfo(destinationPath);

                    if (!sourceFile.Exists)
                        throw new FileNotFoundException($"Source file not found: {sourcePath}");

                    if (destFile.Exists && !overwrite)
                        throw new IOException($"Destination file already exists: {destinationPath}");

                    // Ensure destination directory exists
                    Directory.CreateDirectory(destFile.DirectoryName!);

                    using var sourceStream = new FileStream(sourcePath, 
                        FileMode.Open, FileAccess.Read, FileShare.Read, BUFFER_SIZE, true);
                    using var destStream = new FileStream(destinationPath, 
                        FileMode.Create, FileAccess.Write, FileShare.None, BUFFER_SIZE, true);

                    var progress = new FileOperationProgress
                    {
                        Operation = "Copying",
                        CurrentFile = sourceFile.Name,
                        TotalBytes = sourceFile.Length,
                        TotalFiles = 1
                    };

                    byte[] buffer = new byte[BUFFER_SIZE];
                    long bytesRead = 0;
                    int read;

                    while ((read = await sourceStream.ReadAsync(buffer, 0, buffer.Length, cancellationToken)) > 0)
                    {
                        if (cancellationToken.IsCancellationRequested)
                            return false;

                        await destStream.WriteAsync(buffer, 0, read, cancellationToken);
                        bytesRead += read;

                        progress.BytesProcessed = bytesRead;
                        OnProgress?.Invoke(progress);
                    }

                    // Copy file attributes
                    destFile.CreationTime = sourceFile.CreationTime;
                    destFile.LastWriteTime = sourceFile.LastWriteTime;
                    destFile.LastAccessTime = sourceFile.LastAccessTime;
                    destFile.Attributes = sourceFile.Attributes;

                    OnComplete?.Invoke("Copy", true);
                    return true;
                }
                catch (Exception ex) when (retryCount < MAX_RETRY_COUNT - 1)
                {
                    retryCount++;
                    OnError?.Invoke("CopyFile", sourcePath, ex);
                    await Task.Delay(RETRY_DELAY_MS * retryCount, cancellationToken);
                }
                catch (Exception ex)
                {
                    OnError?.Invoke("CopyFile", sourcePath, ex);
                    OnComplete?.Invoke("Copy", false);
                    throw;
                }
            }

            return false;
        }

        /// <summary>
        /// Copy directory recursively with progress
        /// </summary>
        public static async Task<bool> CopyDirectoryAsync(string sourceDir, string destinationDir, 
            bool overwrite = false, CancellationToken cancellationToken = default)
        {
            try
            {
                var sourceInfo = new DirectoryInfo(sourceDir);
                if (!sourceInfo.Exists)
                    throw new DirectoryNotFoundException($"Source directory not found: {sourceDir}");

                // Create destination directory
                Directory.CreateDirectory(destinationDir);

                // Get all files and directories
                var allFiles = sourceInfo.GetFiles("*", SearchOption.AllDirectories);
                var allDirs = sourceInfo.GetDirectories("*", SearchOption.AllDirectories);

                var progress = new FileOperationProgress
                {
                    Operation = "Copying Directory",
                    TotalFiles = allFiles.Length,
                    TotalBytes = allFiles.Sum(f => f.Length)
                };

                // Create directory structure first
                foreach (var dir in allDirs)
                {
                    if (cancellationToken.IsCancellationRequested)
                        return false;

                    string relativePath = dir.FullName.Substring(sourceInfo.FullName.Length + 1);
                    string destPath = Path.Combine(destinationDir, relativePath);
                    Directory.CreateDirectory(destPath);
                }

                // Copy files
                foreach (var file in allFiles)
                {
                    if (cancellationToken.IsCancellationRequested)
                        return false;

                    string relativePath = file.FullName.Substring(sourceInfo.FullName.Length + 1);
                    string destPath = Path.Combine(destinationDir, relativePath);

                    progress.CurrentFile = file.Name;
                    progress.FilesProcessed++;
                    OnProgress?.Invoke(progress);

                    await CopyFileAsync(file.FullName, destPath, overwrite, cancellationToken);
                }

                OnComplete?.Invoke("CopyDirectory", true);
                return true;
            }
            catch (Exception ex)
            {
                OnError?.Invoke("CopyDirectory", sourceDir, ex);
                OnComplete?.Invoke("CopyDirectory", false);
                throw;
            }
        }

        // ==================== File Move Operations ====================

        /// <summary>
        /// Move file with error handling
        /// </summary>
        public static bool MoveFile(string sourcePath, string destinationPath, bool overwrite = false)
        {
            try
            {
                var sourceFile = new FileInfo(sourcePath);
                var destFile = new FileInfo(destinationPath);

                if (!sourceFile.Exists)
                    throw new FileNotFoundException($"Source file not found: {sourcePath}");

                if (destFile.Exists)
                {
                    if (overwrite)
                    {
                        destFile.Delete();
                    }
                    else
                    {
                        throw new IOException($"Destination file already exists: {destinationPath}");
                    }
                }

                // Ensure destination directory exists
                Directory.CreateDirectory(destFile.DirectoryName!);

                // Try fast move within same volume
                if (Path.GetPathRoot(sourceFile.FullName) == Path.GetPathRoot(destFile.FullName))
                {
                    sourceFile.MoveTo(destinationPath);
                }
                else
                {
                    // Cross-volume move requires copy + delete
                    sourceFile.CopyTo(destinationPath, true);
                    sourceFile.Delete();
                }

                OnComplete?.Invoke("MoveFile", true);
                return true;
            }
            catch (Exception ex)
            {
                OnError?.Invoke("MoveFile", sourcePath, ex);
                OnComplete?.Invoke("MoveFile", false);
                throw;
            }
        }

        /// <summary>
        /// Move directory recursively
        /// </summary>
        public static bool MoveDirectory(string sourceDir, string destinationDir, bool overwrite = false)
        {
            try
            {
                var sourceInfo = new DirectoryInfo(sourceDir);
                if (!sourceInfo.Exists)
                    throw new DirectoryNotFoundException($"Source directory not found: {sourceDir}");

                var destInfo = new DirectoryInfo(destinationDir);

                if (destInfo.Exists)
                {
                    if (overwrite)
                    {
                        DeleteDirectory(destinationDir, true);
                    }
                    else
                    {
                        throw new IOException($"Destination directory already exists: {destinationDir}");
                    }
                }

                // Check if it's the same volume for fast move
                if (Path.GetPathRoot(sourceInfo.FullName) == Path.GetPathRoot(destInfo.FullName))
                {
                    sourceInfo.MoveTo(destinationDir);
                }
                else
                {
                    // Cross-volume move
                    CopyDirectory(sourceDir, destinationDir, true);
                    DeleteDirectory(sourceDir, true);
                }

                OnComplete?.Invoke("MoveDirectory", true);
                return true;
            }
            catch (Exception ex)
            {
                OnError?.Invoke("MoveDirectory", sourceDir, ex);
                OnComplete?.Invoke("MoveDirectory", false);
                throw;
            }
        }

        // ==================== File Delete Operations ====================

        /// <summary>
        /// Delete file with recycle bin option (Windows only)
        /// </summary>
        public static bool DeleteFile(string path, bool permanent = false, bool toRecycleBin = false)
        {
            try
            {
                if (!File.Exists(path))
                    throw new FileNotFoundException($"File not found: {path}");

                if (toRecycleBin && !permanent)
                {
                    // Move to recycle bin (Windows only)
                    if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
                    {
                        NativeMethods.SHFileOperation(
                            new NativeMethods.SHFILEOPSTRUCT
                            {
                                wFunc = NativeMethods.FO_DELETE,
                                pFrom = path + '\0' + '\0',
                                fFlags = NativeMethods.FOF_ALLOWUNDO | NativeMethods.FOF_NOCONFIRMATION
                            }
                        );
                    }
                    else
                    {
                        // On non-Windows, just delete
                        File.Delete(path);
                    }
                }
                else
                {
                    File.Delete(path);
                }

                OnComplete?.Invoke("DeleteFile", true);
                return true;
            }
            catch (Exception ex)
            {
                OnError?.Invoke("DeleteFile", path, ex);
                OnComplete?.Invoke("DeleteFile", false);
                throw;
            }
        }

        /// <summary>
        /// Delete directory recursively
        /// </summary>
        public static bool DeleteDirectory(string path, bool recursive = true)
        {
            try
            {
                if (!Directory.Exists(path))
                    throw new DirectoryNotFoundException($"Directory not found: {path}");

                if (recursive)
                {
                    Directory.Delete(path, true);
                }
                else
                {
                    // Check if directory is empty
                    if (Directory.GetFileSystemEntries(path).Length > 0)
                        throw new IOException($"Directory is not empty: {path}");

                    Directory.Delete(path, false);
                }

                OnComplete?.Invoke("DeleteDirectory", true);
                return true;
            }
            catch (Exception ex)
            {
                OnError?.Invoke("DeleteDirectory", path, ex);
                OnComplete?.Invoke("DeleteDirectory", false);
                throw;
            }
        }

        // ==================== File Creation Operations ====================

        /// <summary>
        /// Create empty file
        /// </summary>
        public static bool CreateFile(string path, string? content = null)
        {
            try
            {
                var fileInfo = new FileInfo(path);

                // Ensure directory exists
                Directory.CreateDirectory(fileInfo.DirectoryName!);

                if (content != null)
                {
                    File.WriteAllText(path, content);
                }
                else
                {
                    // Create empty file
                    using (File.Create(path)) { }
                }

                OnComplete?.Invoke("CreateFile", true);
                return true;
            }
            catch (Exception ex)
            {
                OnError?.Invoke("CreateFile", path, ex);
                OnComplete?.Invoke("CreateFile", false);
                throw;
            }
        }

        /// <summary>
        /// Create directory with optional parents
        /// </summary>
        public static bool CreateDirectory(string path, bool createParents = true)
        {
            try
            {
                if (createParents)
                {
                    Directory.CreateDirectory(path);
                }
                else
                {
                    // Check if parent exists
                    var parent = Path.GetDirectoryName(path);
                    if (parent == null || !Directory.Exists(parent))
                        throw new DirectoryNotFoundException($"Parent directory not found: {parent}");

                    Directory.CreateDirectory(path);
                }

                OnComplete?.Invoke("CreateDirectory", true);
                return true;
            }
            catch (Exception ex)
            {
                OnError?.Invoke("CreateDirectory", path, ex);
                OnComplete?.Invoke("CreateDirectory", false);
                throw;
            }
        }

        // ==================== File Search Operations ====================

        /// <summary>
        /// Search files by name pattern
        /// </summary>
        public static List<string> SearchFiles(string directory, string pattern, 
            bool recursive = false, bool caseSensitive = false)
        {
            var results = new List<string>();

            try
            {
                if (!Directory.Exists(directory))
                    throw new DirectoryNotFoundException($"Directory not found: {directory}");

                var searchOption = recursive ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly;
                var files = Directory.GetFiles(directory, pattern, searchOption);

                if (!caseSensitive)
                {
                    // Case-insensitive search
                    var lowerPattern = pattern.ToLower();
                    results.AddRange(files.Where(f => 
                        Path.GetFileName(f).ToLower().Contains(lowerPattern)));
                }
                else
                {
                    results.AddRange(files);
                }
            }
            catch (Exception ex)
            {
                OnError?.Invoke("SearchFiles", directory, ex);
                throw;
            }

            return results;
        }

        /// <summary>
        /// Search files by content
        /// </summary>
        public static async Task<List<string>> SearchInFilesAsync(string directory, 
            string searchText, string[]? extensions = null, CancellationToken cancellationToken = default)
        {
            var results = new List<string>();

            try
            {
                if (!Directory.Exists(directory))
                    throw new DirectoryNotFoundException($"Directory not found: {directory}");

                var files = Directory.GetFiles(directory, "*", SearchOption.AllDirectories);

                if (extensions != null && extensions.Length > 0)
                {
                    files = files.Where(f => extensions.Contains(Path.GetExtension(f).ToLower())).ToArray();
                }

                var progress = new FileOperationProgress
                {
                    Operation = "Searching in Files",
                    TotalFiles = files.Length
                };

                foreach (var file in files)
                {
                    if (cancellationToken.IsCancellationRequested)
                        break;

                    try
                    {
                        progress.CurrentFile = Path.GetFileName(file);
                        progress.FilesProcessed++;
                        OnProgress?.Invoke(progress);

                        var content = await File.ReadAllTextAsync(file, cancellationToken);
                        if (content.Contains(searchText, StringComparison.OrdinalIgnoreCase))
                        {
                            results.Add(file);
                        }
                    }
                    catch (Exception ex) when (ex is UnauthorizedAccessException || 
                                               ex is IOException || 
                                               ex is SecurityException)
                    {
                        // Skip files we can't read
                        continue;
                    }
                }
            }
            catch (Exception ex)
            {
                OnError?.Invoke("SearchInFiles", directory, ex);
                throw;
            }

            return results;
        }

        // ==================== File Comparison Operations ====================

        /// <summary>
        /// Compare two files for equality
        /// </summary>
        public static async Task<bool> CompareFilesAsync(string file1, string file2, 
            CancellationToken cancellationToken = default)
        {
            try
            {
                var info1 = new FileInfo(file1);
                var info2 = new FileInfo(file2);

                // Quick checks
                if (info1.Length != info2.Length)
                    return false;

                if (info1.LastWriteTime != info2.LastWriteTime)
                    return false;

                // Compare content
                const int bufferSize = 65536; // 64KB
                var buffer1 = new byte[bufferSize];
                var buffer2 = new byte[bufferSize];

                using var stream1 = new FileStream(file1, FileMode.Open, FileAccess.Read, FileShare.Read);
                using var stream2 = new FileStream(file2, FileMode.Open, FileAccess.Read, FileShare.Read);

                int bytesRead1, bytesRead2;
                do
                {
                    if (cancellationToken.IsCancellationRequested)
                        return false;

                    bytesRead1 = await stream1.ReadAsync(buffer1, 0, bufferSize, cancellationToken);
                    bytesRead2 = await stream2.ReadAsync(buffer2, 0, bufferSize, cancellationToken);

                    if (bytesRead1 != bytesRead2)
                        return false;

                    for (int i = 0; i < bytesRead1; i++)
                    {
                        if (buffer1[i] != buffer2[i])
                            return false;
                    }

                } while (bytesRead1 > 0);

                return true;
            }
            catch (Exception ex)
            {
                OnError?.Invoke("CompareFiles", file1, ex);
                throw;
            }
        }

        // ==================== Utility Methods ====================

        private static bool ShouldIncludeFile(FileSystemInfo info, bool includeHidden, bool includeSystem)
        {
            bool isHidden = info.Attributes.HasFlag(FileAttributes.Hidden);
            bool isSystem = info.Attributes.HasFlag(FileAttributes.System);

            if (isHidden && !includeHidden)
                return false;

            if (isSystem && !includeSystem)
                return false;

            return true;
        }

        private static FileItemInfo ConvertToFileItemInfo(FileSystemInfo info)
        {
            var item = new FileItemInfo
            {
                Name = info.Name,
                FullPath = info.FullName,
                Created = info.CreationTime,
                Modified = info.LastWriteTime,
                Accessed = info.LastAccessTime,
                Attributes = (ExtendedFileAttributes)info.Attributes
            };

            if (info is FileInfo fileInfo)
            {
                item.Type = ItemType.File;
                item.Size = fileInfo.Length;
                item.Extension = fileInfo.Extension;
            }
            else if (info is DirectoryInfo dirInfo)
            {
                item.Type = ItemType.Directory;
                item.Size = 0;
            }

            // Get owner and permissions
            try
            {
                if (info is FileInfo f)
                {
                    item.Owner = f.GetAccessControl()
                        .GetOwner(typeof(NTAccount))
                        .ToString();
                }
                else if (info is DirectoryInfo d)
                {
                    item.Owner = d.GetAccessControl()
                        .GetOwner(typeof(NTAccount))
                        .ToString();
                }
            }
            catch
            {
                item.Owner = "Unknown";
            }

            item.Permissions = GetPermissionsString(info.FullName);

            return item;
        }

        private static string GetPermissionsString(string path)
        {
            try
            {
                if (File.Exists(path))
                {
                    var security = File.GetAccessControl(path);
                    var rules = security.GetAccessRules(true, true, typeof(NTAccount));
                    
                    var sb = new StringBuilder();
                    foreach (FileSystemAccessRule rule in rules)
                    {
                        sb.Append($"{rule.IdentityReference}: {rule.FileSystemRights}; ");
                    }
                    return sb.ToString();
                }
                else if (Directory.Exists(path))
                {
                    var security = Directory.GetAccessControl(path);
                    var rules = security.GetAccessRules(true, true, typeof(NTAccount));
                    
                    var sb = new StringBuilder();
                    foreach (FileSystemAccessRule rule in rules)
                    {
                        sb.Append($"{rule.IdentityReference}: {rule.FileSystemRights}; ");
                    }
                    return sb.ToString();
                }
            }
            catch
            {
                // Ignore permission errors
            }

            return "Unknown";
        }

        // Legacy synchronous copy for cross-volume move
        private static void CopyDirectory(string sourceDir, string destinationDir, bool overwrite)
        {
            var dir = new DirectoryInfo(sourceDir);
            var dirs = dir.GetDirectories();
            
            Directory.CreateDirectory(destinationDir);

            // Copy files
            foreach (var file in dir.GetFiles())
            {
                string destPath = Path.Combine(destinationDir, file.Name);
                file.CopyTo(destPath, overwrite);
            }

            // Copy subdirectories
            foreach (var subdir in dirs)
            {
                string destPath = Path.Combine(destinationDir, subdir.Name);
                CopyDirectory(subdir.FullName, destPath, overwrite);
            }
        }

        // ==================== Native Methods for Recycle Bin (Windows) ====================

        private static class NativeMethods
        {
            [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
            public struct SHFILEOPSTRUCT
            {
                public IntPtr hwnd;
                public uint wFunc;
                public string pFrom;
                public string pTo;
                public ushort fFlags;
                public bool fAnyOperationsAborted;
                public IntPtr hNameMappings;
                public string lpszProgressTitle;
            }

            public const uint FO_DELETE = 0x0003;
            public const ushort FOF_ALLOWUNDO = 0x0040;
            public const ushort FOF_NOCONFIRMATION = 0x0010;

            [DllImport("shell32.dll", CharSet = CharSet.Auto)]
            public static extern int SHFileOperation([In] ref SHFILEOPSTRUCT lpFileOp);
        }
    }
}