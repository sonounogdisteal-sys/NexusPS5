#pragma once

#include "nexus/core/logger.hpp"
#include "nexus/guest/executor.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <span>
#include <string>

namespace nexus::guest {

enum class ProcessState {
    created,
    running,
    exited,
    failed,
};

enum class ThreadState {
    created,
    running,
    exited,
    failed,
};

class GuestThread {
public:
    explicit GuestThread(std::uint64_t id);

    std::uint64_t id() const;
    ThreadState state() const;
    bool start(Error& error);
    bool finish(bool succeeded, Error& error);

private:
    std::uint64_t id_;
    ThreadState state_{ThreadState::created};
};

class KernelMutex {
public:
    void lock();
    void unlock();
    bool try_lock();

private:
    std::mutex mutex_;
};

struct ProcessResult {
    ProcessState state{ProcessState::created};
    ExecutionResult execution;
    std::optional<Error> error;
};

class GuestProcess {
public:
    GuestProcess(std::uint64_t id, std::string name, core::Logger* logger = nullptr);

    std::uint64_t id() const;
    const std::string& name() const;
    ProcessState state() const;
    const GuestThread& main_thread() const;

    ProcessResult execute(
        std::span<const std::byte> executable,
        const GuestExecutor& executor
    );

private:
    std::uint64_t id_;
    std::string name_;
    core::Logger* logger_;
    ProcessState state_{ProcessState::created};
    GuestThread main_thread_;
};

} // namespace nexus::guest
