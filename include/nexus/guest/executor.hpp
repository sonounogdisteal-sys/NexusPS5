#pragma once

#include "nexus/core/logger.hpp"
#include "nexus/guest/cpu.hpp"
#include "nexus/guest/elf.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace nexus::guest {

struct ExecutionOptions {
    std::uint64_t stack_base{0x00007fff00000000ULL};
    std::uint64_t stack_size{1024 * 1024};
    std::uint64_t instruction_limit{1'000'000};
};

struct ExecutionResult {
    StopReason reason{StopReason::fault};
    std::uint64_t exit_code{};
    std::uint64_t instructions_executed{};
    std::optional<Error> error;
};

class GuestExecutor {
public:
    explicit GuestExecutor(core::Logger* logger = nullptr);

    ExecutionResult run(
        std::span<const std::byte> executable,
        const ExecutionOptions& options = {}
    ) const;

private:
    core::Logger* logger_;
};

} // namespace nexus::guest
