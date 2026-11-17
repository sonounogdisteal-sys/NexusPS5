#include "nexus/runtime/application_events.hpp"

namespace nexus::runtime {

ApplicationEventPublisher::ApplicationEventPublisher(ApplicationEventSink& sink)
    : sink_(sink) {}

void ApplicationEventPublisher::publish(const ApplicationEvent& event) const {
    sink_.on_application_event(event);
}

guest::ProcessResult ApplicationRuntime::execute(
    guest::GuestProcess& process,
    std::span<const std::byte> executable,
    const guest::GuestExecutor& executor,
    const ApplicationMetadata& metadata,
    const ApplicationEventPublisher& publisher
) const {
    publisher.publish(ApplicationEvent{
        EventType::application_started,
        metadata,
        process.id(),
        process.state(),
        0,
        {},
    });

    const auto result = process.execute(executable, executor);
    publisher.publish(ApplicationEvent{
        EventType::process_status_changed,
        metadata,
        process.id(),
        result.state,
        result.execution.exit_code,
        result.error.has_value() ? result.error->message : std::string{},
    });
    if (result.state == guest::ProcessState::failed || result.error.has_value()) {
        publisher.publish(ApplicationEvent{
            EventType::error_notification,
            metadata,
            process.id(),
            result.state,
            0,
            result.error.has_value() ? result.error->message : "Guest process failed",
        });
        return result;
    }

    publisher.publish(ApplicationEvent{
        EventType::application_completed,
        metadata,
        process.id(),
        result.state,
        result.execution.exit_code,
        {},
    });
    publisher.publish(ApplicationEvent{
        EventType::return_to_shell,
        metadata,
        process.id(),
        result.state,
        result.execution.exit_code,
        {},
    });
    return result;
}

} // namespace nexus::runtime
