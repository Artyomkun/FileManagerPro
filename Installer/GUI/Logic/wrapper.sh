#!/bin/bash
# wrapper.sh - File Manager для n8n
# Поддерживает: list, diskinfo, search, info, mkdir, delete, copy, move, rename, pwd
# Готовые workflow: disk-space-alert.json, file-registry-export.json, tmp-cleanup-automation.json

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY="$SCRIPT_DIR/output/navigator"
LOG_FILE="$SCRIPT_DIR/wrapper.log"
WORKFLOWS_DIR="$SCRIPT_DIR"

# Логирование
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" >> "$LOG_FILE"
}

# Показать помощь
show_help() {
    echo "File Manager для n8n"
    echo ""
    echo "Доступные команды:"
    echo "  list <path>          - список файлов в директории"
    echo "  diskinfo <path>      - информация о дисковом пространстве"
    echo "  search <pattern> [-r] [path] - поиск файлов"
    echo "  info <file>          - информация о файле"
    echo "  mkdir <dir>          - создать директорию"
    echo "  delete <path>        - удалить файл/директорию"
    echo "  copy <src> <dst>     - копировать файл"
    echo "  move <src> <dst>     - переместить/переименовать"
    echo "  rename <old> <new>   - переименовать"
    echo "  pwd                  - текущая директория"
    echo ""
    echo "Готовые workflow (импорт в n8n):"
    echo "  $WORKFLOWS_DIR/disk-space-alert.json         - Мониторинг диска"
    echo "  $WORKFLOWS_DIR/file-registry-export.json     - Экспорт списка файлов"
    echo "  $WORKFLOWS_DIR/tmp-cleanup-automation.json   - Очистка временных файлов"
    echo ""
    echo "Примеры:"
    echo "  $0 list /home"
    echo "  $0 diskinfo /"
    echo "  $0 search \"*.txt\" -r /var"
    echo "  $0 info /etc/passwd"
}

# Проверка аргументов
if [ $# -eq 0 ]; then
    show_help
    exit 0
fi

# Специальные команды
case "$1" in
    "--help"|"-h"|"help")
        show_help
        exit 0
        ;;
    "--version"|"-v")
        echo "File Manager для n8n v1.0"
        exit 0
        ;;
    "--workflows"|"-w")
        echo "Доступные workflow:"
        ls -la $WORKFLOWS_DIR/*.json 2>/dev/null || echo "  Нет workflow файлов"
        exit 0
        ;;
esac

log "Starting wrapper with args: $*"

# Проверки
if [ ! -f "$BINARY" ]; then
    ERROR_MSG="Binary not found: $BINARY"
    log "ERROR: $ERROR_MSG"
    echo "{\"error\":\"$ERROR_MSG\"}"
    exit 1
fi

if [ ! -x "$BINARY" ]; then
    chmod +x "$BINARY" 2>/dev/null || {
        ERROR_MSG="Cannot make binary executable"
        log "ERROR: $ERROR_MSG"
        echo "{\"error\":\"$ERROR_MSG\"}"
        exit 1
    }
fi

# Выполнение
log "Executing: $BINARY $*"
OUTPUT=$("$BINARY" "$@" 2>&1)
EXIT_CODE=$?

log "Exit code: $EXIT_CODE"
log "Output length: ${#OUTPUT}"

if [ $EXIT_CODE -ne 0 ]; then
    log "ERROR output: $OUTPUT"
    # Пробуем вернуть JSON ошибки если есть
    if echo "$OUTPUT" | grep -q '{.*}'; then
        echo "$OUTPUT"
    else
        echo "{\"error\":\"Command failed\", \"exitCode\":$EXIT_CODE}"
    fi
    exit $EXIT_CODE
else
    log "SUCCESS output: $OUTPUT"
    echo "$OUTPUT"
fi

exit 0