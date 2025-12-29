FROM mcr.microsoft.com/dotnet/sdk:8.0

# 1. Установка ВСЕХ зависимостей за один раз
RUN apt-get update && \
    apt-get install -y \
    # Инструменты сборки
    gcc g++ cmake make libuuid1 uuid-dev \
    # X11 зависимости
    xauth x11-apps x11-utils \
    libx11-dev libxrandr-dev libxcursor-dev libxi-dev \
    # Avalonia зависимости
    libfontconfig1 libfreetype6 libglib2.0-0 \
    libgstreamer1.0-0 libgstreamer-plugins-base1.0-0 \
    libharfbuzz0b libicu72 libpango-1.0-0 \
    libxext6 libxcomposite1 libxtst6 libnss3 \
    libgtk-3-0 libgdk-pixbuf2.0-0 \
    # Wayland (опционально)
    libwayland-client0 libwayland-cursor0 libwayland-egl1 \
    mesa-utils mesa-vulkan-drivers \
    # Дополнительные утилиты
    dbus-x11 pulseaudio pulseaudio-utils \
    iproute2 \
    netcat-openbsd \
    openssh-client \
    sshpass \
    expect \
    && rm -rf /var/lib/apt/lists/*

# 2. ОТКЛЮЧЕНИЕ ТЕЛЕМЕТРИИ (КРИТИЧЕСКИ ВАЖНО!)
ENV DOTNET_CLI_TELEMETRY_OPTOUT=1
ENV MSBUILDDISABLETELEMETRY=1
ENV AVALONIA_TELEMETRY_OPTOUT=1
ENV AVALONIA_DISABLE_LICENSE_CHECK=1

# 3. Установка рабочей директории
WORKDIR /app

# 4. Копирование всех файлов проекта
COPY . .

# 5. СОЗДАНИЕ ДИРЕКТОРИИ ДЛЯ AVALONIA
RUN mkdir -p /root/.local/share/AvaloniaUI/Licensing/Tickets/v2 && \
    mkdir -p /home/appuser/.local/share/AvaloniaUI && \
    chmod -R 755 /root/.local && \
    chmod -R 755 /home/appuser/.local

# 6. Создание пользователя для приложения
RUN groupadd -g 1000 appuser || true && \
    useradd -u 1000 -g 1000 -m -s /bin/bash appuser && \
    mkdir -p /home/appuser/.local/share/AvaloniaUI && \
    chown -R appuser:appuser /home/appuser

# 7. Сборка C библиотеки через CMake
RUN cd /app/Installer/GUI/Logic/Core && \
    rm -rf build 2>/dev/null || true && \
    mkdir -p build && \
    cd build && \
    cmake .. && \
    make

# 8. Сборка C++ навигатора
RUN cd /app/Installer/GUI/Logic && \
    g++ -c -fPIC navigator.cpp -o navigator.o && \
    g++ -shared -o libnavigator.so navigator.o

# 9. Копирование библиотек в нужное место
RUN cp -f /app/Installer/GUI/Logic/Core/build/bin/libfileops.so /app/Installer/GUI/ 2>/dev/null || true && \
    cp -f /app/Installer/GUI/Logic/libnavigator.so /app/Installer/GUI/ 2>/dev/null || true

# 10. Восстановление NuGet пакетов
RUN cd /app/Installer/GUI && \
    dotnet restore --disable-parallel --no-cache --ignore-failed-sources

# 11. Сборка C# приложения
RUN cd /app/Installer/GUI && \
    dotnet build -c Release \
    -p:AvaloniaEnableMSBuildTelemetry=false \
    -p:AvaloniaTelemetryEnabled=false \
    -p:AvaloniaLicenseCheckEnabled=false \
    -p:DisableAvaloniaTelemetry=true

# 12. Публикация приложения
RUN cd /app/Installer/GUI && \
    dotnet publish -c Release -o /app/publish \
    -r linux-x64 --self-contained true \
    -p:PublishSingleFile=false

# 12.1. Установка прав доступа для appuser
RUN chown -R appuser:appuser /app/publish

# 13. Копируем и настраиваем entrypoint.sh
COPY Installer/entrypoint.sh /app/publish/entrypoint.sh
RUN chmod +x /app/publish/entrypoint.sh

# 14. Установка рабочей директории и пользователя
USER appuser
WORKDIR /app/publish

ENTRYPOINT ["./entrypoint.sh"]