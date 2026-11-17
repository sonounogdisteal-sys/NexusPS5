#include "nexus/guest/executor.hpp"

#include "nexus/guest/memory.hpp"

#include <array>
#include <limits>
#include <span>

namespace nexus::guest {
namespace {

ExecutionResult failed(Error error) {
    return ExecutionResult{StopReason::fault, 0, 0, std::move(error)};
}

void log_error(core::Logger* logger, const Error& error) {
    if (logger != nullptr) {
        logger->log(core::LogLevel::error, error.message);
    }
}

} // namespace

GuestExecutor::GuestExecutor(core::Logger* logger)
    : logger_(logger) {}

ExecutionResult GuestExecutor::run(
    std::span<const std::byte> executable,
    const ExecutionOptions& options
) const {
    ElfImage image;
    Error error;
    const Elf64Parser parser;
    if (!parser.parse(executable, image, error)) {
        log_error(logger_, error);
        return failed(std::move(error));
    }

    AddressSpace memory;
    const ElfLoader loader;
    if (!loader.load(executable, image, memory, error)) {
        log_error(logger_, error);
        return failed(std::move(error));
    }
    if (options.stack_size == 0 ||
        options.stack_base % AddressSpace::page_size != 0 ||
        options.stack_size % AddressSpace::page_size != 0 ||
        options.stack_size > std::numeric_limits<std::uint64_t>::max() - options.stack_base) {
        error = Error{ErrorCode::invalid_argument, options.stack_base, "Guest stack bounds must be page-aligned and valid"};
        log_error(logger_, error);
        return failed(std::move(error));
    }
    if (!memory.map(
            options.stack_base,
            options.stack_size,
            MemoryPermission::read | MemoryPermission::write,
            error
        )) {
        log_error(logger_, error);
        return failed(std::move(error));
    }

    std::array<std::byte, 8> return_address{};
    for (unsigned index = 0; index < return_address.size(); ++index) {
        return_address[index] = static_cast<std::byte>(
            (X64Interpreter::return_sentinel >> (index * 8U)) & 0xffU
        );
    }
    const auto stack_pointer = options.stack_base + options.stack_size - return_address.size();
    if (!memory.write(stack_pointer, return_address, error)) {
        log_error(logger_, error);
        return failed(std::move(error));
    }

    CpuState state;
    state.instruction_pointer = image.entry_point;
    state.registers[4] = stack_pointer;
    state.flags = 2;
    const auto cpu_result = X64Interpreter{}.run(memory, state, options.instruction_limit);
    if (cpu_result.error.has_value()) {
        log_error(logger_, *cpu_result.error);
    } else if (logger_ != nullptr) {
        logger_->log(core::LogLevel::info, "Guest execution returned to the NEXUS runtime");
    }
    return ExecutionResult{
        cpu_result.reason,
        cpu_result.exit_code,
        cpu_result.instructions_executed,
        cpu_result.error,
    };
}

} // namespace nexus::guest
