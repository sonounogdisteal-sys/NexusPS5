#include "nexus/guest/kernel.hpp"

#include <utility>

namespace nexus::guest {

GuestThread::GuestThread(std::uint64_t id)
    : id_(id) {}

std::uint64_t GuestThread::id() const {
    return id_;
}

ThreadState GuestThread::state() const {
    return state_;
}

bool GuestThread::start(Error& error) {
    if (state_ != ThreadState::created) {
        error = Error{ErrorCode::invalid_process_state, id_, "Only a newly created guest thread can start"};
        return false;
    }
    state_ = ThreadState::running;
    error = {};
    return true;
}

bool GuestThread::finish(bool succeeded, Error& error) {
    if (state_ != ThreadState::running) {
        error = Error{ErrorCode::invalid_process_state, id_, "Only a running guest thread can finish"};
        return false;
    }
    state_ = succeeded ? ThreadState::exited : ThreadState::failed;
    error = {};
    return true;
}

void KernelMutex::lock() {
    mutex_.lock();
}

void KernelMutex::unlock() {
    mutex_.unlock();
}

bool KernelMutex::try_lock() {
    return mutex_.try_lock();
}

GuestProcess::GuestProcess(std::uint64_t id, std::string name, core::Logger* logger)
    : id_(id), name_(std::move(name)), logger_(logger), main_thread_(id) {}

std::uint64_t GuestProcess::id() const {
    return id_;
}

const std::string& GuestProcess::name() const {
    return name_;
}

ProcessState GuestProcess::state() const {
    return state_;
}

const GuestThread& GuestProcess::main_thread() const {
    return main_thread_;
}

ProcessResult GuestProcess::execute(
    std::span<const std::byte> executable,
    const GuestExecutor& executor
) {
    if (state_ != ProcessState::created) {
        Error error{ErrorCode::invalid_process_state, id_, "Guest process can be executed only once"};
        return ProcessResult{
            state_,
            ExecutionResult{StopReason::fault, 0, 0, error},
            std::move(error),
        };
    }

    Error thread_error;
    if (!main_thread_.start(thread_error)) {
        state_ = ProcessState::failed;
        if (logger_ != nullptr) {
            logger_->log(core::LogLevel::error, thread_error.message);
        }
        return ProcessResult{
            state_,
            ExecutionResult{StopReason::fault, 0, 0, thread_error},
            std::move(thread_error),
        };
    }
    state_ = ProcessState::running;
    if (logger_ != nullptr) {
        logger_->log(core::LogLevel::info, "Guest process started");
    }

    auto execution = executor.run(executable);
    const bool succeeded = execution.reason == StopReason::returned;
    state_ = succeeded ? ProcessState::exited : ProcessState::failed;
    if (!main_thread_.finish(succeeded, thread_error)) {
        state_ = ProcessState::failed;
        execution.error = thread_error;
    }
    if (logger_ != nullptr) {
        logger_->log(
            succeeded ? core::LogLevel::info : core::LogLevel::error,
            succeeded ? "Guest process exited" :
                (execution.error.has_value() ? execution.error->message : "Guest process failed")
        );
    }
    return ProcessResult{state_, execution, execution.error};
}

} // namespace nexus::guest
