#ifndef STRING_UTILS_HPP
#define STRING_UTILS_HPP

#include <string>
#include <vector>
#include <sstream>

/**
 * Утилиты для работы со строками
 * Используются в демонстрации File Manager Pro
 */
namespace StringUtils {
    
    /**
     * Разделяет строку по разделителю
     */
    std::vector<std::string> split(const std::string& str, char delimiter);
    
    /**
     * Объединяет строки через разделитель
     */
    std::string join(const std::vector<std::string>& strings,
                     const std::string& delimiter);
    
    /**
     * Проверяет, начинается ли строка с префикса
     */
    bool startsWith(const std::string& str, const std::string& prefix);
    
    /**
     * Проверяет, заканчивается ли строка суффиксом
     */
    bool endsWith(const std::string& str, const std::string& suffix);
    
    /**
     * Форматирует строку (простой аналог std::format)
     */
    template<typename... Args>
    std::string format(const std::string& format, Args... args) {
        size_t size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
        std::vector<char> buf(size);
        snprintf(buf.data(), size, format.c_str(), args...);
        return std::string(buf.data(), buf.data() + size - 1);
    }
    
} // namespace StringUtils

#endif // STRING_UTILS_HPP