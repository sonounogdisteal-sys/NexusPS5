#pragma once

#include "nexus/guest/kernel.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace nexus::runtime {

struct ApplicationMetadata {
    std::string id;
    std::string title;
    bool is_mock_data{};
};

enum class EventType {
    application_started,
    process_status_changed,
    application_completed,
    error_notification,
    return_to_shell,
};

struct ApplicationEvent {
    EventType type{EventType::process_status_changed};
    ApplicationMetadata application;
    std::uint64_t process_id{};
    guest::ProcessState process_status{guest::ProcessState::created};
    std::uint64_t exit_code{};
    std::string message;
};

class ApplicationEventSink {
public:
    virtual ~ApplicationEventSink() = default;
    virtual void on_application_event(const ApplicationEvent& event) = 0;
};

class ApplicationEventPublisher {
public:
    explicit ApplicationEventPublisher(ApplicationEventSink& sink);
    void publish(const ApplicationEvent& event) const;

private:
    ApplicationEventSink& sink_;
};

class ApplicationRuntime {
public:
    guest::ProcessResult execute(
        guest::GuestProcess& process,
        std::span<const std::byte> executable,
        const guest::GuestExecutor& executor,
        const ApplicationMetadata& metadata,
        const ApplicationEventPublisher& publisher
    ) const;
};

} // namespace nexus::runtime
