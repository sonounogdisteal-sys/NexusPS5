#pragma once

#include <filesystem>
#include <string>

namespace nexus::core {

[[nodiscard]] bool replace_file(
    const std::filesystem::path& temporary_path,
    const std::filesystem::path& destination_path,
    std::string& error
);

} // namespace nexus::core
