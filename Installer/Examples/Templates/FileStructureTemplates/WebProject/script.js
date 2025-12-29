/**
 * FileManagerPro - Web Interface
 * File: script.js
 * Description: JavaScript for web-based file manager interface
 * Requirements: Modern browser with ES6+ support
 * Last Modified: 2024-01-15
 */

// ==================== Constants and Configuration ====================
const CONFIG = {
    API_BASE_URL: '/api/files',
    ITEMS_PER_PAGE: 50,
    MAX_FILE_SIZE: 100 * 1024 * 1024, // 100MB
    ALLOWED_EXTENSIONS: ['.txt', '.pdf', '.jpg', '.png', '.zip'],
    THEMES: ['light', 'dark', 'system'],
    DEFAULT_THEME: 'light'
};

// ==================== State Management ====================
class FileManagerState {
    constructor() {
        this.currentPath = '/';
        this.currentFiles = [];
        this.selectedItems = new Set();
        this.sortBy = 'name';
        this.sortAsc = true;
        this.viewMode = 'grid'; // 'grid' or 'list'
        this.showHidden = false;
        this.searchQuery = '';
        this.breadcrumbs = [];
        this.isLoading = false;
        this.error = null;
        this.userPreferences = this.loadPreferences();
    }

    loadPreferences() {
        const prefs = localStorage.getItem('filemanager_prefs');
        return prefs ? JSON.parse(prefs) : {
            theme: CONFIG.DEFAULT_THEME,
            showHidden: false,
            viewMode: 'grid',
            sortBy: 'name',
            sortAsc: true
        };
    }

    savePreferences() {
        localStorage.setItem('filemanager_prefs', JSON.stringify(this.userPreferences));
    }

    updatePreferences(updates) {
        this.userPreferences = { ...this.userPreferences, ...updates };
        this.savePreferences();
        this.applyPreferences();
    }

    applyPreferences() {
        // Apply theme
        document.body.setAttribute('data-theme', this.userPreferences.theme);
        
        // Apply other preferences
        this.viewMode = this.userPreferences.viewMode;
        this.showHidden = this.userPreferences.showHidden;
        this.sortBy = this.userPreferences.sortBy;
        this.sortAsc = this.userPreferences.sortAsc;
    }
}

// ==================== UI Components ====================

class UIComponents {
    static createBreadcrumbs(path) {
        const parts = path.split('/').filter(p => p);
        const breadcrumbs = [{ name: 'Root', path: '/' }];
        
        let currentPath = '';
        for (const part of parts) {
            currentPath += '/' + part;
            breadcrumbs.push({
                name: part,
                path: currentPath
            });
        }
        
        return breadcrumbs;
    }

    static renderBreadcrumbs(breadcrumbs) {
        const container = document.getElementById('breadcrumbs');
        container.innerHTML = '';
        
        breadcrumbs.forEach((crumb, index) => {
            const crumbElement = document.createElement('span');
            
            if (index === breadcrumbs.length - 1) {
                // Last crumb (current directory)
                crumbElement.className = 'breadcrumb current';
                crumbElement.textContent = crumb.name;
            } else {
                // Clickable crumb
                crumbElement.className = 'breadcrumb';
                crumbElement.textContent = crumb.name;
                crumbElement.addEventListener('click', () => {
                    FileManager.navigateTo(crumb.path);
                });
            }
            
            container.appendChild(crumbElement);
            
            // Add separator (except for last)
            if (index < breadcrumbs.length - 1) {
                const separator = document.createElement('span');
                separator.className = 'breadcrumb-separator';
                separator.textContent = '›';
                container.appendChild(separator);
            }
        });
    }

    static createFileItem(file, isSelected = false) {
        const item = document.createElement('div');
        item.className = `file-item ${isSelected ? 'selected' : ''}`;
        item.dataset.name = file.name;
        item.dataset.type = file.type;
        item.dataset.path = file.path;
        
        // Icon based on file type
        const icon = this.getFileIcon(file);
        
        // File name with overflow handling
        const displayName = file.name.length > 20 
            ? file.name.substring(0, 17) + '...' 
            : file.name;
        
        item.innerHTML = `
            <div class="file-icon">${icon}</div>
            <div class="file-info">
                <div class="file-name" title="${file.name}">${displayName}</div>
                ${file.type === 'file' ? 
                    `<div class="file-size">${this.formatSize(file.size)}</div>` : 
                    `<div class="file-count">${file.itemCount || 0} items</div>`
                }
                <div class="file-modified">${this.formatDate(file.modified)}</div>
            </div>
            <div class="file-actions">
                <button class="btn-icon" title="More options">
                    <i class="fas fa-ellipsis-v"></i>
                </button>
            </div>
        `;
        
        // Add click handlers
        item.addEventListener('click', (e) => {
            if (!e.target.closest('.file-actions')) {
                FileManager.handleFileClick(file, e);
            }
        });
        
        item.addEventListener('dblclick', () => {
            if (file.type === 'directory') {
                FileManager.navigateTo(file.path);
            } else {
                FileManager.openFile(file);
            }
        });
        
        // Context menu
        item.addEventListener('contextmenu', (e) => {
            e.preventDefault();
            this.showContextMenu(file, e.clientX, e.clientY);
        });
        
        return item;
    }

    static createDirectoryItem(dir) {
        const item = document.createElement('div');
        item.className = 'directory-item';
        item.dataset.name = dir.name;
        item.dataset.path = dir.path;
        
        item.innerHTML = `
            <div class="dir-icon">
                <i class="fas fa-folder"></i>
            </div>
            <div class="dir-info">
                <div class="dir-name">${dir.name}</div>
                <div class="dir-count">${dir.itemCount || 0} items</div>
                <div class="dir-modified">${this.formatDate(dir.modified)}</div>
            </div>
        `;
        
        item.addEventListener('click', () => {
            FileManager.navigateTo(dir.path);
        });
        
        return item;
    }

    static getFileIcon(file) {
        const icons = {
            // Documents
            'pdf': 'file-pdf',
            'doc': 'file-word',
            'docx': 'file-word',
            'xls': 'file-excel',
            'xlsx': 'file-excel',
            'ppt': 'file-powerpoint',
            'pptx': 'file-powerpoint',
            'txt': 'file-alt',
            
            // Code
            'js': 'file-code',
            'html': 'file-code',
            'css': 'file-code',
            'py': 'file-code',
            'java': 'file-code',
            'cpp': 'file-code',
            'cs': 'file-code',
            'php': 'file-code',
            'json': 'file-code',
            'xml': 'file-code',
            
            // Media
            'jpg': 'file-image',
            'jpeg': 'file-image',
            'png': 'file-image',
            'gif': 'file-image',
            'mp3': 'file-audio',
            'wav': 'file-audio',
            'mp4': 'file-video',
            'avi': 'file-video',
            'mov': 'file-video',
            
            // Archives
            'zip': 'file-archive',
            'rar': 'file-archive',
            '7z': 'file-archive',
            'tar': 'file-archive',
            'gz': 'file-archive',
            
            // Default
            'default': 'file'
        };
        
        const extension = file.name.split('.').pop().toLowerCase();
        const iconClass = icons[extension] || icons.default;
        
        return `<i class="fas fa-${iconClass}"></i>`;
    }

    static formatSize(bytes) {
        if (bytes === 0) return '0 B';
        
        const units = ['B', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(1024));
        
        return `${(bytes / Math.pow(1024, i)).toFixed(1)} ${units[i]}`;
    }

    static formatDate(timestamp) {
        const date = new Date(timestamp);
        const now = new Date();
        const diffMs = now - date;
        const diffDays = Math.floor(diffMs / (1000 * 60 * 60 * 24));
        
        if (diffDays === 0) {
            return 'Today ' + date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
        } else if (diffDays === 1) {
            return 'Yesterday';
        } else if (diffDays < 7) {
            return `${diffDays} days ago`;
        } else {
            return date.toLocaleDateString();
        }
    }

    static showNotification(message, type = 'info', duration = 3000) {
        const notification = document.createElement('div');
        notification.className = `notification notification-${type}`;
        notification.innerHTML = `
            <div class="notification-content">
                <i class="fas fa-${type === 'success' ? 'check-circle' : 
                                 type === 'error' ? 'exclamation-circle' : 
                                 type === 'warning' ? 'exclamation-triangle' : 'info-circle'}"></i>
                <span>${message}</span>
            </div>
            <button class="notification-close">&times;</button>
        `;
        
        document.body.appendChild(notification);
        
        // Show with animation
        setTimeout(() => notification.classList.add('show'), 10);
        
        // Auto-remove
        const autoRemove = setTimeout(() => {
            notification.classList.remove('show');
            setTimeout(() => notification.remove(), 300);
        }, duration);
        
        // Close button
        notification.querySelector('.notification-close').addEventListener('click', () => {
            clearTimeout(autoRemove);
            notification.classList.remove('show');
            setTimeout(() => notification.remove(), 300);
        });
    }

    static showContextMenu(file, x, y) {
        // Remove existing context menu
        const existingMenu = document.querySelector('.context-menu');
        if (existingMenu) existingMenu.remove();
        
        // Create new context menu
        const menu = document.createElement('div');
        menu.className = 'context-menu';
        menu.style.left = `${x}px`;
        menu.style.top = `${y}px`;
        
        const items = [];
        
        if (file.type === 'directory') {
            items.push({ icon: 'folder-open', text: 'Open', action: () => FileManager.navigateTo(file.path) });
            items.push({ icon: 'download', text: 'Download', action: () => FileManager.downloadFile(file) });
            items.push({ icon: 'share', text: 'Share', action: () => FileManager.shareFile(file) });
            items.push('separator');
            items.push({ icon: 'edit', text: 'Rename', action: () => FileManager.renameFile(file) });
            items.push({ icon: 'trash', text: 'Delete', action: () => FileManager.deleteFile(file) });
            items.push({ icon: 'copy', text: 'Copy Path', action: () => FileManager.copyPath(file) });
        } else {
            items.push({ icon: 'eye', text: 'Preview', action: () => FileManager.previewFile(file) });
            items.push({ icon: 'download', text: 'Download', action: () => FileManager.downloadFile(file) });
            items.push({ icon: 'share', text: 'Share', action: () => FileManager.shareFile(file) });
            items.push('separator');
            items.push({ icon: 'edit', text: 'Rename', action: () => FileManager.renameFile(file) });
            items.push({ icon: 'trash', text: 'Delete', action: () => FileManager.deleteFile(file) });
            items.push({ icon: 'copy', text: 'Copy Path', action: () => FileManager.copyPath(file) });
        }
        
        items.forEach(item => {
            if (item === 'separator') {
                const separator = document.createElement('div');
                separator.className = 'context-menu-separator';
                menu.appendChild(separator);
            } else {
                const menuItem = document.createElement('div');
                menuItem.className = 'context-menu-item';
                menuItem.innerHTML = `
                    <i class="fas fa-${item.icon}"></i>
                    <span>${item.text}</span>
                `;
                menuItem.addEventListener('click', () => {
                    item.action();
                    menu.remove();
                });
                menu.appendChild(menuItem);
            }
        });
        
        document.body.appendChild(menu);
        
        // Close menu when clicking elsewhere
        const closeMenu = (e) => {
            if (!menu.contains(e.target)) {
                menu.remove();
                document.removeEventListener('click', closeMenu);
            }
        };
        
        setTimeout(() => document.addEventListener('click', closeMenu), 10);
    }

    static showModal(title, content, buttons = []) {
        // Remove existing modal
        const existingModal = document.querySelector('.modal');
        if (existingModal) existingModal.remove();
        
        // Create modal
        const modal = document.createElement('div');
        modal.className = 'modal';
        modal.innerHTML = `
            <div class="modal-overlay"></div>
            <div class="modal-content">
                <div class="modal-header">
                    <h3>${title}</h3>
                    <button class="modal-close">&times;</button>
                </div>
                <div class="modal-body">${content}</div>
                ${buttons.length > 0 ? `
                    <div class="modal-footer">
                        ${buttons.map(btn => 
                            `<button class="btn ${btn.class || ''}" data-action="${btn.action}">${btn.text}</button>`
                        ).join('')}
                    </div>
                ` : ''}
            </div>
        `;
        
        document.body.appendChild(modal);
        
        // Show with animation
        setTimeout(() => modal.classList.add('show'), 10);
        
        // Close button
        modal.querySelector('.modal-close').addEventListener('click', () => {
            modal.classList.remove('show');
            setTimeout(() => modal.remove(), 300);
        });
        
        // Overlay click
        modal.querySelector('.modal-overlay').addEventListener('click', () => {
            modal.classList.remove('show');
            setTimeout(() => modal.remove(), 300);
        });
        
        // Button handlers
        modal.querySelectorAll('.modal-footer .btn').forEach(button => {
            button.addEventListener('click', () => {
                const action = button.dataset.action;
                if (typeof window[action] === 'function') {
                    window[action]();
                }
                modal.classList.remove('show');
                setTimeout(() => modal.remove(), 300);
            });
        });
        
        return modal;
    }

    static showLoading(show = true) {
        const loader = document.getElementById('loading-overlay');
        if (loader) {
            loader.style.display = show ? 'flex' : 'none';
        }
    }

    static updateStatusBar(selectedCount, totalCount, totalSize) {
        const statusBar = document.getElementById('status-bar');
        if (statusBar) {
            statusBar.innerHTML = `
                <div class="status-item">
                    <span>Selected: ${selectedCount}</span>
                </div>
                <div class="status-item">
                    <span>Total: ${totalCount}</span>
                </div>
                <div class="status-item">
                    <span>Size: ${this.formatSize(totalSize)}</span>
                </div>
            `;
        }
    }
}

// ==================== File Manager Core ====================

class FileManager {
    static state = new FileManagerState();

    static async init() {
        console.log('FileManagerPro Web Interface Initializing...');
        
        // Apply saved preferences
        this.state.applyPreferences();
        
        // Initialize event listeners
        this.initEventListeners();
        
        // Load initial directory
        await this.loadDirectory('/');
        
        // Update UI
        this.updateUI();
    }

    static initEventListeners() {
        // Navigation buttons
        document.getElementById('btn-back')?.addEventListener('click', () => this.navigateBack());
        document.getElementById('btn-forward')?.addEventListener('click', () => this.navigateForward());
        document.getElementById('btn-up')?.addEventListener('click', () => this.navigateUp());
        document.getElementById('btn-refresh')?.addEventListener('click', () => this.refresh());
        document.getElementById('btn-home')?.addEventListener('click', () => this.navigateTo('/'));
        
        // View mode toggles
        document.getElementById('btn-grid-view')?.addEventListener('click', () => this.setViewMode('grid'));
        document.getElementById('btn-list-view')?.addEventListener('click', () => this.setViewMode('list'));
        
        // Sort options
        document.getElementById('sort-by')?.addEventListener('change', (e) => {
            this.state.sortBy = e.target.value;
            this.sortFiles();
        });
        
        document.getElementById('sort-order')?.addEventListener('click', (e) => {
            this.state.sortAsc = !this.state.sortAsc;
            e.target.innerHTML = this.state.sortAsc ? 
                '<i class="fas fa-sort-amount-up"></i>' : 
                '<i class="fas fa-sort-amount-down"></i>';
            this.sortFiles();
        });
        
        // Search
        const searchInput = document.getElementById('search-input');
        if (searchInput) {
            searchInput.addEventListener('input', debounce(() => {
                this.state.searchQuery = searchInput.value;
                this.filterFiles();
            }, 300));
        }
        
        // File operations
        document.getElementById('btn-upload')?.addEventListener('click', () => this.showUploadDialog());
        document.getElementById('btn-new-folder')?.addEventListener('click', () => this.createNewFolder());
        document.getElementById('btn-delete')?.addEventListener('click', () => this.deleteSelected());
        document.getElementById('btn-download')?.addEventListener('click', () => this.downloadSelected());
        
        // Settings
        document.getElementById('btn-settings')?.addEventListener('click', () => this.showSettings());
        
        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => {
            // Ctrl+A: Select all
            if (e.ctrlKey && e.key === 'a') {
                e.preventDefault();
                this.selectAll();
            }
            
            // Delete: Delete selected
            if (e.key === 'Delete') {
                this.deleteSelected();
            }
            
            // F5: Refresh
            if (e.key === 'F5') {
                e.preventDefault();
                this.refresh();
            }
            
            // Escape: Clear selection
            if (e.key === 'Escape') {
                this.clearSelection();
            }
        });
        
        // Drag and drop
        const fileArea = document.getElementById('file-area');
        if (fileArea) {
            fileArea.addEventListener('dragover', (e) => {
                e.preventDefault();
                fileArea.classList.add('drag-over');
            });
            
            fileArea.addEventListener('dragleave', () => {
                fileArea.classList.remove('drag-over');
            });
            
            fileArea.addEventListener('drop', (e) => {
                e.preventDefault();
                fileArea.classList.remove('drag-over');
                this.handleFileDrop(e.dataTransfer.files);
            });
        }
    }

    static async loadDirectory(path) {
        try {
            UIComponents.showLoading(true);
            this.state.isLoading = true;
            this.state.error = null;
            
            // Update current path
            this.state.currentPath = path;
            this.state.breadcrumbs = UIComponents.createBreadcrumbs(path);
            
            // Simulate API call (replace with real API)
            const files = await this.mockApiListFiles(path);
            
            this.state.currentFiles = files;
            this.state.selectedItems.clear();
            
            // Sort and filter
            this.sortFiles();
            this.filterFiles();
            
            // Update UI
            this.updateUI();
            
        } catch (error) {
            console.error('Error loading directory:', error);
            this.state.error = error.message;
            UIComponents.showNotification(`Error: ${error.message}`, 'error');
        } finally {
            UIComponents.showLoading(false);
            this.state.isLoading = false;
        }
    }

    static async mockApiListFiles(path) {
        // Simulate network delay
        await new Promise(resolve => setTimeout(resolve, 300));
        
        // Mock data - replace with real API call
        const mockFiles = [
            { name: 'Documents', type: 'directory', path: '/Documents', size: 0, modified: Date.now() - 86400000, itemCount: 15 },
            { name: 'Downloads', type: 'directory', path: '/Downloads', size: 0, modified: Date.now() - 172800000, itemCount: 8 },
            { name: 'Pictures', type: 'directory', path: '/Pictures', size: 0, modified: Date.now() - 259200000, itemCount: 42 },
            { name: 'Music', type: 'directory', path: '/Music', size: 0, modified: Date.now() - 345600000, itemCount: 23 },
            { name: 'Videos', type: 'directory', path: '/Videos', size: 0, modified: Date.now() - 432000000, itemCount: 7 },
            { name: 'report.pdf', type: 'file', path: '/report.pdf', size: 1500000, modified: Date.now() - 3600000 },
            { name: 'presentation.pptx', type: 'file', path: '/presentation.pptx', size: 3500000, modified: Date.now() - 7200000 },
            { name: 'budget.xlsx', type: 'file', path: '/budget.xlsx', size: 800000, modified: Date.now() - 10800000 },
            { name: 'vacation.jpg', type: 'file', path: '/vacation.jpg', size: 2500000, modified: Date.now() - 86400000 },
            { name: 'song.mp3', type: 'file', path: '/song.mp3', size: 8000000, modified: Date.now() - 172800000 },
            { name: 'movie.mp4', type: 'file', path: '/movie.mp4', size: 150000000, modified: Date.now() - 259200000 },
            { name: 'notes.txt', type: 'file', path: '/notes.txt', size: 5000, modified: Date.now() - 3600000 },
            { name: 'script.js', type: 'file', path: '/script.js', size: 15000, modified: Date.now() - 7200000 },
            { name: 'styles.css', type: 'file', path: '/styles.css', size: 8000, modified: Date.now() - 10800000 },
            { name: '.hidden_file', type: 'file', path: '/.hidden_file', size: 1000, modified: Date.now() - 86400000 }
        ];
        
        // Filter by path and hidden files
        return mockFiles.filter(file => {
            const filePath = file.path.substring(0, file.path.lastIndexOf('/')) || '/';
            const isInPath = filePath === path;
            const isHidden = file.name.startsWith('.');
            
            return isInPath && (this.state.showHidden || !isHidden);
        });
    }

    static updateUI() {
        // Update breadcrumbs
        UIComponents.renderBreadcrumbs(this.state.breadcrumbs);
        
        // Update file display
        this.renderFiles();
        
        // Update status bar
        const totalSize = this.state.currentFiles.reduce((sum, file) => sum + file.size, 0);
        UIComponents.updateStatusBar(
            this.state.selectedItems.size,
            this.state.currentFiles.length,
            totalSize
        );
        
        // Update button states
        this.updateButtonStates();
        
        // Update page title
        document.title = `FileManagerPro - ${this.state.currentPath}`;
    }

    static renderFiles() {
        const container = document.getElementById('file-container');
        if (!container) return;
        
        container.innerHTML = '';
        
        if (this.state.error) {
            container.innerHTML = `
                <div class="error-message">
                    <i class="fas fa-exclamation-triangle"></i>
                    <h3>Error Loading Directory</h3>
                    <p>${this.state.error}</p>
                    <button class="btn" onclick="FileManager.refresh()">Retry</button>
                </div>
            `;
            return;
        }
        
        if (this.state.currentFiles.length === 0) {
            container.innerHTML = `
                <div class="empty-state">
                    <i class="fas fa-folder-open"></i>
                    <h3>Empty Directory</h3>
                    <p>This directory is empty</p>
                    <button class="btn" onclick="FileManager.createNewFolder()">Create New Folder</button>
                </div>
            `;
            return;
        }
        
        // Group files by type
        const directories = this.state.currentFiles.filter(f => f.type === 'directory');
        const files = this.state.currentFiles.filter(f => f.type === 'file');
        
        // Render directories
        if (directories.length > 0) {
            const dirsSection = document.createElement('div');
            dirsSection.className = 'files-section';
            dirsSection.innerHTML = `<h4>Directories (${directories.length})</h4>`;
            
            const dirsGrid = document.createElement('div');
            dirsGrid.className = `files-grid ${this.state.viewMode}`;
            
            directories.forEach(dir => {
                const isSelected = this.state.selectedItems.has(dir.path);
                dirsGrid.appendChild(UIComponents.createFileItem(dir, isSelected));
            });
            
            dirsSection.appendChild(dirsGrid);
            container.appendChild(dirsSection);
        }
        
        // Render files
        if (files.length > 0) {
            const filesSection = document.createElement('div');
            filesSection.className = 'files-section';
            filesSection.innerHTML = `<h4>Files (${files.length})</h4>`;
            
            const filesGrid = document.createElement('div');
            filesGrid.className = `files-grid ${this.state.viewMode}`;
            
            files.forEach(file => {
                const isSelected = this.state.selectedItems.has(file.path);
                filesGrid.appendChild(UIComponents.createFileItem(file, isSelected));
            });
            
            filesSection.appendChild(filesGrid);
            container.appendChild(filesSection);
        }
    }

    static sortFiles() {
        const { sortBy, sortAsc } = this.state;
        
        this.state.currentFiles.sort((a, b) => {
            let comparison = 0;
            
            switch (sortBy) {
                case 'name':
                    comparison = a.name.localeCompare(b.name);
                    break;
                case 'size':
                    comparison = a.size - b.size;
                    break;
                case 'modified':
                    comparison = a.modified - b.modified;
                    break;
                case 'type':
                    comparison = a.type.localeCompare(b.type);
                    break;
            }
            
            // Directories first
            if (a.type !== b.type) {
                return a.type === 'directory' ? -1 : 1;
            }
            
            return sortAsc ? comparison : -comparison;
        });
        
        this.renderFiles();
    }

    static filterFiles() {
        const query = this.state.searchQuery.toLowerCase();
        
        if (!query) {
            this.renderFiles();
            return;
        }
        
        const filtered = this.state.currentFiles.filter(file => 
            file.name.toLowerCase().includes(query) ||
            file.type.toLowerCase().includes(query)
        );
        
        // Temporarily replace current files for rendering
        const originalFiles = this.state.currentFiles;
        this.state.currentFiles = filtered;
        this.renderFiles();
        this.state.currentFiles = originalFiles;
    }

    static updateButtonStates() {
        // Enable/disable buttons based on selection
        const hasSelection = this.state.selectedItems.size > 0;
        
        document.getElementById('btn-delete')?.disabled = !hasSelection;
        document.getElementById('btn-download')?.disabled = !hasSelection;
        
        // Update view mode buttons
        document.getElementById('btn-grid-view')?.classList.toggle('active', this.state.viewMode === 'grid');
        document.getElementById('btn-list-view')?.classList.toggle('active', this.state.viewMode === 'list');
    }

    // ==================== Navigation Methods ====================

    static navigateTo(path) {
        this.loadDirectory(path);
    }

    static navigateBack() {
        // Implement navigation history
        console.log('Navigate back');
        UIComponents.showNotification('Back navigation not implemented', 'info');
    }

    static navigateForward() {
        console.log('Navigate forward');
        UIComponents.showNotification('Forward navigation not implemented', 'info');
    }

    static navigateUp() {
        const currentPath = this.state.currentPath;
        if (currentPath === '/') return;
        
        const parentPath = currentPath.substring(0, currentPath.lastIndexOf('/')) || '/';
        this.navigateTo(parentPath);
    }

    static refresh() {
        this.loadDirectory(this.state.currentPath);
    }

    // ==================== File Operations ====================

    static handleFileClick(file, event) {
        const isCtrl = event.ctrlKey || event.metaKey;
        const isShift = event.shiftKey;
        
        if (isShift) {
            // Range selection
            this.selectRange(file);
        } else if (isCtrl) {
            // Toggle selection
            this.toggleSelection(file);
        } else {
            // Single selection
            this.selectSingle(file);
        }
        
        this.updateUI();
    }

    static selectSingle(file) {
        this.state.selectedItems.clear();
        this.state.selectedItems.add(file.path);
    }

    static toggleSelection(file) {
        if (this.state.selectedItems.has(file.path)) {
            this.state.selectedItems.delete(file.path);
        } else {
            this.state.selectedItems.add(file.path);
        }
    }

    static selectRange(file) {
        // Implement range selection logic
        console.log('Range select:', file.name);
    }

    static selectAll() {
        this.state.currentFiles.forEach(file => {
            this.state.selectedItems.add(file.path);
        });
        this.updateUI();
        UIComponents.showNotification(`Selected ${this.state.currentFiles.length} items`, 'info');
    }

    static clearSelection() {
        this.state.selectedItems.clear();
        this.updateUI();
    }

    static openFile(file) {
        if (file.type === 'directory') {
            this.navigateTo(file.path);
        } else {
            // Open file based on type
            const extension = file.name.split('.').pop().toLowerCase();
            
            switch (extension) {
                case 'pdf':
                case 'jpg':
                case 'jpeg':
                case 'png':
                case 'txt':
                    this.previewFile(file);
                    break;
                default:
                    this.downloadFile(file);
            }
        }
    }

    static previewFile(file) {
        UIComponents.showModal(
            `Preview: ${file.name}`,
            `<div class="file-preview">
                <p>Preview for ${file.name} would appear here.</p>
                <p>File size: ${UIComponents.formatSize(file.size)}</p>
                <p>Last modified: ${UIComponents.formatDate(file.modified)}</p>
            </div>`,
            [
                { text: 'Download', class: 'btn-primary', action: 'downloadSelected' },
                { text: 'Close', class: 'btn-secondary', action: 'closeModal' }
            ]
        );
    }

    static async downloadFile(file) {
        UIComponents.showNotification(`Downloading ${file.name}...`, 'info');
        
        // Simulate download
        await new Promise(resolve => setTimeout(resolve, 1000));
        
        // Create download link
        const link = document.createElement('a');
        link.href = '#'; // Replace with actual file URL
        link.download = file.name;
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
        
        UIComponents.showNotification(`Downloaded ${file.name}`, 'success');
    }

    static downloadSelected() {
        const selectedFiles = this.state.currentFiles.filter(file => 
            this.state.selectedItems.has(file.path)
        );
        
        if (selectedFiles.length === 0) {
            UIComponents.showNotification('No files selected', 'warning');
            return;
        }
        
        if (selectedFiles.length === 1) {
            this.downloadFile(selectedFiles[0]);
        } else {
            // Create zip download for multiple files
            UIComponents.showNotification(`Preparing download of ${selectedFiles.length} files...`, 'info');
            // Implement zip creation here
        }
    }

    static deleteFile(file) {
        const modal = UIComponents.showModal(
            'Delete File',
            `<p>Are you sure you want to delete <strong>${file.name}</strong>?</p>
             <p>This action cannot be undone.</p>`,
            [
                { text: 'Cancel', class: 'btn-secondary', action: 'closeModal' },
                { text: 'Delete', class: 'btn-danger', action: () => this.confirmDelete(file) }
            ]
        );
    }

    static confirmDelete(file) {
        // Simulate deletion
        console.log('Deleting file:', file.path);
        
        // Remove from current files
        this.state.currentFiles = this.state.currentFiles.filter(f => f.path !== file.path);
        this.state.selectedItems.delete(file.path);
        
        this.updateUI();
        UIComponents.showNotification(`Deleted ${file.name}`, 'success');
    }

    static deleteSelected() {
        const selectedFiles = this.state.currentFiles.filter(file => 
            this.state.selectedItems.has(file.path)
        );
        
        if (selectedFiles.length === 0) {
            UIComponents.showNotification('No files selected', 'warning');
            return;
        }
        
        const fileList = selectedFiles.map(f => `<li>${f.name}</li>`).join('');
        
        UIComponents.showModal(
            'Delete Files',
            `<p>Are you sure you want to delete ${selectedFiles.length} item(s)?</p>
             <ul>${fileList}</ul>
             <p>This action cannot be undone.</p>`,
            [
                { text: 'Cancel', class: 'btn-secondary', action: 'closeModal' },
                { text: 'Delete', class: 'btn-danger', action: () => this.confirmDeleteSelected(selectedFiles) }
            ]
        );
    }

    static confirmDeleteSelected(files) {
        files.forEach(file => {
            // Remove from current files
            this.state.currentFiles = this.state.currentFiles.filter(f => f.path !== file.path);
            this.state.selectedItems.delete(file.path);
        });
        
        this.updateUI();
        UIComponents.showNotification(`Deleted ${files.length} items`, 'success');
    }

    static renameFile(file) {
        const content = `
            <div class="rename-dialog">
                <p>Rename <strong>${file.name}</strong></p>
                <input type="text" id="new-filename" value="${file.name}" class="form-control">
                <div class="form-hint">Enter new name for the file</div>
            </div>
        `;
        
        const modal = UIComponents.showModal(
            'Rename File',
            content,
            [
                { text: 'Cancel', class: 'btn-secondary', action: 'closeModal' },
                { text: 'Rename', class: 'btn-primary', action: () => this.confirmRename(file) }
            ]
        );
        
        // Focus input
        setTimeout(() => {
            const input = modal.querySelector('#new-filename');
            if (input) {
                input.focus();
                input.select();
            }
        }, 100);
    }

    static confirmRename(file) {
        const newName = document.getElementById('new-filename')?.value;
        if (!newName || newName === file.name) return;
        
        // Update file name
        const oldPath = file.path;
        const newPath = file.path.substring(0, file.path.lastIndexOf('/')) + '/' + newName;
        
        // Update in current files
        const fileIndex = this.state.currentFiles.findIndex(f => f.path === oldPath);
        if (fileIndex !== -1) {
            this.state.currentFiles[fileIndex].name = newName;
            this.state.currentFiles[fileIndex].path = newPath;
        }
        
        // Update selection
        if (this.state.selectedItems.has(oldPath)) {
            this.state.selectedItems.delete(oldPath);
            this.state.selectedItems.add(newPath);
        }
        
        this.updateUI();
        UIComponents.showNotification(`Renamed to ${newName}`, 'success');
    }

    static createNewFolder() {
        const content = `
            <div class="new-folder-dialog">
                <p>Create New Folder</p>
                <input type="text" id="folder-name" value="New Folder" class="form-control">
                <div class="form-hint">Enter name for the new folder</div>
            </div>
        `;
        
        const modal = UIComponents.showModal(
            'New Folder',
            content,
            [
                { text: 'Cancel', class: 'btn-secondary', action: 'closeModal' },
                { text: 'Create', class: 'btn-primary', action: () => this.confirmNewFolder() }
            ]
        );
        
        setTimeout(() => {
            const input = modal.querySelector('#folder-name');
            if (input) {
                input.focus();
                input.select();
            }
        }, 100);
    }

    static confirmNewFolder() {
        const folderName = document.getElementById('folder-name')?.value;
        if (!folderName) return;
        
        // Add new folder to current files
        const newFolder = {
            name: folderName,
            type: 'directory',
            path: this.state.currentPath + (this.state.currentPath.endsWith('/') ? '' : '/') + folderName,
            size: 0,
            modified: Date.now(),
            itemCount: 0
        };
        
        this.state.currentFiles.push(newFolder);
        this.sortFiles();
        this.updateUI();
        
        UIComponents.showNotification(`Created folder: ${folderName}`, 'success');
    }

    static showUploadDialog() {
        const content = `
            <div class="upload-dialog">
                <p>Upload Files</p>
                <div class="upload-area" id="upload-dropzone">
                    <i class="fas fa-cloud-upload-alt"></i>
                    <p>Drag files here or click to browse</p>
                    <input type="file" id="file-input" multiple style="display: none;">
                    <button class="btn" onclick="document.getElementById('file-input').click()">Browse Files</button>
                </div>
                <div id="upload-progress" style="display: none;">
                    <div class="progress-bar">
                        <div class="progress-fill"></div>
                    </div>
                    <div class="progress-text">Uploading...</div>
                </div>
            </div>
        `;
        
        const modal = UIComponents.showModal(
            'Upload Files',
            content,
            [
                { text: 'Cancel', class: 'btn-secondary', action: 'closeModal' },
                { text: 'Upload', class: 'btn-primary', action: () => this.startUpload() }
            ]
        );
        
        // Setup drag and drop for upload area
        const dropzone = modal.querySelector('#upload-dropzone');
        const fileInput = modal.querySelector('#file-input');
        
        dropzone.addEventListener('dragover', (e) => {
            e.preventDefault();
            dropzone.classList.add('drag-over');
        });
        
        dropzone.addEventListener('dragleave', () => {
            dropzone.classList.remove('drag-over');
        });
        
        dropzone.addEventListener('drop', (e) => {
            e.preventDefault();
            dropzone.classList.remove('drag-over');
            this.handleFileUpload(e.dataTransfer.files);
        });
        
        fileInput.addEventListener('change', (e) => {
            this.handleFileUpload(e.target.files);
        });
    }

    static handleFileDrop(files) {
        this.handleFileUpload(files);
    }

    static async handleFileUpload(files) {
        if (files.length === 0) return;
        
        // Validate files
        for (let i = 0; i < files.length; i++) {
            const file = files[i];
            
            if (file.size > CONFIG.MAX_FILE_SIZE) {
                UIComponents.showNotification(
                    `${file.name} exceeds maximum file size (${UIComponents.formatSize(CONFIG.MAX_FILE_SIZE)})`,
                    'error'
                );
                return;
            }
            
            const extension = '.' + file.name.split('.').pop().toLowerCase();
            if (!CONFIG.ALLOWED_EXTENSIONS.includes(extension) && CONFIG.ALLOWED_EXTENSIONS.length > 0) {
                UIComponents.showNotification(
                    `${file.name} has unsupported file type`,
                    'error'
                );
                return;
            }
        }
        
        // Simulate upload
        UIComponents.showNotification(`Uploading ${files.length} file(s)...`, 'info');
        
        for (let i = 0; i < files.length; i++) {
            const file = files[i];
            
            // Add to current files (simulating upload completion)
            setTimeout(() => {
                const newFile = {
                    name: file.name,
                    type: 'file',
                    path: this.state.currentPath + (this.state.currentPath.endsWith('/') ? '' : '/') + file.name,
                    size: file.size,
                    modified: Date.now()
                };
                
                this.state.currentFiles.push(newFile);
                this.sortFiles();
                this.updateUI();
                
                UIComponents.showNotification(`Uploaded ${file.name}`, 'success');
            }, i * 500);
        }
    }

    static startUpload() {
        const fileInput = document.querySelector('#file-input');
        if (fileInput && fileInput.files.length > 0) {
            this.handleFileUpload(fileInput.files);
        }
    }

    static copyPath(file) {
        navigator.clipboard.writeText(file.path)
            .then(() => {
                UIComponents.showNotification('Path copied to clipboard', 'success');
            })
            .catch(err => {
                UIComponents.showNotification('Failed to copy path', 'error');
            });
    }

    static shareFile(file) {
        if (navigator.share) {
            navigator.share({
                title: file.name,
                text: 'Check out this file',
                url: file.path // Replace with actual URL
            })
            .then(() => UIComponents.showNotification('Shared successfully', 'success'))
            .catch(() => UIComponents.showNotification('Share cancelled', 'info'));
        } else {
            this.copyPath(file);
        }
    }

    // ==================== Settings ====================

    static setViewMode(mode) {
        this.state.viewMode = mode;
        this.state.updatePreferences({ viewMode: mode });
        this.renderFiles();
    }

    static showSettings() {
        const content = `
            <div class="settings-dialog">
                <div class="setting-group">
                    <h4>Appearance</h4>
                    <div class="setting-item">
                        <label>Theme</label>
                        <select id="theme-select" class="form-control">
                            ${CONFIG.THEMES.map(theme => 
                                `<option value="${theme}" ${this.state.userPreferences.theme === theme ? 'selected' : ''}>
                                    ${theme.charAt(0).toUpperCase() + theme.slice(1)}
                                </option>`
                            ).join('')}
                        </select>
                    </div>
                    <div class="setting-item">
                        <label>Default View</label>
                        <select id="default-view" class="form-control">
                            <option value="grid" ${this.state.viewMode === 'grid' ? 'selected' : ''}>Grid View</option>
                            <option value="list" ${this.state.viewMode === 'list' ? 'selected' : ''}>List View</option>
                        </select>
                    </div>
                </div>
                
                <div class="setting-group">
                    <h4>File Display</h4>
                    <div class="setting-item">
                        <label class="checkbox">
                            <input type="checkbox" id="show-hidden" ${this.state.showHidden ? 'checked' : ''}>
                            Show hidden files
                        </label>
                    </div>
                    <div class="setting-item">
                        <label class="checkbox">
                            <input type="checkbox" id="show-thumbnails" checked>
                            Show thumbnails
                        </label>
                    </div>
                </div>
                
                <div class="setting-group">
                    <h4>Behavior</h4>
                    <div class="setting-item">
                        <label class="checkbox">
                            <input type="checkbox" id="confirm-delete" checked>
                            Confirm before deleting
                        </label>
                    </div>
                    <div class="setting-item">
                        <label class="checkbox">
                            <input type="checkbox" id="single-click-open">
                            Single click to open
                        </label>
                    </div>
                </div>
            </div>
        `;
        
        const modal = UIComponents.showModal(
            'Settings',
            content,
            [
                { text: 'Cancel', class: 'btn-secondary', action: 'closeModal' },
                { text: 'Save', class: 'btn-primary', action: () => this.saveSettings() }
            ]
        );
    }

    static saveSettings() {
        const theme = document.getElementById('theme-select')?.value;
        const defaultView = document.getElementById('default-view')?.value;
        const showHidden = document.getElementById('show-hidden')?.checked;
        
        this.state.updatePreferences({
            theme: theme,
            viewMode: defaultView,
            showHidden: showHidden
        });
        
        UIComponents.showNotification('Settings saved', 'success');
    }

    // ==================== Utility Functions ====================

    static closeModal() {
        // Placeholder for modal close action
    }
}

// ==================== Utility Functions ====================

function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

// ==================== Initialize on DOM Load ====================

document.addEventListener('DOMContentLoaded', () => {
    // Check for required elements
    if (!document.getElementById('file-container')) {
        console.error('Required element #file-container not found');
        return;
    }
    
    // Initialize FileManager
    FileManager.init();
    
    // Add global error handler
    window.addEventListener('error', (event) => {
        console.error('Global error:', event.error);
        UIComponents.showNotification(`Error: ${event.message}`, 'error');
    });
    
    // Add offline/online detection
    window.addEventListener('online', () => {
        UIComponents.showNotification('Back online', 'success');
        FileManager.refresh();
    });
    
    window.addEventListener('offline', () => {
        UIComponents.showNotification('You are offline', 'warning');
    });
    
    console.log('FileManagerPro initialized successfully');
});

// ==================== Global Functions for HTML Event Handlers ====================

// Make FileManager methods available globally
window.FileManager = FileManager;
window.UIComponents = UIComponents;

// Global utility functions
window.formatSize = UIComponents.formatSize;
window.formatDate = UIComponents.formatDate;