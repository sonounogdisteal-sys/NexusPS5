#include "nexus/core/config.hpp"
#include "nexus/core/logger.hpp"

#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <fstream>
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

    config.render_height = 720;
    config.user_name.assign(25, 'x');
    require(config.validate().has_value(), "oversized local profile names should be rejected");
}

void test_config_persistence() {
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path() /
        ("nexusps5-config-test-" + std::to_string(unique) + ".ini");

    nexus::core::Config original;
    original.render_width = 1920;
    original.render_height = 1080;
    original.fullscreen = true;
    original.high_contrast = true;
    original.user_name = "Local Player";
    std::string error;
    require(original.save_to_file(path, error), "configuration should be written");

    original.user_name = "Second Save";
    require(original.save_to_file(path, error), "existing configuration should be replaced");
    nexus::core::Config loaded;
    require(
        nexus::core::Config::load_from_file(path, loaded, error),
        "configuration should be loaded"
    );
    require(
        loaded.render_width == 1920 && loaded.render_height == 1080 &&
            loaded.fullscreen && loaded.high_contrast && loaded.user_name == "Second Save",
        "configuration round-trip should preserve settings"
    );

    {
        std::ofstream malformed(path, std::ios::trunc);
        malformed << "unknown=value\n";
    }
    const auto previous_width = loaded.render_width;
    require(
        !nexus::core::Config::load_from_file(path, loaded, error),
        "unknown configuration keys should be rejected"
    );
    require(loaded.render_width == previous_width, "failed load should not partially replace config");

    std::error_code cleanup_error;
    std::filesystem::remove(path, cleanup_error);
    std::filesystem::remove(path.string() + ".tmp", cleanup_error);
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
    test_config_persistence();
    test_logger_threshold();
}
