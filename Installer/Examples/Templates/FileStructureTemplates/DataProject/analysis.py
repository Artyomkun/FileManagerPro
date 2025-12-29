#!/usr/bin/env python3
"""
FileManagerPro - Data Analysis Module
File: analysis.py
Description: Advanced file system analysis and reporting tools
Requirements: Python 3.8+, pandas, matplotlib, numpy
Last Modified: 2024-01-15
"""

import os
import sys
import json
import csv
import hashlib
import mimetypes
import time
from datetime import datetime, timedelta
from pathlib import Path
from collections import defaultdict, Counter
from typing import Dict, List, Tuple, Any, Optional, Generator
from dataclasses import dataclass, asdict
from enum import Enum
import statistics
import math

# Optional imports with fallbacks
try:
    import pandas as pd
    PANDAS_AVAILABLE = True
except ImportError:
    PANDAS_AVAILABLE = False
    print("Warning: pandas not installed. Some features will be limited.")

try:
    import matplotlib.pyplot as plt
    MATPLOTLIB_AVAILABLE = True
except ImportError:
    MATPLOTLIB_AVAILABLE = False
    print("Warning: matplotlib not installed. Visualization features disabled.")

try:
    import numpy as np
    NUMPY_AVAILABLE = True
except ImportError:
    NUMPY_AVAILABLE = True
    print("Warning: numpy not installed. Some statistical functions limited.")

# ==================== Enums and Data Classes ====================

class FileType(Enum):
    """File type categories"""
    TEXT = "Text"
    IMAGE = "Image"
    VIDEO = "Video"
    AUDIO = "Audio"
    ARCHIVE = "Archive"
    DOCUMENT = "Document"
    CODE = "Code"
    EXECUTABLE = "Executable"
    SYSTEM = "System"
    OTHER = "Other"

class AnalysisMode(Enum):
    """Analysis modes"""
    QUICK = "quick"
    STANDARD = "standard"
    DETAILED = "detailed"
    DEEP = "deep"

@dataclass
class FileStats:
    """Statistics for a single file"""
    path: str
    name: str
    size: int
    file_type: FileType
    extension: str
    created: float
    modified: float
    accessed: float
    mime_type: str = ""
    hash_md5: str = ""
    hash_sha256: str = ""
    line_count: int = 0
    word_count: int = 0
    char_count: int = 0
    permissions: int = 0o644
    owner: str = ""
    group: str = ""
    
    def to_dict(self) -> Dict:
        """Convert to dictionary"""
        data = asdict(self)
        data['file_type'] = self.file_type.value
        data['created'] = datetime.fromtimestamp(self.created).isoformat()
        data['modified'] = datetime.fromtimestamp(self.modified).isoformat()
        data['accessed'] = datetime.fromtimestamp(self.accessed).isoformat()
        return data

@dataclass
class DirectoryStats:
    """Statistics for a directory"""
    path: str
    total_files: int = 0
    total_dirs: int = 0
    total_size: int = 0
    avg_file_size: float = 0.0
    median_file_size: float = 0.0
    largest_file: Tuple[str, int] = ("", 0)
    smallest_file: Tuple[str, int] = ("", 0)
    oldest_file: Tuple[str, float] = ("", 0.0)
    newest_file: Tuple[str, float] = ("", 0.0)
    file_types: Dict[str, int] = None
    extensions: Dict[str, int] = None
    depth: int = 0
    owner_stats: Dict[str, int] = None
    
    def __post_init__(self):
        if self.file_types is None:
            self.file_types = defaultdict(int)
        if self.extensions is None:
            self.extensions = defaultdict(int)
        if self.owner_stats is None:
            self.owner_stats = defaultdict(int)
    
    def to_dict(self) -> Dict:
        """Convert to dictionary"""
        data = asdict(self)
        # Convert defaultdict to regular dict
        data['file_types'] = dict(self.file_types)
        data['extensions'] = dict(self.extensions)
        data['owner_stats'] = dict(self.owner_stats)
        return data

@dataclass
class DuplicateFile:
    """Duplicate file information"""
    hash_value: str
    size: int
    files: List[str]
    count: int = 0
    
    def __post_init__(self):
        self.count = len(self.files)

# ==================== File Analyzer Class ====================

class FileSystemAnalyzer:
    """Advanced file system analyzer with multiple analysis modes"""
    
    # File type mappings
    TEXT_EXTENSIONS = {'.txt', '.md', '.rtf', '.log', '.csv', '.json', '.xml', '.yaml', '.yml', '.ini', '.cfg', '.conf'}
    IMAGE_EXTENSIONS = {'.jpg', '.jpeg', '.png', '.gif', '.bmp', '.tiff', '.tif', '.svg', '.webp', '.ico', '.psd'}
    VIDEO_EXTENSIONS = {'.mp4', '.avi', '.mov', '.wmv', '.flv', '.mkv', '.webm', '.m4v', '.mpg', '.mpeg'}
    AUDIO_EXTENSIONS = {'.mp3', '.wav', '.flac', '.aac', '.ogg', '.wma', '.m4a', '.aiff'}
    ARCHIVE_EXTENSIONS = {'.zip', '.rar', '.7z', '.tar', '.gz', '.bz2', '.xz', '.tgz', '.tbz2'}
    DOCUMENT_EXTENSIONS = {'.pdf', '.doc', '.docx', '.xls', '.xlsx', '.ppt', '.pptx', '.odt', '.ods', '.odp'}
    CODE_EXTENSIONS = {'.py', '.js', '.java', '.cpp', '.c', '.h', '.cs', '.php', '.html', '.css', '.rb', '.go', '.rs', '.swift'}
    EXECUTABLE_EXTENSIONS = {'.exe', '.dll', '.so', '.dylib', '.sh', '.bat', '.cmd', '.app', '.msi'}
    SYSTEM_EXTENSIONS = {'.sys', '.dll', '.drv', '.vxd', '.ocx', '.cpl', '.tmp', '.temp', '.log', '.bak'}
    
    def __init__(self, root_path: str, mode: AnalysisMode = AnalysisMode.STANDARD):
        """
        Initialize analyzer
        
        Args:
            root_path: Root directory to analyze
            mode: Analysis mode (quick, standard, detailed, deep)
        """
        self.root_path = Path(root_path).resolve()
        self.mode = mode
        self.stats = DirectoryStats(path=str(self.root_path))
        self.files_data: List[FileStats] = []
        self.duplicates: List[DuplicateFile] = []
        self.errors: List[Tuple[str, str]] = []
        
        # Configuration based on mode
        self.config = {
            AnalysisMode.QUICK: {
                'hash_files': False,
                'count_lines': False,
                'check_duplicates': False,
                'max_depth': 3,
                'skip_hidden': True,
                'skip_system': True
            },
            AnalysisMode.STANDARD: {
                'hash_files': True,
                'count_lines': True,
                'check_duplicates': True,
                'max_depth': 10,
                'skip_hidden': True,
                'skip_system': True
            },
            AnalysisMode.DETAILED: {
                'hash_files': True,
                'count_lines': True,
                'check_duplicates': True,
                'max_depth': 20,
                'skip_hidden': False,
                'skip_system': False
            },
            AnalysisMode.DEEP: {
                'hash_files': True,
                'count_lines': True,
                'check_duplicates': True,
                'max_depth': None,  # Unlimited
                'skip_hidden': False,
                'skip_system': False
            }
        }
        
        self.current_config = self.config[self.mode]
    
    def analyze(self) -> DirectoryStats:
        """Perform comprehensive analysis of file system"""
        print(f"Starting analysis of: {self.root_path}")
        print(f"Mode: {self.mode.value}")
        print("-" * 60)
        
        start_time = time.time()
        
        # Collect file statistics
        self._collect_file_stats()
        
        # Calculate derived statistics
        self._calculate_statistics()
        
        # Find duplicates if enabled
        if self.current_config['check_duplicates']:
            self._find_duplicates()
        
        # Generate reports
        self._generate_reports()
        
        elapsed = time.time() - start_time
        print(f"\nAnalysis completed in {elapsed:.2f} seconds")
        print(f"Files processed: {self.stats.total_files}")
        print(f"Directories found: {self.stats.total_dirs}")
        print(f"Total size: {self._format_size(self.stats.total_size)}")
        
        return self.stats
    
    def _collect_file_stats(self):
        """Collect statistics by walking directory tree"""
        file_count = 0
        dir_count = 0
        total_size = 0
        file_sizes = []
        file_times = []
        
        max_depth = self.current_config['max_depth']
        skip_hidden = self.current_config['skip_hidden']
        skip_system = self.current_config['skip_system']
        
        try:
            for root, dirs, files in os.walk(self.root_path, topdown=True):
                # Calculate current depth
                current_depth = len(Path(root).relative_to(self.root_path).parts)
                
                # Apply depth limit
                if max_depth is not None and current_depth > max_depth:
                    dirs.clear()  # Don't recurse deeper
                    continue
                
                # Filter hidden/system directories
                if skip_hidden or skip_system:
                    dirs[:] = [d for d in dirs if not self._is_hidden_or_system(Path(root) / d)]
                
                # Process directories
                for dir_name in dirs:
                    dir_path = Path(root) / dir_name
                    dir_count += 1
                
                # Process files
                for file_name in files:
                    file_path = Path(root) / file_name
                    
                    # Skip hidden/system files
                    if (skip_hidden and file_name.startswith('.')) or \
                       (skip_system and self._is_system_file(file_path)):
                        continue
                    
                    try:
                        stat = file_path.stat()
                        size = stat.st_size
                        
                        # Get file type
                        extension = file_path.suffix.lower()
                        file_type = self._categorize_file(extension, file_path)
                        
                        # Basic file stats
                        file_stat = FileStats(
                            path=str(file_path),
                            name=file_name,
                            size=size,
                            file_type=file_type,
                            extension=extension,
                            created=stat.st_ctime,
                            modified=stat.st_mtime,
                            accessed=stat.st_atime
                        )
                        
                        # Detailed analysis based on mode
                        if self.current_config['hash_files']:
                            file_stat.hash_md5 = self._calculate_file_hash(file_path, 'md5')
                            file_stat.hash_sha256 = self._calculate_file_hash(file_path, 'sha256')
                        
                        if self.current_config['count_lines'] and file_type == FileType.TEXT:
                            self._count_file_content(file_path, file_stat)
                        
                        # Get MIME type
                        mime_type, _ = mimetypes.guess_type(str(file_path))
                        file_stat.mime_type = mime_type or "application/octet-stream"
                        
                        self.files_data.append(file_stat)
                        
                        # Update counters
                        file_count += 1
                        total_size += size
                        file_sizes.append(size)
                        file_times.append(stat.st_mtime)
                        
                        # Update type and extension counts
                        self.stats.file_types[file_type.value] += 1
                        if extension:
                            self.stats.extensions[extension] += 1
                        
                        # Progress reporting
                        if file_count % 1000 == 0:
                            print(f"  Processed {file_count} files...")
                    
                    except (OSError, PermissionError) as e:
                        self.errors.append((str(file_path), str(e)))
                        continue
            
        except KeyboardInterrupt:
            print("\nAnalysis interrupted by user")
            sys.exit(1)
        except Exception as e:
            print(f"Error during analysis: {e}")
            self.errors.append((str(self.root_path), str(e)))
        
        # Store basic stats
        self.stats.total_files = file_count
        self.stats.total_dirs = dir_count
        self.stats.total_size = total_size
        
        # Store for later calculations
        self._file_sizes = file_sizes
        self._file_times = file_times
    
    def _calculate_statistics(self):
        """Calculate derived statistics"""
        if not self.files_data:
            return
        
        # Size statistics
        if self._file_sizes:
            self.stats.avg_file_size = statistics.mean(self._file_sizes)
            self.stats.median_file_size = statistics.median(self._file_sizes)
            
            # Find largest and smallest files
            largest = max(self.files_data, key=lambda x: x.size, default=None)
            smallest = min((f for f in self.files_data if f.size > 0), 
                          key=lambda x: x.size, default=None)
            
            if largest:
                self.stats.largest_file = (largest.path, largest.size)
            if smallest:
                self.stats.smallest_file = (smallest.path, smallest.size)
        
        # Time statistics
        if self._file_times:
            oldest = min(self.files_data, key=lambda x: x.modified, default=None)
            newest = max(self.files_data, key=lambda x: x.modified, default=None)
            
            if oldest:
                self.stats.oldest_file = (oldest.path, oldest.modified)
            if newest:
                self.stats.newest_file = (newest.path, newest.modified)
    
    def _find_duplicates(self):
        """Find duplicate files by hash and size"""
        print("Looking for duplicate files...")
        
        # Group files by size first (quick check)
        size_groups = defaultdict(list)
        for file_stat in self.files_data:
            if file_stat.size > 0:  # Skip empty files
                size_groups[file_stat.size].append(file_stat)
        
        # Check groups with multiple files
        duplicate_candidates = []
        for size, files in size_groups.items():
            if len(files) > 1:
                duplicate_candidates.extend(files)
        
        # Now group by hash for candidates
        hash_groups = defaultdict(list)
        for file_stat in duplicate_candidates:
            if file_stat.hash_md5:  # Only if we calculated hash
                hash_groups[file_stat.hash_md5].append(file_stat.path)
        
        # Create duplicate records
        for hash_value, file_paths in hash_groups.items():
            if len(file_paths) > 1:
                size = next((f.size for f in self.files_data 
                           if f.path == file_paths[0]), 0)
                self.duplicates.append(DuplicateFile(
                    hash_value=hash_value,
                    size=size,
                    files=file_paths
                ))
        
        print(f"Found {len(self.duplicates)} groups of duplicate files")
    
    def _generate_reports(self):
        """Generate various analysis reports"""
        print("\n" + "=" * 60)
        print("ANALYSIS REPORTS")
        print("=" * 60)
        
        # Basic statistics report
        self._print_basic_stats()
        
        # File type distribution
        self._print_file_type_distribution()
        
        # Largest files report
        self._print_largest_files()
        
        # Duplicates report
        if self.duplicates:
            self._print_duplicates_report()
        
        # Error report
        if self.errors:
            self._print_error_report()
    
    def _print_basic_stats(self):
        """Print basic statistics"""
        print("\n📊 BASIC STATISTICS")
        print(f"  Total Files:       {self.stats.total_files:,}")
        print(f"  Total Directories: {self.stats.total_dirs:,}")
        print(f"  Total Size:        {self._format_size(self.stats.total_size)}")
        print(f"  Average File Size: {self._format_size(self.stats.avg_file_size)}")
        print(f"  Median File Size:  {self._format_size(self.stats.median_file_size)}")
        
        if self.stats.largest_file[0]:
            size_str = self._format_size(self.stats.largest_file[1])
            print(f"  Largest File:      {size_str} ({Path(self.stats.largest_file[0]).name})")
        
        if self.stats.oldest_file[0]:
            date_str = datetime.fromtimestamp(self.stats.oldest_file[1]).strftime('%Y-%m-%d')
            print(f"  Oldest File:       {date_str} ({Path(self.stats.oldest_file[0]).name})")
    
    def _print_file_type_distribution(self):
        """Print file type distribution"""
        print("\n📁 FILE TYPE DISTRIBUTION")
        
        total_files = self.stats.total_files
        if total_files == 0:
            print("  No files found")
            return
        
        # Sort by count descending
        sorted_types = sorted(self.stats.file_types.items(), 
                            key=lambda x: x[1], reverse=True)
        
        for file_type, count in sorted_types:
            percentage = (count / total_files) * 100
            bar_length = int(percentage / 2)  # Scale for display
            bar = "█" * bar_length + "░" * (50 - bar_length)
            print(f"  {file_type:15} {bar} {count:6,} ({percentage:5.1f}%)")
    
    def _print_largest_files(self, limit: int = 10):
        """Print largest files"""
        print(f"\n🏆 TOP {limit} LARGEST FILES")
        
        # Sort files by size descending
        sorted_files = sorted(self.files_data, 
                            key=lambda x: x.size, reverse=True)[:limit]
        
        for i, file_stat in enumerate(sorted_files, 1):
            size_str = self._format_size(file_stat.size)
            relative_path = Path(file_stat.path).relative_to(self.root_path)
            print(f"  {i:2}. {size_str:>10}  {relative_path}")
    
    def _print_duplicates_report(self):
        """Print duplicates report"""
        print(f"\n🔄 DUPLICATE FILES ({len(self.duplicates)} groups)")
        
        # Sort by size descending (largest duplicates first)
        sorted_dups = sorted(self.duplicates, 
                           key=lambda x: x.size * x.count, reverse=True)[:10]
        
        for i, dup in enumerate(sorted_dups, 1):
            size_str = self._format_size(dup.size)
            space_wasted = dup.size * (dup.count - 1)
            space_str = self._format_size(space_wasted)
            print(f"\n  Group {i}: {size_str} each ({dup.count} copies)")
            print(f"  Waste: {space_str} (if keeping only one copy)")
            
            for j, file_path in enumerate(dup.files[:3], 1):  # Show first 3
                relative_path = Path(file_path).relative_to(self.root_path)
                print(f"    {j}. {relative_path}")
            
            if len(dup.files) > 3:
                print(f"    ... and {len(dup.files) - 3} more")
    
    def _print_error_report(self):
        """Print error report"""
        print(f"\n❌ ERRORS ENCOUNTERED ({len(self.errors)} errors)")
        
        for i, (path, error) in enumerate(self.errors[:10], 1):
            print(f"  {i}. {error}: {path}")
        
        if len(self.errors) > 10:
            print(f"  ... and {len(self.errors) - 10} more errors")
    
    # ==================== Utility Methods ====================
    
    def _categorize_file(self, extension: str, file_path: Path) -> FileType:
        """Categorize file by extension"""
        if extension in self.TEXT_EXTENSIONS:
            return FileType.TEXT
        elif extension in self.IMAGE_EXTENSIONS:
            return FileType.IMAGE
        elif extension in self.VIDEO_EXTENSIONS:
            return FileType.VIDEO
        elif extension in self.AUDIO_EXTENSIONS:
            return FileType.AUDIO
        elif extension in self.ARCHIVE_EXTENSIONS:
            return FileType.ARCHIVE
        elif extension in self.DOCUMENT_EXTENSIONS:
            return FileType.DOCUMENT
        elif extension in self.CODE_EXTENSIONS:
            return FileType.CODE
        elif extension in self.EXECUTABLE_EXTENSIONS:
            return FileType.EXECUTABLE
        elif extension in self.SYSTEM_EXTENSIONS:
            return FileType.SYSTEM
        else:
            return FileType.OTHER
    
    def _calculate_file_hash(self, file_path: Path, algorithm: str = 'md5') -> str:
        """Calculate file hash"""
        hash_func = hashlib.new(algorithm)
        
        try:
            with open(file_path, 'rb') as f:
                # Read in chunks for large files
                for chunk in iter(lambda: f.read(8192), b''):
                    hash_func.update(chunk)
            return hash_func.hexdigest()
        except (IOError, OSError):
            return ""
    
    def _count_file_content(self, file_path: Path, file_stat: FileStats):
        """Count lines, words, and characters in text file"""
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                file_stat.line_count = content.count('\n')
                file_stat.word_count = len(content.split())
                file_stat.char_count = len(content)
        except (UnicodeDecodeError, IOError, OSError):
            # Not a text file or can't read
            pass
    
    def _is_hidden_or_system(self, path: Path) -> bool:
        """Check if file/directory is hidden or system"""
        if path.name.startswith('.'):
            return True
        
        try:
            if os.name == 'nt':  # Windows
                import ctypes
                FILE_ATTRIBUTE_HIDDEN = 0x2
                FILE_ATTRIBUTE_SYSTEM = 0x4
                attrs = ctypes.windll.kernel32.GetFileAttributesW(str(path))
                return attrs != -1 and (attrs & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
            else:  # Unix/Linux/Mac
                return False
        except:
            return False
    
    def _is_system_file(self, path: Path) -> bool:
        """Check if file is a system file"""
        # Add your system file detection logic here
        system_patterns = ['thumbs.db', '.ds_store', 'desktop.ini']
        return path.name.lower() in system_patterns
    
    def _format_size(self, size_bytes: float) -> str:
        """Format file size in human-readable format"""
        if size_bytes == 0:
            return "0 B"
        
        size_names = ("B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB")
        i = int(math.floor(math.log(size_bytes, 1024)))
        p = math.pow(1024, i)
        s = round(size_bytes / p, 2)
        
        return f"{s} {size_names[i]}"
    
    # ==================== Export Methods ====================
    
    def export_to_json(self, output_path: str):
        """Export analysis results to JSON file"""
        data = {
            'analysis_info': {
                'root_path': str(self.root_path),
                'mode': self.mode.value,
                'timestamp': datetime.now().isoformat(),
                'duration_seconds': time.time() - getattr(self, '_start_time', 0)
            },
            'directory_stats': self.stats.to_dict(),
            'file_count': len(self.files_data),
            'duplicate_groups': len(self.duplicates),
            'errors': len(self.errors)
        }
        
        # Add file data (limited in detailed mode)
        if self.mode in [AnalysisMode.DETAILED, AnalysisMode.DEEP]:
            data['files'] = [f.to_dict() for f in self.files_data[:1000]]  # Limit
            data['duplicates'] = [{
                'hash': d.hash_value,
                'size': d.size,
                'count': d.count,
                'files': d.files[:5]  # Limit
            } for d in self.duplicates]
        
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, default=str)
        
        print(f"\n✅ Results exported to: {output_path}")
    
    def export_to_csv(self, output_path: str):
        """Export file data to CSV"""
        if not self.files_data:
            print("No file data to export")
            return
        
        fieldnames = ['path', 'name', 'size', 'file_type', 'extension', 
                     'created', 'modified', 'accessed', 'mime_type', 
                     'line_count', 'word_count', 'char_count']
        
        with open(output_path, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            
            for file_stat in self.files_data:
                row = file_stat.to_dict()
                row['created'] = datetime.fromtimestamp(file_stat.created).isoformat()
                row['modified'] = datetime.fromtimestamp(file_stat.modified).isoformat()
                row['accessed'] = datetime.fromtimestamp(file_stat.accessed).isoformat()
                row['file_type'] = file_stat.file_type.value
                writer.writerow(row)
        
        print(f"\n✅ CSV exported to: {output_path}")

# ==================== Visualization Functions ====================

def create_visualizations(analyzer: FileSystemAnalyzer, output_dir: str = "visualizations"):
    """Create visualizations from analysis data"""
    if not MATPLOTLIB_AVAILABLE:
        print("Visualization disabled: matplotlib not available")
        return
    
    os.makedirs(output_dir, exist_ok=True)
    
    # 1. File Type Distribution Pie Chart
    fig1, ax1 = plt.subplots(figsize=(10, 8))
    file_types = analyzer.stats.file_types
    
    # Prepare data
    labels = list(file_types.keys())
    sizes = list(file_types.values())
    
    # Only show top 8, group others as "Other"
    if len(labels) > 8:
        sorted_items = sorted(zip(labels, sizes), key=lambda x: x[1], reverse=True)
        top_labels = [item[0] for item in sorted_items[:7]]
        top_sizes = [item[1] for item in sorted_items[:7]]
        
        other_size = sum(item[1] for item in sorted_items[7:])
        top_labels.append("Other")
        top_sizes.append(other_size)
        
        labels, sizes = top_labels, top_sizes
    
    # Create pie chart
    wedges, texts, autotexts = ax1.pie(sizes, labels=labels, autopct='%1.1f%%',
                                      startangle=90, shadow=True)
    
    # Equal aspect ratio ensures pie is drawn as circle
    ax1.axis('equal')
    ax1.set_title('File Type Distribution', fontsize=16, fontweight='bold')
    
    # Improve readability
    for autotext in autotexts:
        autotext.set_color('white')
        autotext.set_fontweight('bold')
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'file_type_distribution.png'), dpi=150)
    plt.close()
    
    # 2. File Size Distribution Histogram
    if analyzer.files_data:
        fig2, ax2 = plt.subplots(figsize=(12, 6))
        
        # Get file sizes in MB for better visualization
        sizes_mb = [f.size / (1024 * 1024) for f in analyzer.files_data if f.size > 0]
        
        # Log scale for wide distribution
        log_sizes = [math.log10(s) if s > 0 else 0 for s in sizes_mb]
        
        ax2.hist(log_sizes, bins=50, alpha=0.7, color='skyblue', edgecolor='black')
        ax2.set_xlabel('File Size (log10 MB)', fontsize=12)
        ax2.set_ylabel('Number of Files', fontsize=12)
        ax2.set_title('File Size Distribution (Log Scale)', fontsize=14, fontweight='bold')
        ax2.grid(True, alpha=0.3)
        
        # Add size labels on x-axis
        ticks = [0, 1, 2, 3, 4, 5, 6]  # 1B to 1TB
        tick_labels = ['1B', '10MB', '100MB', '1GB', '10GB', '100GB', '1TB']
        ax2.set_xticks(ticks)
        ax2.set_xticklabels(tick_labels)
        
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, 'file_size_distribution.png'), dpi=150)
        plt.close()
    
    # 3. Top 10 File Extensions Bar Chart
    fig3, ax3 = plt.subplots(figsize=(12, 6))
    
    extensions = analyzer.stats.extensions
    sorted_ext = sorted(extensions.items(), key=lambda x: x[1], reverse=True)[:10]
    
    ext_names = [ext[0] if ext[0] else '<no ext>' for ext in sorted_ext]
    ext_counts = [ext[1] for ext in sorted_ext]
    
    bars = ax3.bar(range(len(ext_names)), ext_counts, color='lightcoral', edgecolor='darkred')
    ax3.set_xlabel('File Extension', fontsize=12)
    ax3.set_ylabel('Number of Files', fontsize=12)
    ax3.set_title('Top 10 File Extensions', fontsize=14, fontweight='bold')
    ax3.set_xticks(range(len(ext_names)))
    ax3.set_xticklabels(ext_names, rotation=45, ha='right')
    
    # Add count labels on bars
    for bar, count in zip(bars, ext_counts):
        height = bar.get_height()
        ax3.text(bar.get_x() + bar.get_width()/2., height,
                f'{count:,}', ha='center', va='bottom')
    
    plt.tight_layout()
    plt.savefig(os.path.join(output_dir, 'top_extensions.png'), dpi=150)
    plt.close()
    
    print(f"\n📊 Visualizations saved to: {output_dir}/")

# ==================== Main Function ====================

def main():
    """Main entry point for analysis script"""
    import argparse
    
    parser = argparse.ArgumentParser(
        description='File System Analyzer for FileManagerPro',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s /path/to/analyze
  %(prog)s /path/to/analyze --mode detailed --output report.json
  %(prog)s /path/to/analyze --visualize --export-csv files.csv
        """
    )
    
    parser.add_argument('path', help='Directory path to analyze')
    parser.add_argument('--mode', choices=['quick', 'standard', 'detailed', 'deep'],
                       default='standard', help='Analysis mode (default: standard)')
    parser.add_argument('--output', '-o', help='Output JSON file for results')
    parser.add_argument('--export-csv', help='Export file data to CSV')
    parser.add_argument('--visualize', '-v', action='store_true',
                       help='Generate visualizations')
    parser.add_argument('--quiet', '-q', action='store_true',
                       help='Reduce console output')
    
    args = parser.parse_args()
    
    # Check if path exists
    if not os.path.exists(args.path):
        print(f"Error: Path does not exist: {args.path}")
        sys.exit(1)
    
    if not os.path.isdir(args.path):
        print(f"Error: Path is not a directory: {args.path}")
        sys.exit(1)
    
    # Create analyzer
    analyzer = FileSystemAnalyzer(args.path, AnalysisMode(args.mode))
    analyzer._start_time = time.time()
    
    # Run analysis
    stats = analyzer.analyze()
    
    # Export results
    if args.output:
        analyzer.export_to_json(args.output)
    
    if args.export_csv:
        analyzer.export_to_csv(args.export_csv)
    
    # Generate visualizations
    if args.visualize:
        create_visualizations(analyzer)
    
    # Summary
    if not args.quiet:
        print("\n" + "=" * 60)
        print("ANALYSIS COMPLETE")
        print("=" * 60)
        
        # Space wasted by duplicates
        if analyzer.duplicates:
            wasted_space = sum(dup.size * (dup.count - 1) for dup in analyzer.duplicates)
            wasted_str = analyzer._format_size(wasted_space)
            print(f"💾 Potential space savings: {wasted_str} (by removing duplicates)")
        
        # Recommendations
        print("\n💡 RECOMMENDATIONS:")
        
        if analyzer.stats.total_size > 10 * 1024**3:  # > 10GB
            print("  • Consider archiving old files")
        
        if len(analyzer.duplicates) > 10:
            print("  • Many duplicate files found - consider cleanup")
        
        if analyzer.stats.file_types.get('Temporary', 0) > 100:
            print("  • High number of temporary files - consider cleanup")
        
        if analyzer.errors:
            print(f"  • {len(analyzer.errors)} access errors - check permissions")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())