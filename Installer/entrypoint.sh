#!/bin/bash
set -e

# --- Чтение пароля из переменной ---
if [ -z "$SSH_PASSWORD" ]; then
    echo "✗ Ошибка: Переменная окружения SSH_PASSWORD не установлена."
    exit 1
fi

echo "=== Запуск через SSH туннель (sshpass) ==="
echo "User: $(whoami)"
echo "UID: $(id -u)"
echo "GID: $(id -g)"

# --- Получение IP-адреса хоста ---
HOST_IP=$(ip route | grep default | awk '{print $3}')

# --- Создание SSH-туннеля ---
echo "Creating SSH tunnel to $HOST_IP..."
SSHPASS=$SSH_PASSWORD sshpass -e ssh -N -L 6002:/tmp/.X11-unix/X0 artem@$HOST_IP -o StrictHostKeyChecking=no &
SSH_PID=$!

# Очистка туннеля при завершении скрипта
trap "kill $SSH_PID 2>/dev/null || true" EXIT

# Ждём установку туннеля
echo "Waiting for tunnel to be established..."
sleep 5

# Проверка туннеля
if ! kill -0 $SSH_PID > /dev/null 2>&1; then
    echo "✗ Ошибка: Не удалось запустить процесс sshpass."
    exit 1
fi

# Устанавливаем DISPLAY
export DISPLAY=:02
echo "✓ DISPLAY is set to: $DISPLAY"

# --- Запуск приложения ---
echo ""
echo "=== Starting Application ==="
echo "Directory: $(pwd)"

# Запуск dotnet как foreground-процесс (контейнер будет жить, пока работает приложение)
dotnet FileManager.dll "$@"

# Если приложение завершится — контейнер завершится
echo "Application exited."
exit 0