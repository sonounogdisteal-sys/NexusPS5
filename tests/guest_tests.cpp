#include "nexus/guest/cpu.hpp"
#include "nexus/guest/elf.hpp"
#include "nexus/guest/executor.hpp"
#include "nexus/guest/kernel.hpp"
#include "nexus/guest/memory.hpp"
#include "nexus/runtime/application_events.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <span>
#include <string>
#include <vector>

namespace {

using nexus::guest::AddressSpace;
using nexus::guest::Error;
using nexus::guest::ErrorCode;
using nexus::guest::MemoryPermission;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void put_le(std::vector<std::byte>& bytes, std::size_t offset, std::uint64_t value, unsigned count) {
    for (unsigned index = 0; index < count; ++index) {
        bytes[offset + index] = static_cast<std::byte>((value >> (index * 8U)) & 0xffU);
    }
}

const std::vector<std::byte>& test_program() {
    static const std::vector<std::byte> code{
        std::byte{0x48}, std::byte{0xb8}, std::byte{0x28}, std::byte{0}, std::byte{0},
        std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0x48}, std::byte{0xbb}, std::byte{0x02}, std::byte{0}, std::byte{0},
        std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0x48}, std::byte{0x01}, std::byte{0xd8},
        std::byte{0x48}, std::byte{0x83}, std::byte{0xf8}, std::byte{0x2a},
        std::byte{0x75}, std::byte{0x14},
        std::byte{0xe8}, std::byte{0x1a}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0x48}, std::byte{0x89}, std::byte{0x05}, std::byte{0xd7},
        std::byte{0x0f}, std::byte{0}, std::byte{0},
        std::byte{0x48}, std::byte{0x8b}, std::byte{0x05}, std::byte{0xd0},
        std::byte{0x0f}, std::byte{0}, std::byte{0},
        std::byte{0xc3},
        std::byte{0x48}, std::byte{0xb8}, std::byte{0x01}, std::byte{0}, std::byte{0},
        std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0xc3},
        std::byte{0x55},
        std::byte{0x48}, std::byte{0x89}, std::byte{0xe5},
        std::byte{0x48}, std::byte{0x83}, std::byte{0xc0}, std::byte{0x01},
        std::byte{0x5d},
        std::byte{0xc3},
    };
    return code;
}

std::vector<std::byte> make_test_elf() {
    constexpr std::size_t file_size = 0x2300;
    constexpr std::size_t program_header_offset = 64;
    constexpr std::size_t section_header_offset = 0x2200;
    constexpr std::size_t text_offset = 0x1000;
    constexpr std::size_t names_offset = 0x2100;
    const std::array<std::byte, 17> names{
        std::byte{0}, std::byte{'.'}, std::byte{'t'}, std::byte{'e'}, std::byte{'x'},
        std::byte{'t'}, std::byte{0}, std::byte{'.'}, std::byte{'s'}, std::byte{'h'},
        std::byte{'s'}, std::byte{'t'}, std::byte{'r'}, std::byte{'t'}, std::byte{'a'},
        std::byte{'b'}, std::byte{0},
    };
    std::vector<std::byte> file(file_size);
    file[0] = std::byte{0x7f};
    file[1] = std::byte{'E'};
    file[2] = std::byte{'L'};
    file[3] = std::byte{'F'};
    file[4] = std::byte{2};
    file[5] = std::byte{1};
    file[6] = std::byte{1};
    put_le(file, 16, 2, 2);
    put_le(file, 18, 62, 2);
    put_le(file, 20, 1, 4);
    put_le(file, 24, 0x400000, 8);
    put_le(file, 32, program_header_offset, 8);
    put_le(file, 40, section_header_offset, 8);
    put_le(file, 52, 64, 2);
    put_le(file, 54, 56, 2);
    put_le(file, 56, 2, 2);
    put_le(file, 58, 64, 2);
    put_le(file, 60, 3, 2);
    put_le(file, 62, 1, 2);

    put_le(file, program_header_offset, 1, 4);
    put_le(file, program_header_offset + 4, 5, 4);
    put_le(file, program_header_offset + 8, text_offset, 8);
    put_le(file, program_header_offset + 16, 0x400000, 8);
    put_le(file, program_header_offset + 32, test_program().size(), 8);
    put_le(file, program_header_offset + 40, AddressSpace::page_size, 8);
    put_le(file, program_header_offset + 48, AddressSpace::page_size, 8);

    constexpr std::size_t data_header = program_header_offset + 56;
    put_le(file, data_header, 1, 4);
    put_le(file, data_header + 4, 6, 4);
    put_le(file, data_header + 8, 0x2000, 8);
    put_le(file, data_header + 16, 0x401000, 8);
    put_le(file, data_header + 32, 0, 8);
    put_le(file, data_header + 40, AddressSpace::page_size, 8);
    put_le(file, data_header + 48, AddressSpace::page_size, 8);

    std::copy(test_program().begin(), test_program().end(), file.begin() + text_offset);
    std::copy(names.begin(), names.end(), file.begin() + names_offset);

    constexpr std::size_t string_section = section_header_offset + 64;
    put_le(file, string_section + 4, 3, 4);
    put_le(file, string_section + 24, names_offset, 8);
    put_le(file, string_section + 32, names.size(), 8);
    constexpr std::size_t text_section = section_header_offset + 128;
    put_le(file, text_section, 1, 4);
    put_le(file, text_section + 4, 1, 4);
    put_le(file, text_section + 8, 6, 8);
    put_le(file, text_section + 16, 0x400000, 8);
    put_le(file, text_section + 24, text_offset, 8);
    put_le(file, text_section + 32, test_program().size(), 8);
    put_le(file, text_section + 48, 16, 8);
    return file;
}

void test_memory_regions_and_permissions() {
    AddressSpace memory;
    Error error;
    require(
        memory.map(0x100000, AddressSpace::page_size, MemoryPermission::read | MemoryPermission::write, error),
        "page-aligned guest memory mapping should succeed"
    );
    require(
        memory.map(0x101000, AddressSpace::page_size, MemoryPermission::read | MemoryPermission::write, error),
        "adjacent guest region should map"
    );
    require(
        memory.map(0x102000, AddressSpace::page_size, MemoryPermission::read, error),
        "read-only guest region should map"
    );
    require(
        !memory.map(0x100000, AddressSpace::page_size, MemoryPermission::read, error) &&
            error.code == ErrorCode::invalid_argument,
        "overlapping memory mapping should be rejected"
    );
    require(
        !memory.map(0x100001, AddressSpace::page_size, MemoryPermission::read, error),
        "unaligned memory mapping should be rejected"
    );

    const std::array<std::byte, 4> bytes{
        std::byte{0x11}, std::byte{0x22}, std::byte{0x33}, std::byte{0x44},
    };
    require(memory.write(0x100ffe, bytes, error), "write should cross adjacent mapped pages");
    std::array<std::byte, 4> actual{};
    require(memory.read(0x100ffe, actual, error) && actual == bytes, "cross-page read should preserve bytes");
    require(
        !memory.write(0x102000, bytes, error) && error.code == ErrorCode::permission_denied,
        "write to a read-only guest page should fail"
    );
    require(
        !memory.fetch(0x100000, actual, error) && error.code == ErrorCode::permission_denied,
        "instruction fetch from a non-executable guest page should fail"
    );
    require(
        !memory.read(0x103000, actual, error) && error.code == ErrorCode::unmapped_memory,
        "read outside mapped guest ranges should fail"
    );
    std::array<std::byte, 2> before_failed_write{};
    require(memory.read(0x101ffe, before_failed_write, error), "preflight bytes should be readable");
    require(
        !memory.write(0x101ffe, bytes, error) && error.code == ErrorCode::permission_denied,
        "cross-boundary write into a read-only page should fail"
    );
    std::array<std::byte, 2> after_failed_write{};
    require(
        memory.read(0x101ffe, after_failed_write, error) &&
            after_failed_write == before_failed_write,
        "failed cross-boundary writes should not partially modify guest memory"
    );
    require(
        !memory.map(0xfffffffffffff000ULL, AddressSpace::page_size * 2, MemoryPermission::read, error) &&
            error.code == ErrorCode::address_overflow,
        "mapping arithmetic overflow should be rejected"
    );
    require(memory.unmap(0x100000, error), "exact guest region should unmap");
    require(
        !memory.unmap(0x100000, error) && error.code == ErrorCode::unmapped_memory,
        "unmapping a missing region should fail explicitly"
    );
}

void test_cpu_instruction_subset_and_faults() {
    AddressSpace memory;
    Error error;
    require(memory.map(0x100000, AddressSpace::page_size, MemoryPermission::read | MemoryPermission::execute, error),
        "CPU test code page should map");
    require(memory.map(0x200000, AddressSpace::page_size, MemoryPermission::read | MemoryPermission::write, error),
        "CPU test stack page should map");
    const std::array<std::byte, 6> code{
        std::byte{0xb8}, std::byte{42}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0xc3},
    };
    require(memory.initialize(0x100000, code, error), "code bytes should initialize");
    std::array<std::byte, 8> sentinel{};
    for (unsigned index = 0; index < sentinel.size(); ++index) {
        sentinel[index] = static_cast<std::byte>(
            (nexus::guest::X64Interpreter::return_sentinel >> (index * 8U)) & 0xffU
        );
    }
    require(memory.write(0x200ff8, sentinel, error), "return sentinel should be set up on stack");
    nexus::guest::CpuState state;
    state.instruction_pointer = 0x100000;
    state.registers[4] = 0x200ff8;
    const auto result = nexus::guest::X64Interpreter{}.run(memory, state);
    require(result.reason == nexus::guest::StopReason::returned, "RET should stop at the host return sentinel");
    require(result.exit_code == 42 && result.instructions_executed == 2, "MOV EAX,imm32 and RET should execute");

    const std::array<std::byte, 27> arithmetic{
        std::byte{0x48}, std::byte{0xb8}, std::byte{5}, std::byte{0}, std::byte{0},
        std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0x48}, std::byte{0x83}, std::byte{0xc0}, std::byte{3},
        std::byte{0x48}, std::byte{0x83}, std::byte{0xe8}, std::byte{2},
        std::byte{0x48}, std::byte{0x31}, std::byte{0xc0},
        std::byte{0xb8}, std::byte{7}, std::byte{0}, std::byte{0}, std::byte{0},
        std::byte{0xc3},
    };
    require(memory.initialize(0x100000, arithmetic, error), "arithmetic code should initialize");
    nexus::guest::CpuState arithmetic_state;
    arithmetic_state.instruction_pointer = 0x100000;
    arithmetic_state.registers[4] = 0x200ff8;
    const auto arithmetic_result = nexus::guest::X64Interpreter{}.run(memory, arithmetic_state);
    require(
        arithmetic_result.reason == nexus::guest::StopReason::returned &&
            arithmetic_result.exit_code == 7,
        "64-bit ADD, SUB, XOR and 32-bit MOV should execute"
    );

    nexus::guest::CpuState unsupported;
    unsupported.instruction_pointer = 0x100000;
    const std::array<std::byte, 1> hlt{std::byte{0xf4}};
    require(memory.initialize(0x100000, hlt, error), "unsupported opcode should initialize");
    const auto unsupported_result = nexus::guest::X64Interpreter{}.run(memory, unsupported);
    require(
        unsupported_result.reason == nexus::guest::StopReason::fault &&
            unsupported_result.error->code == ErrorCode::unsupported_instruction,
        "unimplemented instructions should return a structured error"
    );
    require(
        unsupported.instruction_pointer == 0x100000,
        "faulting guest instruction pointer should remain on the faulting instruction"
    );

    const std::array<std::byte, 7> invalid_read{
        std::byte{0x48}, std::byte{0x8b}, std::byte{0x05}, std::byte{0},
        std::byte{0x40}, std::byte{0}, std::byte{0},
    };
    require(memory.initialize(0x100000, invalid_read, error), "faulting memory instruction should initialize");
    nexus::guest::CpuState faulted;
    faulted.instruction_pointer = 0x100000;
    const auto memory_fault = nexus::guest::X64Interpreter{}.run(memory, faulted);
    require(
        memory_fault.reason == nexus::guest::StopReason::fault &&
            memory_fault.error->code == ErrorCode::unmapped_memory &&
            faulted.instruction_pointer == 0x100000,
        "guest memory faults should be reported without advancing the faulting instruction"
    );

    nexus::guest::CpuState limited;
    limited.instruction_pointer = 0x100000;
    const std::array<std::byte, 2> loop{std::byte{0xeb}, std::byte{0xfe}};
    require(memory.initialize(0x100000, loop, error), "loop bytes should initialize");
    const auto limit_result = nexus::guest::X64Interpreter{}.run(memory, limited, 5);
    require(
        limit_result.reason == nexus::guest::StopReason::instruction_limit &&
            limit_result.instructions_executed == 5,
        "instruction budget should stop non-terminating guest code"
    );
}

void test_elf_parsing_and_loading() {
    const auto file = make_test_elf();
    nexus::guest::ElfImage image;
    Error error;
    const nexus::guest::Elf64Parser parser;
    require(parser.parse(file, image, error), "original x86-64 ELF fixture should parse");
    require(image.entry_point == 0x400000 && image.segments.size() == 2, "ELF entry and load segments should parse");
    require(image.sections.size() == 3 && image.sections[2].name == ".text", "ELF section names should be inspected");

    AddressSpace memory;
    require(nexus::guest::ElfLoader{}.load(file, image, memory, error), "static test ELF should load");
    std::array<std::byte, 1> byte{};
    require(memory.fetch(0x400000, byte, error), "loaded code should have execute permission");
    require(
        !memory.write(0x400000, byte, error) && error.code == ErrorCode::permission_denied,
        "loaded text should not be writable"
    );
    require(memory.read(0x401000, byte, error) && byte[0] == std::byte{0}, "BSS memory should be zero-filled");

    auto invalid = file;
    invalid[0] = std::byte{0};
    require(!parser.parse(invalid, image, error) && error.code == ErrorCode::invalid_elf,
        "invalid ELF magic should be rejected");

    auto invalid_entry = file;
    put_le(invalid_entry, 24, 0x401000, 8);
    require(
        !parser.parse(invalid_entry, image, error) && error.code == ErrorCode::invalid_elf,
        "entry point outside executable segments should be rejected"
    );

    auto invalid_names = file;
    put_le(invalid_names, 0x2200 + 64 + 4, 1, 4);
    require(
        !parser.parse(invalid_names, image, error) && error.code == ErrorCode::invalid_elf,
        "section name index must identify a string table"
    );

    auto dynamic = file;
    put_le(dynamic, 16, 3, 2);
    require(!parser.parse(dynamic, image, error) && error.code == ErrorCode::unsupported_elf,
        "ET_DYN must not be misrepresented as a supported executable");

    auto relocation = file;
    put_le(relocation, 0x2200 + 128 + 4, 4, 4);
    put_le(relocation, 0x2200 + 128 + 8, 2, 8);
    require(parser.parse(relocation, image, error), "ELF relocation metadata should remain inspectable");
    AddressSpace relocation_memory;
    require(
        !nexus::guest::ElfLoader{}.load(relocation, image, relocation_memory, error) &&
            error.code == ErrorCode::unsupported_elf &&
            !relocation_memory.mapped(0x400000, 1),
        "unsupported relocation loading should fail without leaving partial guest mappings"
    );
}

void test_guest_execution_and_process_lifecycle() {
    const auto file = make_test_elf();
    const nexus::guest::GuestExecutor executor;
    const auto result = executor.run(file);
    if (result.error.has_value()) {
        std::cerr << "Guest execution error: " << result.error->message
                  << " at 0x" << std::hex << result.error->address << '\n';
    }
    require(result.reason == nexus::guest::StopReason::returned, "guest ELF should return cleanly");
    require(result.exit_code == 43, "guest should execute arithmetic, branch, memory, call, and return instructions");

    auto branch_program = file;
    branch_program[0x1000 + 12] = std::byte{3};
    const auto taken_branch = executor.run(branch_program);
    require(
        taken_branch.reason == nexus::guest::StopReason::returned && taken_branch.exit_code == 1,
        "conditional branch should take the alternate path when comparison fails"
    );

    nexus::guest::GuestProcess process(17, "local-test-program");
    const auto process_result = process.execute(file, executor);
    require(process_result.state == nexus::guest::ProcessState::exited, "successful process should exit");
    require(process.main_thread().state() == nexus::guest::ThreadState::exited, "main guest thread should exit");
    require(process_result.execution.exit_code == 43, "process should preserve guest exit result");

    const auto second_run = process.execute(file, executor);
    require(
        second_run.error.has_value() &&
            second_run.error->code == ErrorCode::invalid_process_state,
        "process lifecycle should reject a second execution"
    );
}

class EventRecorder final : public nexus::runtime::ApplicationEventSink {
public:
    void on_application_event(const nexus::runtime::ApplicationEvent& event) override {
        events.push_back(event);
    }

    std::vector<nexus::runtime::ApplicationEvent> events;
};

void test_kernel_and_shell_event_boundary() {
    nexus::guest::GuestThread thread(9);
    Error error;
    require(thread.start(error), "new guest thread should start");
    require(thread.finish(true, error) && thread.state() == nexus::guest::ThreadState::exited,
        "running guest thread should finish");
    require(!thread.start(error), "finished guest thread cannot be restarted");

    nexus::guest::KernelMutex mutex;
    require(mutex.try_lock(), "kernel synchronization primitive should acquire when free");
    require(!mutex.try_lock(), "kernel synchronization primitive should exclude a second acquisition");
    mutex.unlock();

    EventRecorder recorder;
    const nexus::runtime::ApplicationEventPublisher publisher(recorder);
    const nexus::runtime::ApplicationMetadata metadata{
        "local.test", "NEXUS original test program", true,
    };
    nexus::guest::GuestProcess process(22, metadata.title);
    const nexus::guest::GuestExecutor executor;
    const auto result = nexus::runtime::ApplicationRuntime{}.execute(
        process,
        make_test_elf(),
        executor,
        metadata,
        publisher
    );
    require(result.state == nexus::guest::ProcessState::exited, "runtime should execute a test process");
    require(recorder.events.size() == 4, "runtime should publish lifecycle and return-to-shell events");
    require(
        recorder.events.front().type == nexus::runtime::EventType::application_started &&
            recorder.events.back().type == nexus::runtime::EventType::return_to_shell &&
            recorder.events.back().application.is_mock_data,
        "shell event boundary should carry explicit local mock metadata and return status"
    );
    recorder.events.clear();
    const auto duplicate_launch = nexus::runtime::ApplicationRuntime{}.execute(
        process,
        make_test_elf(),
        executor,
        metadata,
        publisher
    );
    require(
        duplicate_launch.error.has_value() &&
            recorder.events.back().type == nexus::runtime::EventType::error_notification,
        "runtime must surface invalid process transitions instead of reporting success"
    );

    EventRecorder failure_recorder;
    const nexus::runtime::ApplicationEventPublisher failure_publisher(failure_recorder);
    nexus::guest::GuestProcess failed_process(23, "invalid-input");
    const std::array<std::byte, 4> invalid_file{};
    const auto failed_result = nexus::runtime::ApplicationRuntime{}.execute(
        failed_process,
        invalid_file,
        executor,
        metadata,
        failure_publisher
    );
    require(failed_result.state == nexus::guest::ProcessState::failed,
        "malformed executable should fail the process explicitly");
    require(
        failure_recorder.events.back().type == nexus::runtime::EventType::error_notification,
        "runtime should forward guest errors to the shell event boundary"
    );
}

} // namespace

int main() {
    test_memory_regions_and_permissions();
    test_cpu_instruction_subset_and_faults();
    test_elf_parsing_and_loading();
    test_guest_execution_and_process_lifecycle();
    test_kernel_and_shell_event_boundary();
}
