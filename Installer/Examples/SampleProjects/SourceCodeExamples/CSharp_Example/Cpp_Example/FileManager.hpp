#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <filesystem>
#include <cstdint>
#include <functional>

// Forward declarations
struct FileInfo;
struct DirectoryStats;

/**
 * @enum SortBy
 * @brief Sorting criteria for file listings
 */
enum class SortBy {
    NAME,       ///< Sort by filename
    SIZE,       ///< Sort by file size
    DATE,       ///< Sort by modification date
    TYPE        ///< Sort by file type/extension
};

/**
 * @enum DisplayMode
 * @brief Display modes for file listings
 */
enum class DisplayMode {
    LIST,       ///< Simple list view
    DETAILS,    ///< Detailed view with attributes
    GRID,       ///< Grid/icon view
    TREE        ///< Tree view of directory structure
};

/**
 * @struct FileInfo
 * @brief Information about a file or directory
 */
struct FileInfo {
    std::string name;           ///< Filename
    std::string path;           ///< Full path
    std::string extension;      ///< File extension (for files)
    
    uintmax_t size;             ///< File size in bytes
    time_t modifiedTime;        ///< Last modification time
    time_t createdTime;         ///< Creation time
    
    bool isDirectory;           ///< True if this is a directory
    bool isRegularFile;         ///< True if this is a regular file
    bool isSymlink;             ///< True if this is a symbolic link
    bool isHidden;              ///< True if file is hidden
    bool isReadOnly;            ///< True if file is read-only
    bool isSystem;              ///< True if system file
    bool isArchive;             ///< True if archive file (Windows)
    
    // Unix-specific attributes
    mode_t permissions;         ///< File permissions (Unix)
    uid_t ownerId;              ///< Owner user ID (Unix)
    gid_t groupId;              ///< Owner group ID (Unix)
    
    // Default constructor
    FileInfo() 
        : size(0), 
            modifiedTime(0), 
            createdTime(0),
            isDirectory(false),
            isRegularFile(false),
            isSymlink(false),
            isHidden(false),
            isReadOnly(false),
            isSystem(false),
            isArchive(false),
            permissions(0),
            ownerId(0),
            groupId(0) {}
};

/**
 * @struct DirectoryStats
 * @brief Statistics about a directory
 */
struct DirectoryStats {
    size_t fileCount = 0;               ///< Number of files
    size_t directoryCount = 0;          ///< Number of directories
    uintmax_t totalSize = 0;            ///< Total size of all files
    size_t errorCount = 0;              ///< Number of access errors
    
    /// Map of file extensions to counts
    std::unordered_map<std::string, size_t> fileTypes;
    
    /// Get formatted string representation
    std::string toString() const {
        return "Files: " + std::to_string(fileCount) + 
               ", Dirs: " + std::to_string(directoryCount) +
               ", Size: " + std::to_string(totalSize) + " bytes";
    }
};

/**
 * @class FileManager
 * @brief Main file manager class for navigating and managing files
 */
class FileManager {
public:
    // ==================== Constructors/Destructor ====================
    
    /**
     * @brief Default constructor - starts at current directory
     */
    FileManager();
    
    /**
     * @brief Constructor with custom starting path
     * @param startPath Initial directory path
     */
    explicit FileManager(const std::string& startPath);
    
    /**
     * @brief Destructor
     */
    ~FileManager();
    
    // Delete copy constructor and assignment operator
    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;
    
    // ==================== Navigation Methods ====================
    
    /**
     * @brief Change current directory
     * @param path Directory path (relative or absolute)
     * @return true if successful, false otherwise
     */
    bool changeDirectory(const std::string& path);
    
    /**
     * @brief Get current directory path
     * @return Current path as string
     */
    std::string getCurrentPath() const;
    
    /**
     * @brief Go to parent directory
     * @return true if successful, false if already at root
     */
    bool goToParent();
    
    /**
     * @brief Go to root directory
     * @return true if successful
     */
    bool goToRoot();
    
    /**
     * @brief Go to user's home directory
     * @return true if successful
     */
    bool goToHome();
    
    /**
     * @brief Get navigation history
     * @return Vector of previously visited directories
     */
    std::vector<std::string> getHistory() const { return directoryHistory; }
    
    /**
     * @brief Clear navigation history
     */
    void clearHistory() { directoryHistory.clear(); }
    
    // ==================== File Listing Methods ====================
    
    /**
     * @brief List files in current directory
     * @param showHidden Include hidden files
     * @param sortBy Sorting criterion
     * @param reverse Reverse sort order
     * @return Vector of FileInfo objects
     */
    std::vector<FileInfo> listFiles(
        bool showHidden = false,
        SortBy sortBy = SortBy::NAME,
        bool reverse = false
    ) const;
    
    /**
     * @brief Display files using specified display mode
     * @param files Files to display
     * @param mode Display mode to use
     */
    void displayFiles(
        const std::vector<FileInfo>& files,
        DisplayMode mode = DisplayMode::LIST
    ) const;
    
    /**
     * @brief Display directory tree
     */
    void displayTree() const;
    
    /**
     * @brief Sort vector of files
     * @param files Files to sort (modified in-place)
     * @param sortBy Sorting criterion
     * @param reverse Reverse sort order
     */
    void sortFiles(
        std::vector<FileInfo>& files,
        SortBy sortBy = SortBy::NAME,
        bool reverse = false
    ) const;
    
    // ==================== File Information Methods ====================
    
    /**
     * @brief Get detailed information about a file
     * @param filename Name of the file (relative to current directory)
     * @return FileInfo structure
     */
    FileInfo getFileInfo(const std::string& filename) const;
    
    /**
     * @brief Display formatted file information
     * @param info FileInfo to display
     */
    void displayFileInfo(const FileInfo& info) const;
    
    // ==================== File Operations ====================
    
    /**
     * @brief Check if file exists
     * @param filename File to check
     * @return true if file exists
     */
    bool fileExists(const std::string& filename) const;
    
    /**
     * @brief Create a new directory
     * @param dirname Directory name
     * @return true if successful
     */
    bool createDirectory(const std::string& dirname);
    
    /**
     * @brief Create a new empty file
     * @param filename File name
     * @return true if successful
     */
    bool createFile(const std::string& filename);
    
    /**
     * @brief Rename a file or directory
     * @param oldName Current name
     * @param newName New name
     * @return true if successful
     */
    bool rename(const std::string& oldName, const std::string& newName);
    
    /**
     * @brief Delete a file
     * @param filename File to delete
     * @param permanent If true, delete permanently (no recycle bin)
     * @return true if successful
     */
    bool deleteFile(const std::string& filename, bool permanent = false);
    
    /**
     * @brief Copy a file
     * @param source Source file
     * @param destination Destination path
     * @param overwrite Overwrite if exists
     * @return true if successful
     */
    bool copyFile(const std::string& source, const std::string& destination, bool overwrite = false);
    
    /**
     * @brief Move a file
     * @param source Source file
     * @param destination Destination path
     * @return true if successful
     */
    bool moveFile(const std::string& source, const std::string& destination);
    
    // ==================== Search Methods ====================
    
    /**
     * @brief Search for files by name pattern
     * @param pattern Search pattern (supports wildcards)
     * @param recursive Search in subdirectories
     * @return Vector of matching file paths
     */
    std::vector<std::string> searchFiles(
        const std::string& pattern,
        bool recursive = false
    ) const;
    
    /**
     * @brief Search for files by content
     * @param text Text to search for
     * @param extensions File extensions to search (empty for all)
     * @return Vector of matching file paths
     */
    std::vector<std::string> searchInFiles(
        const std::string& text,
        const std::vector<std::string>& extensions = {}
    ) const;
    
    // ==================== Bookmarks Management ====================
    
    /**
     * @brief Add a directory bookmark
     * @param name Bookmark name
     * @param path Directory path
     * @return true if added successfully
     */
    bool addBookmark(const std::string& name, const std::string& path);
    
    /**
     * @brief Remove a bookmark
     * @param name Bookmark name
     * @return true if removed successfully
     */
    bool removeBookmark(const std::string& name);
    
    /**
     * @brief Navigate to a bookmarked directory
     * @param name Bookmark name
     * @return true if navigation successful
     */
    bool goToBookmark(const std::string& name);
    
    /**
     * @brief List all bookmarks
     */
    void listBookmarks() const;
    
    /**
     * @brief Get all bookmarks
     * @return Map of bookmark names to paths
     */
    std::map<std::string, std::string> getBookmarks() const { return bookmarks; }
    
    // ==================== Statistics Methods ====================
    
    /**
     * @brief Get statistics for current directory
     * @return DirectoryStats structure
     */
    DirectoryStats getDirectoryStats() const;
    
    /**
     * @brief Display directory statistics
     */
    void displayStats() const;
    
    /**
     * @brief Calculate directory size recursively
     * @param path Directory path
     * @return Size in bytes
     */
    uintmax_t calculateDirectorySize(const std::string& path) const;
    
    // ==================== Utility Methods ====================
    
    /**
     * @brief Format file size in human-readable format
     * @param bytes Size in bytes
     * @return Formatted string (e.g., "1.5 MB")
     */
    static std::string formatSize(uintmax_t bytes);
    
    /**
     * @brief Clear terminal screen
     */
    void clearScreen() const;
    
    /**
     * @brief Get recent directories
     * @return Vector of recent directory paths
     */
    std::vector<std::string> getRecentDirectories() const { return recentDirectories; }
    
    /**
     * @brief Get platform-specific root directory
     * @return Root directory path
     */
    std::string getSystemRoot() const { return systemRoot; }
    
    /**
     * @brief Get user home directory
     * @return Home directory path
     */
    std::string getUserHome() const { return userProfile; }
    
    /**
     * @brief Check if current user has write permissions
     * @return true if user can write to current directory
     */
    bool hasWritePermission() const;
    
    // ==================== Event Callbacks ====================
    
    /**
     * @brief Callback type for directory change events
     */
    using DirectoryChangeCallback = std::function<void(const std::string& oldPath, const std::string& newPath)>;
    
    /**
     * @brief Set callback for directory change events
     * @param callback Function to call when directory changes
     */
    void setDirectoryChangeCallback(DirectoryChangeCallback callback) {
        directoryChangeCallback = callback;
    }
    
    /**
     * @brief Callback type for file operation events
     */
    using FileOperationCallback = std::function<void(const std::string& operation, const std::string& filePath, bool success)>;
    
    /**
     * @brief Set callback for file operation events
     * @param callback Function to call on file operations
     */
    void setFileOperationCallback(FileOperationCallback callback) {
        fileOperationCallback = callback;
    }
    
private:
    // ==================== Private Constants ====================
    static constexpr size_t MAX_HISTORY_SIZE = 100;          ///< Max navigation history entries
    static constexpr size_t MAX_RECENT_DIRECTORIES = 20;     ///< Max recent directories
    static constexpr size_t MAX_SEARCH_RESULTS = 1000;       ///< Max search results
    
    // ==================== Private Member Variables ====================
    std::filesystem::path currentPath;                       ///< Current working directory
    std::vector<std::string> directoryHistory;               ///< Navigation history
    std::vector<std::string> recentDirectories;              ///< Recently visited directories
    std::map<std::string, std::string> bookmarks;            ///< Directory bookmarks
    
    std::string systemRoot;                                  ///< System root directory
    std::string userProfile;                                 ///< User home directory
    
    // Event callbacks
    DirectoryChangeCallback directoryChangeCallback;         ///< Directory change callback
    FileOperationCallback fileOperationCallback;             ///< File operation callback
    
    // ==================== Private Helper Methods ====================
    
    /**
     * @brief Initialize terminal/console settings
     */
    void initializeTerminal();
    
    /**
     * @brief Cleanup terminal/console settings
     */
    void cleanupTerminal();
    
    /**
     * @brief Load navigation history from file
     */
    void loadHistory();
    
    /**
     * @brief Save navigation history to file
     */
    void saveHistory() const;
    
    /**
     * @brief Load bookmarks from file
     */
    void loadBookmarks();
    
    /**
     * @brief Save bookmarks to file
     */
    void saveBookmarks() const;
    
    /**
     * @brief Add directory to recent list
     * @param path Directory path
     */
    void addToRecentDirectories(const std::string& path);
    
    /**
     * @brief Display files in list mode
     * @param files Files to display
     */
    void displayList(const std::vector<FileInfo>& files) const;
    
    /**
     * @brief Display files in detailed mode
     * @param files Files to display
     */
    void displayDetails(const std::vector<FileInfo>& files) const;
    
    /**
     * @brief Display files in grid mode
     * @param files Files to display
     */
    void displayGrid(const std::vector<FileInfo>& files) const;
    
    /**
     * @brief Recursive helper for tree display
     * @param path Current path
     * @param depth Current depth
     * @param prefix Prefix for tree lines
     * @param isLast If this is the last item at this level
     */
    void displayTreeRecursive(
        const std::filesystem::path& path,
        int depth,
        const std::string& prefix,
        bool isLast
    ) const;
    
    /**
     * @brief Execute file operation with callback
     * @tparam Func Operation function type
     * @param operation Operation name for callback
     * @param func Operation function
     * @param args Operation arguments
     * @return true if successful
     */
    template<typename Func, typename... Args>
    bool executeFileOperation(const std::string& operation, Func&& func, Args&&... args) {
        bool success = false;
        try {
            success = func(std::forward<Args>(args)...);
        } catch (const std::exception& e) {
            std::cerr << "Error in " << operation << ": " << e.what() << std::endl;
            success = false;
        }
        
        if (fileOperationCallback) {
            fileOperationCallback(operation, "", success);
        }
        
        return success;
    }
    
    /**
     * @brief Validate filename for current platform
     * @param filename Filename to validate
     * @return true if filename is valid
     */
    bool validateFilename(const std::string& filename) const;
    
    /**
     * @brief Platform-specific file deletion
     * @param path File path
     * @param permanent Permanent deletion flag
     * @return true if successful
     */
    bool platformDeleteFile(const std::filesystem::path& path, bool permanent);
    
    /**
     * @brief Platform-specific file copy
     * @param source Source path
     * @param destination Destination path
     * @return true if successful
     */
    bool platformCopyFile(const std::filesystem::path& source, const std::filesystem::path& destination);
};

/**
 * @brief Utility function to get file extension
 * @param filename Filename or path
 * @return File extension (including dot)
 */
std::string getFileExtension(const std::string& filename);

/**
 * @brief Utility function to get filename without extension
 * @param filename Filename or path
 * @return Filename without extension
 */
std::string getFilenameWithoutExtension(const std::string& filename);

/**
 * @brief Utility function to check if path is absolute
 * @param path Path to check
 * @return true if path is absolute
 */
bool isAbsolutePath(const std::string& path);

/**
 * @brief Utility function to normalize path separators
 * @param path Path to normalize
 * @return Normalized path
 */
std::string normalizePath(const std::string& path);

#endif // FILEMANAGER_HPP