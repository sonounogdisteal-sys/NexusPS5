#include "nexus/core/logger.hpp"

#include <array>
#include <ostream>

namespace nexus::core {
namespace {

constexpr std::array<std::string_view, 4> level_names{
    "DEBUG", "INFO", "WARNING", "ERROR",
};

constexpr auto index_of(LogLevel level) {
    return static_cast<std::size_t>(level);
}

} // namespace

Logger::Logger(std::ostream& output, LogLevel minimum)
    : output_(output), minimum_(minimum) {}

void Logger::set_minimum_level(LogLevel minimum) {
    minimum_ = minimum;
}

void Logger::log(LogLevel level, std::string_view message) const {
    if (index_of(level) < index_of(minimum_)) {
        return;
    }

    output_ << '[' << level_names[index_of(level)] << "] " << message << '\n';
}

} // namespace nexus::core
