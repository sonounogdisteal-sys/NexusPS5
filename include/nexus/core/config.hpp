#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace nexus::core {

struct Config {
    int render_width = 1280;
    int render_height = 720;
    bool fullscreen = false;
    bool high_contrast = false;
    std::string user_name = "Player";

    [[nodiscard]] std::optional<std::string> validate() const;
    [[nodiscard]] static bool load_from_file(
        const std::filesystem::path& path,
        Config& config,
        std::string& error
    );
    [[nodiscard]] bool save_to_file(const std::filesystem::path& path, std::string& error) const;
};

} // namespace nexus::core
