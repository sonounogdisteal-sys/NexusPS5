#pragma once

#include "nexus/guest/error.hpp"
#include "nexus/guest/memory.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace nexus::guest {

struct ElfSegment {
    std::uint32_t type{};
    std::uint32_t flags{};
    std::uint64_t file_offset{};
    std::uint64_t virtual_address{};
    std::uint64_t file_size{};
    std::uint64_t memory_size{};
    std::uint64_t alignment{};
};

struct SectionMetadata {
    std::uint32_t index{};
    std::string name;
    std::uint32_t type{};
    std::uint64_t flags{};
    std::uint64_t virtual_address{};
    std::uint64_t file_offset{};
    std::uint64_t size{};
};

struct ElfImage {
    std::uint64_t entry_point{};
    std::vector<ElfSegment> segments;
    std::vector<SectionMetadata> sections;
    bool has_interpreter{};
    bool has_dynamic_segment{};
    bool has_allocated_relocations{};
};

class Elf64Parser {
public:
    bool parse(std::span<const std::byte> file, ElfImage& image, Error& error) const;
};

class ElfLoader {
public:
    bool load(
        std::span<const std::byte> file,
        const ElfImage& image,
        AddressSpace& address_space,
        Error& error
    ) const;
};

} // namespace nexus::guest
