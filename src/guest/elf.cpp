#include "nexus/guest/elf.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <utility>

namespace nexus::guest {
namespace {

constexpr std::uint32_t pt_load = 1;
constexpr std::uint32_t pt_dynamic = 2;
constexpr std::uint32_t pt_interpreter = 3;
constexpr std::uint32_t pf_execute = 1;
constexpr std::uint32_t pf_write = 2;
constexpr std::uint32_t pf_read = 4;
constexpr std::uint32_t sht_rel = 9;
constexpr std::uint32_t sht_rela = 4;
constexpr std::uint32_t sht_strtab = 3;
constexpr std::uint64_t shf_alloc = 2;
constexpr std::uint32_t sht_nobits = 8;

struct RawSection {
    std::uint32_t name_offset{};
    std::uint32_t type{};
    std::uint64_t flags{};
    std::uint64_t address{};
    std::uint64_t offset{};
    std::uint64_t size{};
};

void set_error(Error& error, ErrorCode code, const char* message, std::uint64_t address = 0) {
    error = Error{code, address, message};
}

bool read_le(
    std::span<const std::byte> file,
    std::uint64_t offset,
    unsigned count,
    std::uint64_t& value
) {
    if (offset > file.size() || count > file.size() - static_cast<std::size_t>(offset)) {
        return false;
    }
    value = 0;
    for (unsigned index = 0; index < count; ++index) {
        value |= static_cast<std::uint64_t>(
            std::to_integer<std::uint8_t>(file[static_cast<std::size_t>(offset) + index])
        ) << (index * 8U);
    }
    return true;
}

bool range_within_file(std::uint64_t offset, std::uint64_t size, std::size_t file_size) {
    return offset <= file_size && size <= file_size - static_cast<std::size_t>(offset);
}

bool read_section(
    std::span<const std::byte> file,
    std::uint64_t offset,
    RawSection& section
) {
    std::uint64_t value{};
    if (!read_le(file, offset, 4, value)) return false;
    section.name_offset = static_cast<std::uint32_t>(value);
    if (!read_le(file, offset + 4, 4, value)) return false;
    section.type = static_cast<std::uint32_t>(value);
    if (!read_le(file, offset + 8, 8, section.flags)) return false;
    if (!read_le(file, offset + 16, 8, section.address)) return false;
    if (!read_le(file, offset + 24, 8, section.offset)) return false;
    if (!read_le(file, offset + 32, 8, section.size)) return false;
    return true;
}

} // namespace

bool Elf64Parser::parse(std::span<const std::byte> file, ElfImage& image, Error& error) const {
    constexpr std::array<std::uint8_t, 4> elf_magic{0x7f, 'E', 'L', 'F'};
    if (file.size() < 64) {
        set_error(error, ErrorCode::invalid_elf, "ELF header is truncated");
        return false;
    }
    for (std::size_t index = 0; index < elf_magic.size(); ++index) {
        if (std::to_integer<std::uint8_t>(file[index]) != elf_magic[index]) {
            set_error(error, ErrorCode::invalid_elf, "ELF magic is invalid");
            return false;
        }
    }
    if (std::to_integer<std::uint8_t>(file[4]) != 2 ||
        std::to_integer<std::uint8_t>(file[5]) != 1 ||
        std::to_integer<std::uint8_t>(file[6]) != 1) {
        set_error(error, ErrorCode::unsupported_elf, "Only little-endian ELF64 version 1 is supported");
        return false;
    }

    std::uint64_t type{};
    std::uint64_t machine{};
    std::uint64_t version{};
    std::uint64_t entry{};
    std::uint64_t program_offset{};
    std::uint64_t section_offset{};
    std::uint64_t header_size{};
    std::uint64_t program_entry_size{};
    std::uint64_t program_count{};
    std::uint64_t section_entry_size{};
    std::uint64_t section_count{};
    std::uint64_t names_index{};
    if (!read_le(file, 16, 2, type) || !read_le(file, 18, 2, machine) ||
        !read_le(file, 20, 4, version) || !read_le(file, 24, 8, entry) ||
        !read_le(file, 32, 8, program_offset) || !read_le(file, 40, 8, section_offset) ||
        !read_le(file, 52, 2, header_size) || !read_le(file, 54, 2, program_entry_size) ||
        !read_le(file, 56, 2, program_count) || !read_le(file, 58, 2, section_entry_size) ||
        !read_le(file, 60, 2, section_count) || !read_le(file, 62, 2, names_index)) {
        set_error(error, ErrorCode::invalid_elf, "ELF header fields are truncated");
        return false;
    }
    if (type != 2 || machine != 62 || version != 1) {
        set_error(error, ErrorCode::unsupported_elf, "Only static-layout x86-64 ET_EXEC files are supported");
        return false;
    }
    if (header_size < 64 || program_entry_size < 56 ||
        (program_count != 0 && !range_within_file(
            program_offset,
            program_entry_size * program_count,
            file.size()
        ))) {
        set_error(error, ErrorCode::invalid_elf, "Program header table is malformed or truncated");
        return false;
    }
    if (section_count != 0 &&
        (section_entry_size < 64 ||
            !range_within_file(section_offset, section_entry_size * section_count, file.size()))) {
        set_error(error, ErrorCode::invalid_elf, "Section header table is malformed or truncated");
        return false;
    }
    if (section_count == 0 && names_index != 0) {
        set_error(error, ErrorCode::invalid_elf, "Section name table index exists without sections");
        return false;
    }
    if (program_count == 0xffff || section_count == 0xffff || names_index == 0xffff ||
        (section_count == 0 && section_offset != 0)) {
        set_error(error, ErrorCode::unsupported_elf, "Extended ELF table numbering is not supported");
        return false;
    }
    if (section_count != 0 && names_index >= section_count) {
        set_error(error, ErrorCode::invalid_elf, "Section name table index is out of range");
        return false;
    }
    if (program_count > std::numeric_limits<std::uint64_t>::max() / program_entry_size ||
        (section_count != 0 &&
            section_count > std::numeric_limits<std::uint64_t>::max() / section_entry_size)) {
        set_error(error, ErrorCode::invalid_elf, "ELF table size overflows");
        return false;
    }

    ElfImage parsed;
    parsed.entry_point = entry;
    bool entry_is_executable = false;
    for (std::uint64_t index = 0; index < program_count; ++index) {
        const auto offset = program_offset + index * program_entry_size;
        std::uint64_t segment_type{};
        std::uint64_t flags{};
        std::uint64_t file_offset{};
        std::uint64_t virtual_address{};
        std::uint64_t file_size{};
        std::uint64_t memory_size{};
        std::uint64_t alignment{};
        if (!read_le(file, offset, 4, segment_type) ||
            !read_le(file, offset + 4, 4, flags) ||
            !read_le(file, offset + 8, 8, file_offset) ||
            !read_le(file, offset + 16, 8, virtual_address) ||
            !read_le(file, offset + 32, 8, file_size) ||
            !read_le(file, offset + 40, 8, memory_size) ||
            !read_le(file, offset + 48, 8, alignment)) {
            set_error(error, ErrorCode::invalid_elf, "Program header is truncated");
            return false;
        }
        if (segment_type == pt_interpreter) {
            parsed.has_interpreter = true;
        } else if (segment_type == pt_dynamic) {
            parsed.has_dynamic_segment = true;
        }
        if (segment_type != pt_load) {
            continue;
        }
        if (memory_size < file_size || !range_within_file(file_offset, file_size, file.size()) ||
            memory_size > std::numeric_limits<std::uint64_t>::max() - virtual_address) {
            set_error(error, ErrorCode::invalid_elf, "Loadable segment has invalid bounds", virtual_address);
            return false;
        }
        if (alignment > 1 && ((alignment & (alignment - 1)) != 0 ||
            virtual_address % alignment != file_offset % alignment)) {
            set_error(error, ErrorCode::invalid_elf, "Loadable segment alignment is invalid", virtual_address);
            return false;
        }
        const auto segment_flags = static_cast<std::uint32_t>(flags);
        parsed.segments.push_back(ElfSegment{
            static_cast<std::uint32_t>(segment_type),
            segment_flags,
            file_offset,
            virtual_address,
            file_size,
            memory_size,
            alignment,
        });
        if ((segment_flags & pf_execute) != 0 && entry >= virtual_address &&
            entry < virtual_address + memory_size) {
            entry_is_executable = true;
        }
    }
    if (parsed.segments.empty()) {
        set_error(error, ErrorCode::invalid_elf, "ELF has no loadable segments");
        return false;
    }
    if (!entry_is_executable) {
        set_error(error, ErrorCode::invalid_elf, "Entry point is not inside an executable segment", entry);
        return false;
    }

    std::vector<RawSection> raw_sections;
    raw_sections.reserve(static_cast<std::size_t>(section_count));
    for (std::uint64_t index = 0; index < section_count; ++index) {
        RawSection section;
        if (!read_section(file, section_offset + index * section_entry_size, section)) {
            set_error(error, ErrorCode::invalid_elf, "Section header is truncated");
            return false;
        }
        if (section.type != sht_nobits &&
            !range_within_file(section.offset, section.size, file.size())) {
            set_error(error, ErrorCode::invalid_elf, "Section data is outside the file");
            return false;
        }
        if ((section.type == sht_rel || section.type == sht_rela) &&
            (section.flags & shf_alloc) != 0) {
            parsed.has_allocated_relocations = true;
        }
        raw_sections.push_back(section);
    }

    std::span<const std::byte> section_names;
    if (section_count != 0 && names_index != 0) {
        const auto& names = raw_sections[static_cast<std::size_t>(names_index)];
        if (names.type != sht_strtab) {
            set_error(error, ErrorCode::invalid_elf, "Section name index must refer to a string table");
            return false;
        }
        section_names = file.subspan(
            static_cast<std::size_t>(names.offset),
            static_cast<std::size_t>(names.size)
        );
    } else if (std::any_of(raw_sections.begin(), raw_sections.end(), [](const RawSection& section) {
        return section.name_offset != 0;
    })) {
        set_error(error, ErrorCode::invalid_elf, "Section names require a section name string table");
        return false;
    }
    parsed.sections.reserve(raw_sections.size());
    for (std::size_t index = 0; index < raw_sections.size(); ++index) {
        const auto& raw = raw_sections[index];
        std::string name;
        if (!section_names.empty()) {
            if (raw.name_offset >= section_names.size()) {
                set_error(error, ErrorCode::invalid_elf, "Section name offset is out of range");
                return false;
            }
            const auto begin = section_names.begin() + static_cast<std::ptrdiff_t>(raw.name_offset);
            const auto end = std::find(begin, section_names.end(), std::byte{});
            if (end == section_names.end()) {
                set_error(error, ErrorCode::invalid_elf, "Section name is not null-terminated");
                return false;
            }
            name.reserve(static_cast<std::size_t>(end - begin));
            for (auto current = begin; current != end; ++current) {
                name.push_back(static_cast<char>(std::to_integer<unsigned char>(*current)));
            }
        }
        parsed.sections.push_back(SectionMetadata{
            static_cast<std::uint32_t>(index),
            std::move(name),
            raw.type,
            raw.flags,
            raw.address,
            raw.offset,
            raw.size,
        });
    }

    image = std::move(parsed);
    error = {};
    return true;
}

bool ElfLoader::load(
    std::span<const std::byte> file,
    const ElfImage& image,
    AddressSpace& address_space,
    Error& error
) const {
    if (image.has_interpreter || image.has_dynamic_segment || image.has_allocated_relocations) {
        set_error(
            error,
            ErrorCode::unsupported_elf,
            "Dynamic linking, PT_INTERP, and allocated ELF relocations are not supported"
        );
        return false;
    }

    AddressSpace mapped;
    for (const auto& segment : image.segments) {
        if (segment.type != pt_load || segment.memory_size == 0) {
            continue;
        }
        if (segment.memory_size < segment.file_size ||
            !range_within_file(segment.file_offset, segment.file_size, file.size()) ||
            segment.memory_size > std::numeric_limits<std::uint64_t>::max() - segment.virtual_address) {
            set_error(error, ErrorCode::invalid_elf, "Loadable segment became invalid before mapping", segment.virtual_address);
            return false;
        }
        const auto end = segment.virtual_address + segment.memory_size;
        const auto map_base = segment.virtual_address & ~(AddressSpace::page_size - 1);
        if (end > std::numeric_limits<std::uint64_t>::max() - (AddressSpace::page_size - 1)) {
            set_error(error, ErrorCode::invalid_elf, "Loadable segment page range overflows", segment.virtual_address);
            return false;
        }
        const auto map_end = (end + AddressSpace::page_size - 1) & ~(AddressSpace::page_size - 1);
        MemoryPermission permissions = MemoryPermission::none;
        if ((segment.flags & pf_read) != 0) permissions = permissions | MemoryPermission::read;
        if ((segment.flags & pf_write) != 0) permissions = permissions | MemoryPermission::write;
        if ((segment.flags & pf_execute) != 0) permissions = permissions | MemoryPermission::execute;
        if (!mapped.map(map_base, map_end - map_base, permissions, error)) {
            return false;
        }
        const auto file_bytes = file.subspan(
            static_cast<std::size_t>(segment.file_offset),
            static_cast<std::size_t>(segment.file_size)
        );
        if (!mapped.initialize(segment.virtual_address, file_bytes, error)) {
            return false;
        }
    }
    address_space = std::move(mapped);
    error = {};
    return true;
}

} // namespace nexus::guest
