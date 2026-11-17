#pragma once

#include "nexus/guest/error.hpp"
#include "nexus/guest/memory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace nexus::guest {

struct CpuState {
    std::array<std::uint64_t, 16> registers{};
    std::uint64_t instruction_pointer{};
    std::uint64_t flags{2};
};

enum class StopReason {
    returned,
    fault,
    instruction_limit,
};

struct CpuResult {
    StopReason reason{StopReason::fault};
    std::uint64_t instructions_executed{};
    std::uint64_t exit_code{};
    std::optional<Error> error;
};

class X64Interpreter {
public:
    static constexpr std::uint64_t return_sentinel = 0xfffffffffffffff0ULL;

    CpuResult run(
        AddressSpace& memory,
        CpuState& state,
        std::uint64_t instruction_limit = 1'000'000
    ) const;
};

} // namespace nexus::guest
