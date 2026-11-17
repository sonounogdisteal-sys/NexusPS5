#pragma once

#include <iosfwd>
#include <string_view>

namespace nexus::core {

enum class LogLevel {
    debug,
    info,
    warning,
    error,
};

class Logger {
public:
    explicit Logger(std::ostream& output, LogLevel minimum = LogLevel::info);

    void set_minimum_level(LogLevel minimum);
    void log(LogLevel level, std::string_view message) const;

private:
    std::ostream& output_;
    LogLevel minimum_;
};

} // namespace nexus::core
