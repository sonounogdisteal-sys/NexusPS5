#include "nexus/core/config.hpp"

namespace nexus::core {

std::optional<std::string> Config::validate() const {
    if (render_width <= 0 || render_height <= 0) {
        return "Render dimensions must be positive.";
    }

    return std::nullopt;
}

} // namespace nexus::core
