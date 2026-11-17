#pragma once

#include <cstdint>
#include <string>

namespace nexus::guest {

enum class ErrorCode {
    invalid_argument,
    unmapped_memory,
    permission_denied,
    address_overflow,
    invalid_instruction,
    unsupported_instruction,
    instruction_limit,
    invalid_elf,
    unsupported_elf,
    invalid_process_state,
    execution_fault,
    resource_exhausted,
};

struct Error {
    ErrorCode code{ErrorCode::execution_fault};
    std::uint64_t address{};
    std::string message;
};

} // namespace nexus::guest
