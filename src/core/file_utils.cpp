#include "nexus/core/file_utils.hpp"

#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace nexus::core {

bool replace_file(
    const std::filesystem::path& temporary_path,
    const std::filesystem::path& destination_path,
    std::string& error
) {
    error.clear();
#if defined(_WIN32)
    if (MoveFileExW(
            temporary_path.c_str(),
            destination_path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        ) == 0) {
        error = "Windows error " + std::to_string(GetLastError());
        return false;
    }
    return true;
#else
    std::error_code filesystem_error;
    std::filesystem::rename(temporary_path, destination_path, filesystem_error);
    if (filesystem_error) {
        error = filesystem_error.message();
        return false;
    }
    return true;
#endif
}

} // namespace nexus::core
