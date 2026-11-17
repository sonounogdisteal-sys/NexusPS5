#pragma once

#include <optional>
#include <string>

namespace nexus::core {

struct Config {
    int render_width = 1280;
    int render_height = 720;

    [[nodiscard]] std::optional<std::string> validate() const;
};

} // namespace nexus::core
