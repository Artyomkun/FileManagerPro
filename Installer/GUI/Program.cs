using Avalonia;
using Avalonia.X11; 
using System;

namespace FileManager
{
    partial class Program
    {
        [STAThread]
        public static void Main(string[] args) => BuildAvaloniaApp()
            .StartWithClassicDesktopLifetime(args);

        public static AppBuilder BuildAvaloniaApp()
            => AppBuilder.Configure<App>()
                // Попробуем такой порядок: сначала даем Avalonia возможность
                // самой определить платформу, а затем явно заставляем использовать X11.
                .UsePlatformDetect() 
                .UseX11()            
                .LogToTrace();
    }
}