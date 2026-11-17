#include "nexus/core/config.hpp"
#include "nexus/core/file_utils.hpp"

#include <charconv>
#include <fstream>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace nexus::core {
namespace {

bool parse_integer(const std::string& value, int& result) {
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    return error == std::errc{} && end == value.data() + value.size();
}

bool parse_boolean(const std::string& value, bool& result) {
    if (value == "true") {
        result = true;
        return true;
    }
    if (value == "false") {
        result = false;
        return true;
    }
    return false;
}

} // namespace

std::optional<std::string> Config::validate() const {
    if (render_width <= 0 || render_height <= 0) {
        return "Render dimensions must be positive.";
    }
    if (render_width > 7680 || render_height > 4320) {
        return "Render dimensions exceed the supported 8K limit.";
    }
    if (user_name.empty() || user_name.size() > 24) {
        return "The local user name must contain 1 to 24 characters.";
    }
    for (const unsigned char character : user_name) {
        if (character < 32 || character == '=' || character == '\n' || character == '\r') {
            return "The local user name contains an unsupported character.";
        }
    }

    return std::nullopt;
}

bool Config::load_from_file(
    const std::filesystem::path& path,
    Config& config,
    std::string& error
) {
    error.clear();
    Config loaded;
    if (path.empty()) {
        error = "A configuration path is required.";
        return false;
    }

    std::error_code file_error;
    const bool exists = std::filesystem::exists(path, file_error);
    if (file_error) {
        error = "Could not inspect configuration file: " + file_error.message();
        return false;
    }
    if (!exists) {
        config = std::move(loaded);
        return true;
    }

    std::ifstream input(path);
    if (!input) {
        error = "Could not open configuration file: " + path.string();
        return false;
    }

    std::unordered_set<std::string> seen_keys;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            error = "Invalid configuration entry on line " + std::to_string(line_number) + ".";
            return false;
        }
        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        if (!seen_keys.insert(key).second) {
            error = "Duplicate configuration key on line " + std::to_string(line_number) + ".";
            return false;
        }

        if (key == "render_width") {
            if (!parse_integer(value, loaded.render_width)) {
                error = "Invalid render_width on line " + std::to_string(line_number) + ".";
                return false;
            }
        } else if (key == "render_height") {
            if (!parse_integer(value, loaded.render_height)) {
                error = "Invalid render_height on line " + std::to_string(line_number) + ".";
                return false;
            }
        } else if (key == "fullscreen") {
            if (!parse_boolean(value, loaded.fullscreen)) {
                error = "Invalid fullscreen value on line " + std::to_string(line_number) + ".";
                return false;
            }
        } else if (key == "high_contrast") {
            if (!parse_boolean(value, loaded.high_contrast)) {
                error = "Invalid high_contrast value on line " + std::to_string(line_number) + ".";
                return false;
            }
        } else if (key == "user_name") {
            loaded.user_name = value;
        } else {
            error = "Unknown configuration key on line " + std::to_string(line_number) + ": " + key;
            return false;
        }
    }
    if (input.bad()) {
        error = "Error while reading configuration file: " + path.string();
        return false;
    }
    if (const auto validation_error = loaded.validate()) {
        error = "Invalid configuration: " + *validation_error;
        return false;
    }

    config = std::move(loaded);
    return true;
}

bool Config::save_to_file(const std::filesystem::path& path, std::string& error) const {
    error.clear();
    if (path.empty()) {
        error = "A configuration path is required.";
        return false;
    }
    if (const auto validation_error = validate()) {
        error = "Invalid configuration: " + *validation_error;
        return false;
    }

    std::error_code filesystem_error;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), filesystem_error);
        if (filesystem_error) {
            error = "Could not create configuration directory: " + filesystem_error.message();
            return false;
        }
    }

    const std::filesystem::path temporary_path = path.string() + ".tmp";
    {
        std::ofstream output(temporary_path, std::ios::trunc);
        if (!output) {
            error = "Could not create temporary configuration file: " + temporary_path.string();
            return false;
        }
        output << "render_width=" << render_width << '\n'
               << "render_height=" << render_height << '\n'
               << "fullscreen=" << (fullscreen ? "true" : "false") << '\n'
               << "high_contrast=" << (high_contrast ? "true" : "false") << '\n'
               << "user_name=" << user_name << '\n';
        output.flush();
        if (!output) {
            error = "Could not write configuration file: " + temporary_path.string();
            output.close();
            std::filesystem::remove(temporary_path, filesystem_error);
            return false;
        }
    }

    std::string replace_error;
    if (!replace_file(temporary_path, path, replace_error)) {
        error = "Could not replace configuration file: " + replace_error;
        std::error_code cleanup_error;
        std::filesystem::remove(temporary_path, cleanup_error);
        return false;
    }
    return true;
}

} // namespace nexus::core
