using System;
using System.IO;
using System.Linq;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Threading.Tasks;
using System.Globalization;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Threading;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Layout;
using Avalonia.Controls.Templates;
using Avalonia.Controls.Presenters;
using Avalonia.VisualTree;
using System.Collections.Generic;
using System.Diagnostics;

namespace FileManager
{
    public enum ViewMode
    {
        ExtraLargeIcons,
        LargeIcons,
        MediumIcons,
        SmallIcons,
        List,
        Details,
        Tiles,
        Content
    }

    public enum SortBy
    {
        Name,
        DateModified,
        Type,
        Size,
        None
    }

    public partial class MainWindow : Window, INotifyPropertyChanged
    {
        private bool _isInitialized = false;
        private string _currentPath = string.Empty;
        public string CurrentPath
        {
            get => _currentPath;
            set
            {
                _currentPath = value;
                OnPropertyChanged(nameof(CurrentPath));
            }
        }

        private DriveItem? _selectedDrive;
        public DriveItem? SelectedDrive
        {
            get => _selectedDrive;
            set
            {
                if (!EqualityComparer<DriveItem?>.Default.Equals(_selectedDrive, value))
                {
                    _selectedDrive = value;
                    OnPropertyChanged(nameof(SelectedDrive));
                }
            }
        }
        
        private ObservableCollection<FileItem> _items = new();
        public ObservableCollection<FileItem> Items
        {
            get => _items;
            set
            {
                _items = value;
                OnPropertyChanged(nameof(Items));
            }
        }

        public bool ShowFilePreview
        {
            get 
            {
                return SelectedItem != null && !SelectedItem.IsDirectory;
            }
        }

        // Обновляйте это свойство при изменении SelectedItem
        private FileItem? _selectedItem;
        public FileItem? SelectedItem
        {
            get => _selectedItem;
            set
            {
                if (!EqualityComparer<FileItem?>.Default.Equals(_selectedItem, value))
                {
                    _selectedItem = value;
                    OnPropertyChanged(nameof(SelectedItem));
                    OnPropertyChanged(nameof(ShowFilePreview)); 
                    
                    if (ShowFilePreview)
                    {
                        UpdateFilePreview();
                    }
                }
            }
        }
        
        private void UpdateFilePreview()
        {
            if (SelectedItem == null || SelectedItem.IsDirectory) return;
            
            var previewControl = this.FindControl<ContentControl>("PreviewContentControl");
            if (previewControl != null)
            {
                // Простая иконка файла
                previewControl.Content = new TextBlock 
                { 
                    Text = SelectedItem.Icon, 
                    FontSize = 64
                };
            }
        }

        private ViewMode _currentViewMode = ViewMode.Details;
        public ViewMode CurrentViewMode
        {
            get => _currentViewMode;
            set
            {
                _currentViewMode = value;
                OnPropertyChanged(nameof(CurrentViewMode));
                UpdateViewMode();
            }
        }
        
        private string _statusText = "Готово";
        public string StatusText
        {
            get => _statusText;
            set
            {
                _statusText = value;
                OnPropertyChanged(nameof(StatusText));
            }
        }
        
        private object? _previewContent;
        public object? PreviewContent
        {
            get => _previewContent;
            set
            {
                _previewContent = value;
                OnPropertyChanged(nameof(PreviewContent));
            }
        }
        
        private bool _showPreview = true;
        public bool ShowPreview
        {
            get => _showPreview;
            set
            {
                _showPreview = value;
                OnPropertyChanged(nameof(ShowPreview));
                UpdatePreviewVisibility();
            }
        }

        private void UpdateNavigationButtons()
        {
            CanGoBack = _backHistory.Count > 0;
            CanGoForward = _forwardHistory.Count > 0;
            CanGoUp = CurrentPath != Path.GetPathRoot(CurrentPath);
        }
        
        // История навигации
        private Stack<string> _backHistory = new();
        private Stack<string> _forwardHistory = new();
        
        public MainWindow()
        {
            InitializeComponent();
            DataContext = this; 
            
            this.Loaded += OnMainWindowLoaded;

            #if DEBUG
                this.AttachDevTools();
            #endif
        }

        private void OnMainWindowLoaded(object? sender, RoutedEventArgs e)
        {
            Loaded -= OnMainWindowLoaded;
            
            // Инициализируем только необходимые компоненты
            InitializeAfterLoad();
        }

        private void InitializeAfterLoad()
        {
            // Обновляем информацию о дисках
            UpdateDriveInfo(); 
            LoadDrives();
            
            // Инициализируем TreeView
            InitializeTreeViewTags();
            
            // Инициализация сортировки
            InitializeSorting();
            
            // Навигация в домашнюю директорию
            NavigateToDirectory(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile));
            
            // Инициализируем ленту (с проверкой, чтобы не вызвалась дважды)
            InitializeRibbon();
            _isInitialized = true; 
        }
        
        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
        
        private void InitializeControls()
        {
            UpdateDriveTree();
            InitializeTreeViewTags(); 
            InitializeSorting();
        }

        private readonly Dictionary<string, Control> _ribbonTabsCache = new Dictionary<string, Control>();
        private void InitializeRibbon()
        {
            // Создаем панели заранее и добавляем в кэш
            _ribbonTabsCache["Главная"] = CreateHomeRibbon();
            _ribbonTabsCache["Вид"] = CreateViewRibbon();
            _ribbonTabsCache["Поделиться"] = CreateShareRibbon();

            // Получаем контейнер для содержимого ленты
            var contentControl = this.FindControl<ContentControl>("RibbonContentControl");
            if (contentControl != null)
            {
                // Показываем первую вкладку по умолчанию
                contentControl.Content = _ribbonTabsCache["Главная"];
            }
        }

        private Control GetTab(string tabName)
        {
            return tabName switch
            {
                "Главная" => CreateHome(),
                "Вид" => CreateView(),
                "Поделиться" => CreateShare(),
                _ => new TextBlock { Text = $"Содержимое для '{tabName}'", Margin = new Thickness(10) }
            };
        }

        private Control CreateHome()
        {
            // Создаем ScrollViewer для горизонтальной прокрутки
            var scrollViewer = new ScrollViewer
            {
                HorizontalScrollBarVisibility = ScrollBarVisibility.Auto,
                VerticalScrollBarVisibility = ScrollBarVisibility.Disabled,
                Height = 90
            };
            
            var mainPanel = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Margin = new Thickness(15, 10, 15, 15),
                Spacing = 0
            };
            
            // Секция: Буфер обмена
            var clipboardSection = new Border
            {
                MinWidth = 160,
                Padding = new Thickness(15, 0, 15, 0),
                BorderThickness = new Thickness(0, 0, 1, 0),
                BorderBrush = new SolidColorBrush(Color.Parse("#E5E5E5"))
            };
            
            var clipboardStack = new StackPanel { Spacing = 5 };
            clipboardStack.Children.Add(new TextBlock 
            { 
                Text = "Буфер обмена",
                FontSize = 10,
                FontWeight = FontWeight.SemiBold,
                Foreground = new SolidColorBrush(Color.Parse("#666666")),
                Margin = new Thickness(0, 0, 0, 5)
            });
            
            var clipboardButtons = new WrapPanel();
            
            var pasteButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "📋", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Вставить", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0)
            };
            pasteButton.Click += OnPasteClick;
            
            var cutButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "✂️", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Вырезать", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0)
            };
            cutButton.Click += OnCutClick;
            
            clipboardButtons.Children.Add(pasteButton);
            clipboardButtons.Children.Add(cutButton);
            clipboardStack.Children.Add(clipboardButtons);
            clipboardSection.Child = clipboardStack;
            
            // Добавляем секцию в главную панель
            mainPanel.Children.Add(clipboardSection);
            
            scrollViewer.Content = mainPanel;
            return scrollViewer;
        }

        private TabItem CreateTabWith(string header, Control content)
        {
            var tabItem = new TabItem
            {
                Header = new TextBlock
                {
                    Text = header,
                    FontSize = 11,
                    FontWeight = FontWeight.SemiBold,
                    Foreground = Brushes.Black
                },
                Content = content,
                Height = 40,
                Padding = new Thickness(15, 10),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0)
            };
            
            return tabItem;
        }

        private Control CreateView()
        {
            var panel = new StackPanel
            {
                Height = 90,
                Background = Brushes.White,
                Children = 
                {
                    new TextBlock 
                    { 
                        Text = "Содержимое вкладки 'Вид'", 
                        FontSize = 14,
                        HorizontalAlignment = HorizontalAlignment.Center,
                        VerticalAlignment = VerticalAlignment.Center
                    }
                }
            };
            
            return panel;
        }

        private Control CreateShare()
        {
            var panel = new StackPanel
            {
                Height = 90,
                Background = Brushes.White,
                Children = 
                {
                    new TextBlock 
                    { 
                        Text = "Содержимое вкладки 'Поделиться'", 
                        FontSize = 14,
                        HorizontalAlignment = HorizontalAlignment.Center,
                        VerticalAlignment = VerticalAlignment.Center
                    }
                }
            };
            
            return panel;
        }

        private void OnRibbonTabChanged(object? sender, SelectionChangedEventArgs e)
        {
            // Dispatcher гарантирует, что FindControl сработает без ошибки NameScope
            Dispatcher.UIThread.Post(() =>
            {
                var tabControl = sender as TabControl;
                var contentControl = this.FindControl<ContentControl>("RibbonContentControl");

                if (tabControl == null || contentControl == null) return;

                if (tabControl.SelectedItem is TabItem selectedTab)
                {
                    var header = (selectedTab.Header as TextBlock)?.Text ?? selectedTab.Header?.ToString();

                    if (!string.IsNullOrEmpty(header))
                    {
                        // Проверяем, есть ли готовая вкладка в кэше
                        if (_ribbonTabsCache.TryGetValue(header, out var content))
                        {
                            contentControl.Content = _ribbonTabsCache[header];
                            Console.WriteLine($"Отображена вкладка: {header}");
                        }
                        else
                        {
                            // Если вкладка неизвестна (fallback)
                            contentControl.Content = new TextBlock { Text = $"Вкладка {header} не найдена" };
                        }
                    }
                }
            });
        }

        private void UpdateRibbonTabStyles(TabControl ribbonTabControl)
        {
            if (ribbonTabControl == null) return;
            
            // Сбрасываем стили всех вкладок
            foreach (var item in ribbonTabControl.Items)
            {
                if (item is TabItem tab)
                {
                    tab.Background = new SolidColorBrush(Color.Parse("#F2F2F2"));
                    tab.BorderThickness = new Thickness(0);
                    tab.Classes.Add("ribbon-tab");
                }
            }
            
            // Применяем стиль к выбранной вкладке
            if (ribbonTabControl.SelectedItem is TabItem selectedTab)
            {
                selectedTab.Background = Brushes.White;
                selectedTab.BorderBrush = new SolidColorBrush(Color.Parse("#E5E5E5"));
                selectedTab.BorderThickness = new Thickness(1, 1, 1, 0);
                selectedTab.Classes.Add("ribbon-tab");
            }
        }

        private Control CreateHomeRibbon()
        {
            var scrollViewer = new ScrollViewer
            {
                HorizontalScrollBarVisibility = ScrollBarVisibility.Auto,
                VerticalScrollBarVisibility = ScrollBarVisibility.Disabled,
                Height = 90,
                Background = Brushes.White
            };
            
            var mainPanel = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Margin = new Thickness(15, 10),
                Spacing = 0
            };
            
            // Секция: Буфер обмена
            var clipboardSection = new Border
            {
                MinWidth = 160,
                Padding = new Thickness(15, 0),
                BorderThickness = new Thickness(0, 0, 1, 0),
                BorderBrush = new SolidColorBrush(Color.Parse("#E5E5E5"))
            };
            
            var clipboardStack = new StackPanel { Spacing = 5 };
            clipboardStack.Children.Add(new TextBlock 
            { 
                Text = "Буфер обмена",
                FontSize = 10,
                FontWeight = FontWeight.SemiBold,
                Foreground = new SolidColorBrush(Color.Parse("#666666")),
                Margin = new Thickness(0, 0, 0, 5)
            });
            
            var clipboardButtons = new WrapPanel();
            
            var pasteButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "📋", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Вставить", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            pasteButton.Click += OnPasteClick;
            
            var cutButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "✂️", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Вырезать", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            cutButton.Click += OnCutClick;
            
            var copyButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "📄", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Копировать", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            copyButton.Click += OnCopyClick;
            
            clipboardButtons.Children.Add(pasteButton);
            clipboardButtons.Children.Add(cutButton);
            clipboardButtons.Children.Add(copyButton);
            clipboardStack.Children.Add(clipboardButtons);
            clipboardSection.Child = clipboardStack;
            
            mainPanel.Children.Add(clipboardSection);
            
            // Секция: Организовать
            var organizeSection = new Border
            {
                MinWidth = 160,
                Padding = new Thickness(15, 0),
                BorderThickness = new Thickness(0, 0, 1, 0),
                BorderBrush = new SolidColorBrush(Color.Parse("#E5E5E5"))
            };
            
            var organizeStack = new StackPanel { Spacing = 5 };
            organizeStack.Children.Add(new TextBlock 
            { 
                Text = "Организовать",
                FontSize = 10,
                FontWeight = FontWeight.SemiBold,
                Foreground = new SolidColorBrush(Color.Parse("#666666")),
                Margin = new Thickness(0, 0, 0, 5)
            });
            
            var renameButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "✏️", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Переименовать", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            renameButton.Click += OnRenameClick;
            
            var deleteButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "🗑️", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Удалить", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            deleteButton.Click += OnDeleteClick;
            
            var organizeButtons = new WrapPanel();
            organizeButtons.Children.Add(renameButton);
            organizeButtons.Children.Add(deleteButton);
            organizeStack.Children.Add(organizeButtons);
            organizeSection.Child = organizeStack;
            
            mainPanel.Children.Add(organizeSection);
            
            // Секция: Создать
            var createSection = new Border
            {
                MinWidth = 160,
                Padding = new Thickness(15, 0)
            };
            
            var createStack = new StackPanel { Spacing = 5 };
            createStack.Children.Add(new TextBlock 
            { 
                Text = "Создать",
                FontSize = 10,
                FontWeight = FontWeight.SemiBold,
                Foreground = new SolidColorBrush(Color.Parse("#666666")),
                Margin = new Thickness(0, 0, 0, 5)
            });
            
            var folderButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "📁", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Папку", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            folderButton.Click += OnNewFolderClick;
            
            var fileButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "📄", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Текстовый", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            fileButton.Click += OnNewTextFileClick;
            
            var createButtons = new WrapPanel();
            createButtons.Children.Add(folderButton);
            createButtons.Children.Add(fileButton);
            createStack.Children.Add(createButtons);
            createSection.Child = createStack;
            
            mainPanel.Children.Add(createSection);
            
            scrollViewer.Content = mainPanel;
            return scrollViewer;
        }

        private Control CreateViewRibbon()
        {
            var panel = new StackPanel
            {
                Height = 90,
                Background = Brushes.White,
                Margin = new Thickness(15, 10),
                Spacing = 5
            };
            
            panel.Children.Add(new TextBlock
            {
                Text = "Вид",
                FontSize = 10,
                FontWeight = FontWeight.SemiBold,
                Foreground = new SolidColorBrush(Color.Parse("#666666"))
            });
            
            var viewButtons = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Spacing = 10
            };
            
            var detailsButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "📋", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Таблица", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = CurrentViewMode == ViewMode.Details ? 
                    new SolidColorBrush(Color.Parse("#E0E0E0")) : Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            detailsButton.Click += (s, e) => CurrentViewMode = ViewMode.Details;
            
            var iconsButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "🖼️", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Значки", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = CurrentViewMode == ViewMode.LargeIcons ? 
                    new SolidColorBrush(Color.Parse("#E0E0E0")) : Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            iconsButton.Click += (s, e) => CurrentViewMode = ViewMode.LargeIcons;
            
            viewButtons.Children.Add(detailsButton);
            viewButtons.Children.Add(iconsButton);
            panel.Children.Add(viewButtons);
            
            return panel;
        }

        private Control CreateShareRibbon()
        {
            var panel = new StackPanel
            {
                Height = 90,
                Background = Brushes.White,
                Margin = new Thickness(15, 10),
                Spacing = 5
            };
            
            panel.Children.Add(new TextBlock
            {
                Text = "Поделиться",
                FontSize = 10,
                FontWeight = FontWeight.SemiBold,
                Foreground = new SolidColorBrush(Color.Parse("#666666"))
            });
            
            var shareButtons = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Spacing = 10
            };
            
            var emailButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "📧", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Email", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            emailButton.Click += OnEmailClick;
            
            var zipButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "🗜️", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Сжать", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            zipButton.Click += OnZipClick;
            
            var burnButton = new Button
            {
                Content = new StackPanel
                {
                    Children =
                    {
                        new TextBlock { Text = "💿", FontSize = 16, HorizontalAlignment = HorizontalAlignment.Center },
                        new TextBlock { Text = "Записать", FontSize = 10, HorizontalAlignment = HorizontalAlignment.Center }
                    }
                },
                Width = 60,
                Height = 55,
                Margin = new Thickness(2),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                Classes = { "ribbon-button" }
            };
            burnButton.Click += OnBurnClick;
            
            shareButtons.Children.Add(emailButton);
            shareButtons.Children.Add(zipButton);
            shareButtons.Children.Add(burnButton);
            panel.Children.Add(shareButtons);
            
            return panel;
        }

        private Border CreateRibbonSection(string title, Control[] controls, bool withRightBorder = true)
        {
            var border = new Border
            {
                MinWidth = 160,
                Padding = new Thickness(15, 0, 15, 0),
                Margin = new Thickness(0),
                BorderThickness = new Thickness(0, 0, withRightBorder ? 1 : 0, 0),
                BorderBrush = new SolidColorBrush(Color.Parse("#E5E5E5"))
            };
            
            var stackPanel = new StackPanel
            {
                Spacing = 5
            };
            
            // Заголовок секции
            var titleBlock = new TextBlock 
            { 
                Text = title,
                FontSize = 10,
                FontWeight = FontWeight.SemiBold,
                Foreground = new SolidColorBrush(Color.Parse("#666666")),
                Margin = new Thickness(0, 0, 0, 5)
            };
            stackPanel.Children.Add(titleBlock);
            
            // Контейнер для контролов в одной строке
            var controlsContainer = new WrapPanel
            {
                Orientation = Orientation.Horizontal,
                MaxWidth = 200 // Ограничиваем ширину для компактности
            };
            
            // В Avalonia WrapPanel не имеет свойства Spacing, используем Margin
            foreach (var control in controls)
            {
                if (control is Button button)
                {
                    button.Margin = new Thickness(2);
                }
                else if (control is ComboBox combo)
                {
                    combo.Margin = new Thickness(2);
                }
                controlsContainer.Children.Add(control);
            }
            
            stackPanel.Children.Add(controlsContainer);
            border.Child = stackPanel;
            
            return border;
        }

        // Дополнительные обработчики для навигации в левой панели
        private void OnQuickAccessClick(object? sender, RoutedEventArgs e)
        {
            var path = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
            NavigateToDirectory(path);
        }

        private void OnDocumentsClick(object? sender, RoutedEventArgs e)
        {
            var path = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
            NavigateToDirectory(path);
        }

        private void OnDesktopClick(object? sender, RoutedEventArgs e)
        {
            var path = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
            NavigateToDirectory(path);
        }

        private void OnVideosClick(object? sender, RoutedEventArgs e)
        {
            var path = Environment.GetFolderPath(Environment.SpecialFolder.MyVideos);
            NavigateToDirectory(path);
        }

        private void OnDownloadsClick(object? sender, RoutedEventArgs e)
        {
            var path = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "Downloads");
            NavigateToDirectory(path);
        }

        private void OnMusicClick(object? sender, RoutedEventArgs e)
        {
            var path = Environment.GetFolderPath(Environment.SpecialFolder.MyMusic);
            NavigateToDirectory(path);
        }

        private void OnPicturesClick(object? sender, RoutedEventArgs e)
        {
            var path = Environment.GetFolderPath(Environment.SpecialFolder.MyPictures);
            NavigateToDirectory(path);
        }

        private void OnCDriveClick(object? sender, RoutedEventArgs e)
        {
            var drives = DriveInfo.GetDrives();
            var cDrive = drives.FirstOrDefault(d => d.Name.StartsWith("C:", StringComparison.OrdinalIgnoreCase));
            if (cDrive != null && cDrive.IsReady)
            {
                NavigateToDirectory(cDrive.Name);
            }
            else
            {
                ShowMessage("Диск C: не найден или не доступен");
            }
        }

        private void OnDDriveClick(object? sender, RoutedEventArgs e)
        {
            var drives = DriveInfo.GetDrives();
            var dDrive = drives.FirstOrDefault(d => d.Name.StartsWith("D:", StringComparison.OrdinalIgnoreCase));
            if (dDrive != null && dDrive.IsReady)
            {
                NavigateToDirectory(dDrive.Name);
            }
            else
            {
                ShowMessage("Диск D: не найден или не доступен");
            }
        }

        private void OnRecentFilesClick(object? sender, RoutedEventArgs e)
        {
            ShowMessage("Показать недавние файлы");
            // Можно реализовать загрузку недавних файлов из системы
            var recentPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Recent));
            NavigateToDirectory(recentPath);
        }

        private void OnNetworkDriveClick(object? sender, RoutedEventArgs e)
        {
            ShowMessage("Подключение сетевого диска");
            // Заглушка для сетевых дисков
        }

        // Обработчики для кнопок вида
        private void OnViewDetailsClick(object? sender, RoutedEventArgs e)
        {
            CurrentViewMode = ViewMode.Details;
        }

        private void OnViewLargeIconsClick(object? sender, RoutedEventArgs e)
        {
            CurrentViewMode = ViewMode.LargeIcons;
        }

        // Обработчики для строки поиска
        private void OnSearchGotFocus(object? sender, GotFocusEventArgs e)
        {
            var searchBox = sender as TextBox;
            if (searchBox != null && searchBox.Text == "Поиск...")
            {
                searchBox.Text = "";
                searchBox.Foreground = Brushes.Black;
            }
        }

        private void OnSearchLostFocus(object? sender, RoutedEventArgs e)
        {
            var searchBox = sender as TextBox;
            if (searchBox != null && string.IsNullOrWhiteSpace(searchBox.Text))
            {
                searchBox.Text = "Поиск...";
                searchBox.Foreground = Brushes.Gray;
            }
        }

        private void OnSearchKeyDown(object? sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                PerformSearch();
            }
        }

        private void PerformSearch()
        {
            var searchBox = this.FindControl<TextBox>("SearchTextBox");
            if (searchBox == null) return;
            
            var searchText = searchBox.Text;
            if (string.IsNullOrWhiteSpace(searchText) || searchText == "Поиск...")
            {
                ShowMessage("Введите текст для поиска");
                return;
            }
            
            ShowMessage($"Поиск: {searchText}");
            // Реализация поиска файлов
            SearchFiles(searchText);
        }

        private void SearchFiles(string searchPattern)
        {
            try
            {
                if (!Directory.Exists(CurrentPath))
                {
                    ShowMessage("Текущая директория не существует");
                    return;
                }
                
                var tempItems = new ObservableCollection<FileItem>();
                
                // Добавляем ".." для возврата
                if (CurrentPath != "/" && CurrentPath != "\\")
                {
                    var parent = Directory.GetParent(CurrentPath);
                    if (parent != null)
                    {
                        tempItems.Add(new FileItem
                        {
                            Name = "..",
                            Path = parent.FullName,
                            IsDirectory = true,
                            Size = 0,
                            Modified = DateTime.Now
                        });
                    }
                }
                
                // Ищем директории
                foreach (var dir in Directory.GetDirectories(CurrentPath, $"*{searchPattern}*", SearchOption.TopDirectoryOnly))
                {
                    var info = new DirectoryInfo(dir);
                    tempItems.Add(new FileItem
                    {
                        Name = info.Name,
                        Path = dir,
                        IsDirectory = true,
                        Size = 0,
                        Modified = info.LastWriteTime
                    });
                }
                
                // Ищем файлы
                foreach (var file in Directory.GetFiles(CurrentPath, $"*{searchPattern}*", SearchOption.TopDirectoryOnly))
                {
                    var info = new FileInfo(file);
                    tempItems.Add(new FileItem
                    {
                        Name = info.Name,
                        Path = file,
                        IsDirectory = false,
                        Size = info.Length,
                        Modified = info.LastWriteTime,
                        Extension = info.Extension
                    });
                }
                
                Items = tempItems;
                StatusText = $"Найдено {Items.Count} элементов по запросу \"{searchPattern}\"";
            }
            catch (Exception ex)
            {
                ShowMessage($"Ошибка поиска: {ex.Message}");
            }
        }

        // Метод для обновления заголовка текущей папки
        private void UpdateCurrentFolderText()
        {
            var currentFolderText = this.FindControl<TextBlock>("CurrentFolderText");
            if (currentFolderText != null)
            {
                var dir = new DirectoryInfo(CurrentPath);
                var folderName = dir.Name;
                var itemCount = Items.Count(i => i.Name != "..");
                currentFolderText.Text = $"{folderName} ({itemCount})";
            }
        }

        // Обновляем NavigateToDirectory, чтобы обновлять заголовок папки
        private void NavigateToDirectory(string? path)
        {
            try
            {
                if (!Directory.Exists(path))
                {
                    ShowMessage($"Директория не существует: {path}");
                    return;
                }
                
                // Сохраняем текущий путь в историю
                if (!string.IsNullOrEmpty(CurrentPath))
                    _backHistory.Push(CurrentPath);
                
                CurrentPath = path;
                
                // Очищаем вперед историю при новой навигации
                _forwardHistory.Clear();
                
                // Обновляем кнопки навигации
                UpdateNavigationButtons();
                
                // Обновляем адресную строку
                var addressTextBox = this.FindControl<TextBox>("AddressTextBox");
                if (addressTextBox != null)
                {
                    addressTextBox.Text = path;
                }
                
                Items.Clear();
                
                // Добавляем родительскую директорию
                if (path != "/" && path != "\\")
                {
                    var parent = Directory.GetParent(path);
                    if (parent != null)
                    {
                        Items.Add(new FileItem
                        {
                            Name = "..",
                            Path = parent.FullName,
                            IsDirectory = true,
                            Size = 0,
                            Modified = DateTime.Now
                        });
                    }
                }
                
                // Загружаем директории
                foreach (var dir in Directory.GetDirectories(path))
                {
                    var badItem = _items.FirstOrDefault(x => x.Name.Equals("appuser", StringComparison.OrdinalIgnoreCase));

                    // 2. Если нашли — удаляем из памяти
                    if (badItem != null)
                    {
                        _items.Remove(badItem);
                    }

                    var info = new DirectoryInfo(dir);

                    Items.Add(new FileItem
                    {
                        Name = info.Name,
                        Path = dir,
                        IsDirectory = true,
                        Size = 0,
                        Modified = info.LastWriteTime
                    });
                }
                
                // Загружаем файлы
                foreach (var file in Directory.GetFiles(path))
                {
                    var info = new FileInfo(file);
                    Items.Add(new FileItem
                    {
                        Name = info.Name,
                        Path = file,
                        IsDirectory = false,
                        Size = info.Length,
                        Modified = info.LastWriteTime,
                        Extension = info.Extension
                    });
                }
                
                StatusText = $"Загружено {Items.Count} элементов";
                
                // Обновляем заголовок папки
                UpdateCurrentFolderText();
                
                // Показываем/скрываем сообщение о пустой папке
                var emptyFolderMessage = this.FindControl<Border>("EmptyFolderMessage");
                if (emptyFolderMessage != null)
                {
                    emptyFolderMessage.IsVisible = Items.Count <= 1; // Только ".." если папка пуста
                }
            }
            catch (UnauthorizedAccessException)
            {
                ShowMessage("Нет доступа к директории");
            }
            catch (Exception ex)
            {
                ShowMessage($"Ошибка: {ex.Message}");
            }
        }

        // Добавляем свойство для привязки режима просмотра
        private bool _isDetailsView = true;
        public bool IsDetailsView
        {
            get => _isDetailsView;
            set
            {
                _isDetailsView = value;
                OnPropertyChanged(nameof(IsDetailsView));
                OnPropertyChanged(nameof(IsIconsView));
            }
        }

        private bool _isIconsView = false;
        public bool IsIconsView
        {
            get => _isIconsView;
            set
            {
                _isIconsView = value;
                OnPropertyChanged(nameof(IsIconsView));
                OnPropertyChanged(nameof(IsDetailsView));
            }
        }

        private string GetDriveInfoText(string driveLetter)
        {
            try
            {
                var drives = DriveInfo.GetDrives();
                // Ищем диск по букве (C или D)
                var drive = drives.FirstOrDefault(d => d.Name.StartsWith(driveLetter, StringComparison.OrdinalIgnoreCase));

                if (drive == null) return "Диск отсутствует";
                if (!drive.IsReady) return "Диск недоступен";

                // Рассчитываем объем
                long freeBytes = drive.AvailableFreeSpace;
                long totalBytes = drive.TotalSize;

                return $"{FormatSize(freeBytes)} свободно из {FormatSize(totalBytes)}";
            }
            catch
            {
                return "Ошибка данных";
            }
        }

        private void UpdateDriveInfo()
        {
            // Обновляем диск C:
            var cDriveTextBlock = this.FindControl<TextBlock>("CDriveInfoTextBlock");
            if (cDriveTextBlock != null)
            {
                cDriveTextBlock.Text = GetDriveInfoText("C:");
            }

            // Обновляем диск D:
            var dDriveTextBlock = this.FindControl<TextBlock>("DDriveInfoTextBlock");
            if (dDriveTextBlock != null)
            {
                dDriveTextBlock.Text = GetDriveInfoText("D:");
            }
        }

        private void UpdateDriveTree()
        {
            var navigationTree = this.FindControl<TreeView>("NavigationTree");
            if (navigationTree == null) return;
            
            // Находим элемент "Этот компьютер"
            var thisComputerItem = navigationTree.Items
                .OfType<TreeViewItem>()
                .FirstOrDefault(item => item.Header?.ToString() == "Этот компьютер");
            
            if (thisComputerItem != null)
            {
                thisComputerItem.Items.Clear();
                
                try
                {
                    // Добавляем все доступные диски с иконками
                    foreach (var drive in DriveInfo.GetDrives())
                    {
                        if (drive.IsReady)
                        {
                            var driveItem = new TreeViewItem
                            {
                                Header = CreateDriveHeader(drive),
                                Tag = drive.Name
                            };
                            
                            thisComputerItem.Items.Add(driveItem);
                        }
                    }
                }
                catch (Exception ex)
                {
                    Debug.WriteLine($"Error getting drives: {ex.Message}");
                }
            }
        }

        private StackPanel CreateDriveHeader(DriveInfo drive)
        {
            var panel = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Spacing = 5
            };
            
            string icon = GetDriveIcon(drive.DriveType);
            string displayName = GetDriveDisplayName(drive);
            string freeSpace = FormatSize(drive.AvailableFreeSpace);
            string totalSize = FormatSize(drive.TotalSize);
            
            panel.Children.Add(new TextBlock { Text = icon });
            panel.Children.Add(new TextBlock { Text = $"{drive.Name} ({displayName})" });
            panel.Children.Add(new TextBlock 
            { 
                Text = $"{freeSpace} свободно из {totalSize}",
                Foreground = Brushes.Gray,
                FontSize = 11
            });
            
            return panel;
        }

        private string GetDriveIcon(DriveType type)
        {
            return type switch
            {
                DriveType.Fixed => "💻",
                DriveType.Network => "🌐",
                DriveType.CDRom => "💿",
                DriveType.Removable => "💾",
                _ => "📀"
            };
        }

        private string GetDriveDisplayName(DriveInfo drive)
        {
            try
            {
                if (!string.IsNullOrEmpty(drive.VolumeLabel))
                {
                    return $"{drive.VolumeLabel} ({GetDriveType(drive.DriveType)})";
                }
                return GetDriveType(drive.DriveType);
            }
            catch
            {
                return GetDriveType(drive.DriveType);
            }
        }

        private string GetDriveType(DriveType type)
        {
            return type switch
            {
                DriveType.Fixed => "Локальный диск",
                DriveType.Network => "Сетевой диск",
                DriveType.CDRom => "CD/DVD",
                DriveType.Removable => "Съемный диск",
                DriveType.Ram => "RAM диск",
                _ => "Неизвестный"
            };
        }
        
        private void InitializeTreeViewTags()
        {
            var navigationTree = this.FindControl<TreeView>("NavigationTree");
            if (navigationTree == null) return;
            
            // Находим элемент "Быстрый доступ"
            var quickAccess = navigationTree.Items
                .OfType<TreeViewItem>()
                .FirstOrDefault(item => item.Header?.ToString() == "Быстрый доступ");
            
            if (quickAccess != null)
            {
                // Добавляем часто используемые папки
                var folders = new[]
                {
                    ("Загрузки", "⬇️", Environment.GetFolderPath(Environment.SpecialFolder.UserProfile) + "\\Downloads"),
                    ("Рабочий стол", "🖥️", Environment.GetFolderPath(Environment.SpecialFolder.Desktop)),
                    ("Документы", "📄", Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)),
                    ("Изображения", "🖼️", Environment.GetFolderPath(Environment.SpecialFolder.MyPictures)),
                    ("Музыка", "🎵", Environment.GetFolderPath(Environment.SpecialFolder.MyMusic)),
                    ("Видео", "🎬", Environment.GetFolderPath(Environment.SpecialFolder.MyVideos))
                };
                
                foreach (var (name, icon, path) in folders)
                {
                    if (Directory.Exists(path))
                    {
                        var item = new TreeViewItem
                        {
                            Header = new StackPanel
                            {
                                Orientation = Orientation.Horizontal,
                                Spacing = 5,
                                Children =
                                {
                                    new TextBlock { Text = icon },
                                    new TextBlock { Text = name }
                                }
                            },
                            Tag = path
                        };
                        quickAccess.Items.Add(item);
                    }
                }
            }
        }
        
        private Button CreateRibbonButton(string text, string icon, EventHandler<RoutedEventArgs> handler, bool isLarge = false)
        {
            var button = new Button
            {
                Content = new StackPanel
                {
                    Orientation = Orientation.Vertical,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    VerticalAlignment = VerticalAlignment.Center,
                    Spacing = 3,
                    MinWidth = isLarge ? 70 : 60,
                    Margin = new Thickness(2)
                },
                Height = isLarge ? 65 : 55,
                Width = isLarge ? 70 : 60,
                Padding = new Thickness(0),
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(0),
                CornerRadius = new CornerRadius(2),
                Cursor = new Cursor(StandardCursorType.Hand)
            };
            
            var contentStack = button.Content as StackPanel;
            if (contentStack != null)
            {
                // Иконка
                var iconBlock = new TextBlock 
                { 
                    Text = icon, 
                    FontSize = isLarge ? 20 : 16,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    Margin = new Thickness(0, 2, 0, 2)
                };
                contentStack.Children.Add(iconBlock);
                
                // Текст
                var textBlock = new TextBlock 
                { 
                    Text = text, 
                    FontSize = 10,
                    TextWrapping = TextWrapping.Wrap,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    TextAlignment = TextAlignment.Center,
                    MaxWidth = 65
                };
                contentStack.Children.Add(textBlock);
            }
            
            if (handler != null)
                button.Click += handler;
            
            // Стиль при наведении
            button.PointerEntered += (s, e) =>
            {
                button.Background = new SolidColorBrush(Color.Parse("#E5F1FB"));
                button.BorderBrush = new SolidColorBrush(Color.Parse("#C5E0FA"));
                button.BorderThickness = new Thickness(1);
            };
            
            button.PointerExited += (s, e) =>
            {
                button.Background = Brushes.Transparent;
                button.BorderThickness = new Thickness(0);
            };
            
            button.PointerPressed += (s, e) =>
            {
                button.Background = new SolidColorBrush(Color.Parse("#C5E0FA"));
            };
            
            button.PointerReleased += (s, e) =>
            {
                button.Background = new SolidColorBrush(Color.Parse("#E5F1FB"));
            };
            
            return button;
        }
        
        private Button CreateViewModeButton(string text, string icon, ViewMode mode, bool isSelected = false)
        {
            var button = CreateRibbonButton(text, icon, (s, e) => CurrentViewMode = mode);
            
            if (isSelected)
            {
                button.Background = Brushes.LightBlue;
                button.BorderThickness = new Thickness(1);
                button.BorderBrush = Brushes.Blue;
            }
            
            return button;
        }
        
        private CheckBox CreateToggleButton(string text, string icon, bool isChecked, EventHandler<RoutedEventArgs> handler)
        {
            var checkBox = new CheckBox
            {
                Content = new StackPanel
                {
                    Orientation = Orientation.Horizontal,
                    Spacing = 5,
                    Children =
                    {
                        new TextBlock { Text = icon },
                        new TextBlock { Text = text, FontSize = 11 }
                    }
                },
                IsChecked = isChecked,
                Margin = new Thickness(2)
            };
            
            if (handler != null)
                checkBox.Click += handler;
            
            return checkBox;
        }
        
        private ComboBox CreateRibbonCombo(string placeholder, string[] items)
        {
            var combo = new ComboBox
            {
                ItemsSource = items,
                SelectedIndex = 0,
                Margin = new Thickness(2),
                MinWidth = 140,
                Height = 25,
                VerticalContentAlignment = VerticalAlignment.Center,
                FontSize = 11,
                Background = Brushes.White,
                BorderBrush = new SolidColorBrush(Color.Parse("#CCCCCC")),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(2)
            };
            
            // Стиль для выпадающего списка
            combo.DropDownOpened += (s, e) =>
            {
                combo.Background = new SolidColorBrush(Color.Parse("#F0F0F0"));
            };
            
            combo.DropDownClosed += (s, e) =>
            {
                combo.Background = Brushes.White;
            };
            
            return combo;
        }
        
        private Button CreateRibbonSplitButton(string text, string icon, string[] menuItems)
        {
            var button = new Button
            {
                Content = new StackPanel
                {
                    Orientation = Orientation.Horizontal,
                    Spacing = 5,
                    Children =
                    {
                        new TextBlock { Text = icon, FontSize = 16 },
                        new TextBlock { Text = text, FontSize = 11 }
                    }
                },
                Margin = new Thickness(2),
                Padding = new Thickness(10, 5),
                Height = 30,
                Background = Brushes.Transparent,
                BorderThickness = new Thickness(1),
                BorderBrush = new SolidColorBrush(Color.Parse("#CCCCCC")),
                CornerRadius = new CornerRadius(2)
            };
            
            // Создаем контекстное меню
            var contextMenu = new ContextMenu();
            foreach (var item in menuItems)
            {
                var menuItem = new MenuItem { Header = item };
                contextMenu.Items.Add(menuItem);
            }
            
            button.ContextMenu = contextMenu;
            
            // Обработчик клика для открытия меню
            button.Click += (s, e) =>
            {
                contextMenu.Open(button);
            };
            
            // Эффекты при наведении - используем PointerEntered/Exited
            button.PointerEntered += (s, e) =>
            {
                button.Background = new SolidColorBrush(Color.Parse("#F0F0F0"));
            };
            
            button.PointerExited += (s, e) =>
            {
                button.Background = Brushes.Transparent;
            };
            
            return button;
        }
        
        private void InitializeSorting()
        {
            var sortCombo = this.FindControl<ComboBox>("SortCombo");
            if (sortCombo != null)
            {
                sortCombo.SelectedIndex = 0;
                sortCombo.SelectionChanged += (s, e) => SortItems();
            }
        }
        
        private void SortItems()
        {
            // Реализация сортировки элементов
            var items = Items.ToList();
            items.Sort((a, b) => a.Name.CompareTo(b.Name));
            Items = new ObservableCollection<FileItem>(items);
        }
        
        private void UpdateViewMode()
        {
            // Синхронизируем с свойствами привязки
            if (CurrentViewMode == ViewMode.Details || CurrentViewMode == ViewMode.List)
            {
                IsDetailsView = true;
                IsIconsView = false;
            }
            else
            {
                IsDetailsView = false;
                IsIconsView = true;
            }
            
            var filesDataGrid = this.FindControl<DataGrid>("FilesDataGrid");
            var filesIconsControl = this.FindControl<ItemsControl>("FilesIconsControl");
            
            // Обновляем видимость элементов управления
            if (filesDataGrid != null)
                filesDataGrid.IsVisible = IsDetailsView;
            
            if (filesIconsControl != null)
                filesIconsControl.IsVisible = IsIconsView;
            
            StatusText = $"Режим просмотра: {CurrentViewMode}";
            
            // Обновляем иконки кнопок вида (если есть в XAML)
            var viewDetailsButton = this.FindControl<Button>("ViewDetailsButton");
            var viewLargeIconsButton = this.FindControl<Button>("ViewLargeIconsButton");
            
            if (viewDetailsButton != null)
            {
                viewDetailsButton.Background = IsDetailsView ? 
                    new SolidColorBrush(Color.Parse("#E0E0E0")) : Brushes.Transparent;
            }
            
            if (viewLargeIconsButton != null)
            {
                viewLargeIconsButton.Background = IsIconsView ? 
                    new SolidColorBrush(Color.Parse("#E0E0E0")) : Brushes.Transparent;
            }
        }

        private void UpdateRibbonHeight()
        {
            var ribbonPanel = this.FindControl<Border>("RibbonPanel");
            if (ribbonPanel != null)
            {
                ribbonPanel.Height = _isRibbonExpanded ? 130 : 40;
            }
        }

        // Обновите свойство IsRibbonExpanded для обновления высоты
        private bool _isRibbonExpanded = true;
        public bool IsRibbonExpanded
        {
            get => _isRibbonExpanded;
            set
            {
                _isRibbonExpanded = value;
                OnPropertyChanged(nameof(IsRibbonExpanded));
                UpdateRibbonHeight();
            }
        }

        private void UpdatePreviewVisibility()
        {
            var previewPanel = this.FindControl<Border>("PreviewPanel");
            if (previewPanel != null)
            {
                previewPanel.IsVisible = ShowPreview;
            }
        }
        
        private void UpdatePreview()
        {
            if (!ShowPreview || SelectedItem == null)
            {
                PreviewContent = null;
                return;
            }
            
            var previewPanel = new StackPanel
            {
                Margin = new Thickness(10),
                Spacing = 10
            };
            
            // Заголовок предпросмотра
            var header = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Spacing = 10,
                Margin = new Thickness(0, 0, 0, 10)
            };
            
            header.Children.Add(new TextBlock 
            { 
                Text = "ПРЕДПРОСМОТР",
                FontWeight = FontWeight.Bold,
                FontSize = 12
            });
            
            previewPanel.Children.Add(header);
            
            // Содержимое предпросмотра
            var content = new StackPanel
            {
                Spacing = 15
            };
            
            // Иконка и имя
            var titlePanel = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Spacing = 10
            };
            
            titlePanel.Children.Add(new TextBlock 
            { 
                Text = SelectedItem.Icon,
                FontSize = 32 
            });
            
            var namePanel = new StackPanel();
            namePanel.Children.Add(new TextBlock 
            { 
                Text = SelectedItem.Name,
                FontWeight = FontWeight.Bold,
                TextWrapping = TextWrapping.Wrap
            });
            
            if (!SelectedItem.IsDirectory)
            {
                namePanel.Children.Add(new TextBlock 
                { 
                    Text = SelectedItem.Type,
                    FontSize = 11,
                    Foreground = Brushes.Gray
                });
            }
            
            titlePanel.Children.Add(namePanel);
            content.Children.Add(titlePanel);
            
            // Разделитель
            content.Children.Add(new Separator());
            
            // Детали
            var details = new StackPanel { Spacing = 5 };
            
            if (!SelectedItem.IsDirectory)
            {
                AddDetail(details, "Тип:", SelectedItem.Type);
                AddDetail(details, "Размер:", SelectedItem.SizeDisplay);
            }
            
            AddDetail(details, "Изменен:", SelectedItem.ModifiedDisplay);
            AddDetail(details, "Создан:", File.GetCreationTime(SelectedItem.Path).ToString("dd.MM.yyyy HH:mm"));
            AddDetail(details, "Атрибуты:", GetAttributes(SelectedItem.Path));
            
            content.Children.Add(details);
            
            // Разделитель
            content.Children.Add(new Separator());
            
            // Кнопки действий
            var actions = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Spacing = 5
            };
            
            if (!SelectedItem.IsDirectory)
            {
                actions.Children.Add(CreatePreviewButton("Открыть", OnOpenClick));
                actions.Children.Add(CreatePreviewButton("Печать", OnPrintClick));
            }
            
            actions.Children.Add(CreatePreviewButton("Открыть расположение", OnOpenLocationClick));
            
            content.Children.Add(actions);
            
            previewPanel.Children.Add(content);
            
            PreviewContent = new ScrollViewer
            {
                Content = previewPanel,
                VerticalScrollBarVisibility = ScrollBarVisibility.Auto
            };
        }
        
        private void AddDetail(StackPanel panel, string label, string value)
        {
            var detail = new StackPanel
            {
                Orientation = Orientation.Horizontal,
                Spacing = 10
            };
            
            detail.Children.Add(new TextBlock 
            { 
                Text = label,
                FontWeight = FontWeight.SemiBold,
                MinWidth = 80
            });
            
            detail.Children.Add(new TextBlock 
            { 
                Text = value,
                TextWrapping = TextWrapping.Wrap
            });
            
            panel.Children.Add(detail);
        }
        
        private string GetAttributes(string path)
        {
            try
            {
                var attributes = File.GetAttributes(path);
                var attrList = new List<string>();
                
                if ((attributes & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
                    attrList.Add("Только чтение");
                if ((attributes & FileAttributes.Hidden) == FileAttributes.Hidden)
                    attrList.Add("Скрытый");
                if ((attributes & FileAttributes.System) == FileAttributes.System)
                    attrList.Add("Системный");
                
                return string.Join(", ", attrList);
            }
            catch
            {
                return "";
            }
        }
        
        private Button CreatePreviewButton(string text, EventHandler<RoutedEventArgs> handler)
        {
            return new Button
            {
                Content = text,
                Padding = new Thickness(8, 4),
                FontSize = 11,
                Background = Brushes.LightGray,
                BorderThickness = new Thickness(0)
            };
        }
        
        private void OnItemDoubleClick(object? sender, TappedEventArgs e)
        {
            if (SelectedItem != null)
            {
                if (SelectedItem.Name == ".." || SelectedItem.IsDirectory)
                {
                    NavigateToDirectory(SelectedItem.Path);
                }
                else
                {
                    OpenFile(SelectedItem.Path);
                }
            }
        }
        
        private void OpenFile(string path)
        {
            try
            {
                Process.Start(new ProcessStartInfo
                {
                    FileName = path,
                    UseShellExecute = true
                });
            }
            catch (Exception ex)
            {
                ShowMessage($"Не удалось открыть файл: {ex.Message}");
            }
        }
        
        // Обработчики кнопок навигации
        private void OnBackClick(object? sender, RoutedEventArgs e)
        {
            if (_backHistory.Count > 0)
            {
                _forwardHistory.Push(CurrentPath);
                var path = _backHistory.Pop();
                if (!string.IsNullOrEmpty(path))
                {
                    NavigateToDirectory(path);
                }
            }
        }
        
        private void OnForwardClick(object? sender, RoutedEventArgs e)
        {
            if (_forwardHistory.Count > 0)
            {
                _backHistory.Push(CurrentPath);
                var path = _forwardHistory.Pop();
                if (!string.IsNullOrEmpty(path))
                {
                    NavigateToDirectory(path);
                }
            }
        }
        
        private void OnUpClick(object? sender, RoutedEventArgs e)
        {
            var parent = Directory.GetParent(CurrentPath);
            if (parent != null)
            {
                NavigateToDirectory(parent.FullName);
            }
        }

        private void OnToggleRibbonClick(object? sender, RoutedEventArgs e)
        {
            IsRibbonExpanded = !IsRibbonExpanded;
            StatusText = IsRibbonExpanded ? "Лента развернута" : "Лента свернута";
        }

        private void OnSortChanged(object? sender, SelectionChangedEventArgs e)
        {
            if (sender is ComboBox combo && combo.SelectedItem != null)
            {
                StatusText = $"Сортировка: {combo.SelectedItem}";
            }
        }

        
        private void OnRefreshClick(object? sender, RoutedEventArgs e)
        {
            NavigateToDirectory(CurrentPath);
        }
        
        private void OnHomeClick(object? sender, RoutedEventArgs e)
        {
            NavigateToDirectory(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile));
        }
        
        private void OnRootClick(object? sender, RoutedEventArgs e)
        {
            NavigateToDirectory(Path.GetPathRoot(Environment.SystemDirectory) ?? "/");
        }
        
        private void OnAddressKeyDown(object? sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                var addressTextBox = this.FindControl<TextBox>("AddressTextBox");
                if (addressTextBox != null && !string.IsNullOrEmpty(addressTextBox.Text))
                {
                    NavigateToDirectory(addressTextBox.Text);
                }
            }
        }
        
        private void OnTreeSelectionChanged(object? sender, SelectionChangedEventArgs e)
        {
            var navigationTree = this.FindControl<TreeView>("NavigationTree");
            if (navigationTree?.SelectedItem is TreeViewItem item)
            {
                if (item.Tag is string path && !string.IsNullOrEmpty(path))
                {
                    NavigateToDirectory(path);
                }
            }
        }
        
        // Новые обработчики для дополнительных кнопок
        private async void OnCopyPathClick(object? sender, RoutedEventArgs e)
        {
            if (SelectedItem != null)
            {
                try
                {
                    // Получаем доступ к буферу обмена через TopLevel
                    var topLevel = TopLevel.GetTopLevel(this);
                    
                    if (topLevel?.Clipboard != null)
                    {
                        await topLevel.Clipboard.SetTextAsync(SelectedItem.Path);
                        ShowMessage("Путь скопирован в буфер обмена");
                    }
                    else
                    {
                        ShowMessage("Буфер обмена не доступен");
                    }
                }
                catch (Exception ex)
                {
                    ShowMessage($"Ошибка при копировании: {ex.Message}");
                }
            }
        }
        
        private void OnDeleteClick(object? sender, RoutedEventArgs e)
        {
            if (SelectedItem != null)
            {
                ShowMessage($"Удалить: {SelectedItem.Name}");
            }
        }
        
        private void OnShareClick(object? sender, RoutedEventArgs e)
        {
            ShowMessage("Настройки общего доступа");
        }
        
        private void OnEncryptClick(object? sender, RoutedEventArgs e)
        {
            ShowMessage("Защита паролем");
        }
        
        void OnOpenLocationClick(object? sender, RoutedEventArgs e)
        {
            if (SelectedItem != null)
            {
                Process.Start("explorer.exe", $"/select,\"{SelectedItem.Path}\"");
            }
        }
        
        // Существующие обработчики (оставляем для совместимости)
        private void OnCutClick(object? sender, RoutedEventArgs e) => ShowMessage("Вырезать");
        private void OnCopyClick(object? sender, RoutedEventArgs e) => ShowMessage("Копировать");
        private void OnPasteClick(object? sender, RoutedEventArgs e) => ShowMessage("Вставить");
        private void OnRenameClick(object? sender, RoutedEventArgs e) => ShowMessage("Переименовать");
        private void OnNewFolderClick(object? sender, RoutedEventArgs e) => ShowMessage("Новая папка");
        private void OnNewTextFileClick(object? sender, RoutedEventArgs e) => ShowMessage("Текстовый документ");
        private void OnOpenClick(object? sender, RoutedEventArgs e) => ShowMessage("Открыть");
        private void OnPrintClick(object? sender, RoutedEventArgs e) => ShowMessage("Печать");
        private void OnEmailClick(object? sender, RoutedEventArgs e) => ShowMessage("Отправить по email");
        private void OnZipClick(object? sender, RoutedEventArgs e) => ShowMessage("Сжать в ZIP");
        private void OnBurnClick(object? sender, RoutedEventArgs e) => ShowMessage("Записать на DVD");
        private void OnPropertiesClick(object? sender, RoutedEventArgs e) => ShowMessage("Свойства");
        
        private string FormatSize(long bytes)
        {
            if (bytes == 0) return "0 B";
            
            string[] sizes = { "B", "KB", "MB", "GB", "TB" };
            double len = bytes;
            int order = 0;
            
            while (len >= 1024 && order < sizes.Length - 1)
            {
                order++;
                len /= 1024;
            }
            
            return $"{len:0.#} {sizes[order]}";
        }
        
        private void ShowMessage(string message)
        {
            Dispatcher.UIThread.Post(() =>
            {
                StatusText = message;
            });
        }
        
        public new event PropertyChangedEventHandler? PropertyChanged;
        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private bool _canGoBack = false;
        public bool CanGoBack
        {
            get => _canGoBack;
            set
            {
                _canGoBack = value;
                OnPropertyChanged(nameof(CanGoBack));
            }
        }

        private bool _canGoForward = false;
        public bool CanGoForward
        {
            get => _canGoForward;
            set
            {
                _canGoForward = value;
                OnPropertyChanged(nameof(CanGoForward));
            }
        }

        private bool _canGoUp = false;
        public bool CanGoUp
        {
            get => _canGoUp;
            set
            {
                _canGoUp = value;
                OnPropertyChanged(nameof(CanGoUp));
            }
        }

        // Добавьте в MainWindow
        private void UpdatePreviewWindowsStyle()
        {
            if (SelectedItem == null)
            {
                PreviewContent = null;
                
                // Показываем сообщение "Выберите файл"
                var noPreviewMessage = this.FindControl<Border>("NoPreviewMessage");
                if (noPreviewMessage != null)
                    noPreviewMessage.IsVisible = true;
                
                return;
            }

            // Скрываем сообщение
            var noPreviewMessageControl = this.FindControl<Border>("NoPreviewMessage");
            if (noPreviewMessageControl != null)
                noPreviewMessageControl.IsVisible = false;

            // Обновляем атрибуты
            var attributesTextBlock = this.FindControl<TextBlock>("AttributesTextBlock");
            if (attributesTextBlock != null)
            {
                attributesTextBlock.Text = GetAttributes(SelectedItem.Path);
            }

            // Обновляем дату создания
            var createdDateText = this.FindControl<TextBlock>("CreatedDateText");
            if (createdDateText != null)
            {
                try
                {
                    var creationTime = File.GetCreationTime(SelectedItem.Path);
                    createdDateText.Text = creationTime.ToString("dd.MM.yyyy HH:mm");
                }
                catch
                {
                    createdDateText.Text = "Недоступно";
                }
            }

            // Создаем контент предпросмотра в стиле Windows
            var previewControl = this.FindControl<ContentControl>("PreviewContentControl");
            if (previewControl != null)
            {
                if (SelectedItem.IsDirectory)
                {
                    // Для папки
                    previewControl.Content = new TextBlock 
                    { 
                        Text = "📁", 
                        FontSize = 72,
                        Opacity = 0.8
                    };
                }
                else
                {
                    // Для файлов - пытаемся показать миниатюру или иконку
                    var extension = SelectedItem.Extension.ToLower();
                    
                    if (IsImageFile(extension))
                    {
                        // Для изображений пытаемся загрузить миниатюру
                        try
                        {
                            var bitmap = new Bitmap(SelectedItem.Path);
                            var image = new Image 
                            { 
                                Source = bitmap,
                                MaxWidth = 200,
                                MaxHeight = 150,
                                Stretch = Stretch.Uniform
                            };
                            previewControl.Content = image;
                        }
                        catch
                        {
                            // Если не удалось загрузить изображение, показываем иконку
                            previewControl.Content = CreateFilePreviewIcon(extension);
                        }
                    }
                    else if (IsTextFile(extension))
                    {
                        // Для текстовых файлов показываем превью содержимого
                        previewControl.Content = CreateTextPreview();
                    }
                    else
                    {
                        // Для остальных файлов - иконка
                        previewControl.Content = CreateFilePreviewIcon(extension);
                    }
                }
            }
        }

        private Control CreateFilePreviewIcon(string extension)
        {
            var icon = SelectedItem?.Icon ?? "📄";
            return new TextBlock 
            { 
                Text = icon, 
                FontSize = 72,
                Opacity = 0.8
            };
        }

        private Control CreateTextPreview()
        {
            var stack = new StackPanel 
            { 
                MaxWidth = 250,
                Spacing = 5
            };

            // Иконка текстового файла
            stack.Children.Add(new TextBlock 
            { 
                Text = "📄", 
                FontSize = 48,
                HorizontalAlignment = HorizontalAlignment.Center
            });

            // Превью первых строк
            try
            {
                var lines = File.ReadLines(SelectedItem.Path).Take(5);
                foreach (var line in lines)
                {
                    var textBlock = new TextBlock 
                    { 
                        Text = line.Length > 50 ? line.Substring(0, 50) + "..." : line,
                        FontSize = 10,
                        TextWrapping = TextWrapping.Wrap,
                        Foreground = Brushes.Gray
                    };
                    stack.Children.Add(textBlock);
                }
            }
            catch
            {
                // Не удалось прочитать файл
            }

            return stack;
        }

        private bool IsImageFile(string extension)
        {
            return new[] { ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".tiff" }.Contains(extension);
        }

        private bool IsTextFile(string extension)
        {
            return new[] { ".txt", ".md", ".cs", ".xml", ".json", ".html", ".css", ".js" }.Contains(extension);
        }

        // В конструкторе MainWindow или в методе инициализации:
        private ObservableCollection<DriveItem> _driveItems = new();
        public ObservableCollection<DriveItem> DriveItems
        {
            get => _driveItems;
            set
            {
                _driveItems = value;
                OnPropertyChanged(nameof(DriveItems));
            }
        }

        private void LoadDrives()
        {
            try
            {
                var drives = new ObservableCollection<DriveItem>();
                var systemDrives = DriveInfo.GetDrives();
                
                foreach (var drive in systemDrives)
                {
                    try
                    {
                        if (drive.IsReady)
                        {
                            // ПРОПУСКАЕМ диск, если его метка тома "appuser"
                            if (drive.VolumeLabel.Equals("appuser", StringComparison.OrdinalIgnoreCase))
                            {
                                continue;
                            }

                            drives.Add(new DriveItem
                            {
                                Name = $"{drive.Name} ({drive.VolumeLabel})",
                                Path = drive.Name,
                                TotalSize = drive.TotalSize,
                                FreeSpace = drive.AvailableFreeSpace,
                                DriveFormat = drive.DriveFormat,
                                DriveType = GetDriveTypeString(drive.DriveType),
                                Type = GetDriveTypeString(drive.DriveType)
                            });
                        }
                    }
                    catch (Exception ex)
                    {
                        Debug.WriteLine($"Error loading drive {drive.Name}: {ex.Message}");
                    }
                }
                
                DriveItems = drives;
                
                // Обновляем заголовок
                var currentFolderText = this.FindControl<TextBlock>("CurrentFolderText");
                if (currentFolderText != null)
                {
                    currentFolderText.Text = $"Этот компьютер ({DriveItems.Count})";
                }
                
                // Обновляем статус
                StatusText = $"Загружено {DriveItems.Count} дисков";
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Error loading drives: {ex.Message}");
                ShowMessage($"Ошибка загрузки дисков: {ex.Message}");
            }
        }

        private string GetDriveTypeString(DriveType type)
        {
            return type switch
            {
                DriveType.Fixed => "Локальный диск",
                DriveType.Network => "Сетевой диск",
                DriveType.CDRom => "CD/DVD диск",
                DriveType.Removable => "Съемный диск",
                DriveType.Ram => "RAM диск",
                _ => "Неизвестный диск"
            };
        } 

        private void OnDriveDoubleTapped(object? sender, RoutedEventArgs e)
        {
            DriveItem? driveToNavigate = null;
            
            // 1. Если кликнули в Таблице (DataGrid)
            if (sender is DataGrid)
            {
                driveToNavigate = SelectedDrive;
            }
            // 2. Если кликнули по Иконке (Border)
            else if (sender is Border border && border.DataContext is DriveItem driveItem)
            {
                driveToNavigate = driveItem;
            }
            
            // Переход в папку
            if (driveToNavigate != null)
            {
                NavigateToDirectory(driveToNavigate.Path);
            }
            
            e.Handled = true; // Говорим системе, что мы обработали клик
        }
    }
    
    public class FileItem : INotifyPropertyChanged
    {
        private string _name = string.Empty;
        public string Name
        {
            get => _name;
            set
            {
                _name = value;
                OnPropertyChanged(nameof(Name));
            }
        }
        public bool IsVisible => !Name.Equals("appuser", StringComparison.OrdinalIgnoreCase);
        public string Path { get; set; } = string.Empty;
        public bool IsDirectory { get; set; }
        public long Size { get; set; }
        public DateTime Modified { get; set; }
        public string Extension { get; set; } = string.Empty;
        
        public string Type => IsDirectory ? "Папка" : GetFileType(Extension);
        public string SizeDisplay => IsDirectory ? "" : FormatSize(Size);
        public string ModifiedDisplay => Modified.ToString("dd.MM.yyyy HH:mm");
        public string Icon => GetIcon();
        
        private string GetIcon()
        {
            if (IsDirectory) return "📁";
            
            return Extension.ToLower() switch
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
        
        private string GetFileType(string extension)
        {
            return extension.ToLower() switch
            {
                ".txt" => "Текстовый файл",
                ".pdf" => "PDF документ",
                ".jpg" or ".jpeg" => "Изображение JPEG",
                ".png" => "Изображение PNG",
                ".mp3" => "Аудио файл",
                ".mp4" => "Видео файл",
                ".zip" => "Архив ZIP",
                ".exe" => "Приложение",
                ".cs" => "Исходный код C#",
                _ => "Файл" + extension
            };
        }
        
        private string FormatSize(long bytes)
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
        
        public event PropertyChangedEventHandler? PropertyChanged;
        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }

    public class DriveItem : INotifyPropertyChanged
    {
        public bool IsVisible => !Name.Equals("appuser", StringComparison.OrdinalIgnoreCase);
        public string Name { get; set; } = string.Empty;
        public string Path { get; set; } = string.Empty;
        public string Type { get; set; } = "Локальный диск";
        public long TotalSize { get; set; }
        public long FreeSpace { get; set; }
        public string DriveFormat { get; set; } = "NTFS";
        public string DriveType { get; set; } = "Fixed";
        
        // Добавляем свойство для процента использования
        public double UsagePercentage 
        {
            get
            {
                if (TotalSize > 0)
                {
                    return 100 - ((double)FreeSpace / TotalSize * 100);
                }
                return 0;
            }
        }
        
        public string Icon => "💽";
        public string TotalSizeDisplay => FormatSize(TotalSize);
        public string FreeSpaceDisplay => FormatSize(FreeSpace);
        
        private string FormatSize(long bytes)
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
            return $"{len:0.#} {sizes[order]}";
        }
        
        public event PropertyChangedEventHandler? PropertyChanged;
        protected virtual void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}