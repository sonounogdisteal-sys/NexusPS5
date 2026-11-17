#include "nexus/core/config.hpp"
#include "nexus/core/logger.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void test_config_validation() {
    nexus::core::Config config;
    require(!config.validate(), "default render dimensions should be valid");

    config.render_width = 0;
    require(config.validate().has_value(), "zero render width should be rejected");

    config.render_width = 1280;
    config.render_height = -1;
    require(config.validate().has_value(), "negative render height should be rejected");
}

void test_logger_threshold() {
    std::ostringstream output;
    nexus::core::Logger logger(output, nexus::core::LogLevel::warning);
    logger.log(nexus::core::LogLevel::info, "hidden");
    logger.log(nexus::core::LogLevel::error, "visible");
    require(output.str() == "[ERROR] visible\n", "logger should filter below-threshold messages");

    logger.set_minimum_level(nexus::core::LogLevel::debug);
    logger.log(nexus::core::LogLevel::debug, "now visible");
    require(output.str().ends_with("[DEBUG] now visible\n"), "logger should accept updated threshold");
}

} // namespace

int main() {
    test_config_validation();
    test_logger_threshold();
}
