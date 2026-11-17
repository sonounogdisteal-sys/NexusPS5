#pragma once

#include "nexus/guest/error.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <vector>

namespace nexus::guest {

enum class MemoryPermission : std::uint8_t {
    none = 0,
    read = 1,
    write = 2,
    execute = 4,
};

constexpr MemoryPermission operator|(MemoryPermission left, MemoryPermission right) {
    return static_cast<MemoryPermission>(
        static_cast<std::uint8_t>(left) | static_cast<std::uint8_t>(right)
    );
}

constexpr bool has_permission(MemoryPermission value, MemoryPermission permission) {
    return (static_cast<std::uint8_t>(value) & static_cast<std::uint8_t>(permission)) != 0;
}

class AddressSpace {
public:
    static constexpr std::uint64_t page_size = 4096;

    bool map(
        std::uint64_t guest_address,
        std::uint64_t size,
        MemoryPermission permissions,
        Error& error
    );
    bool unmap(std::uint64_t guest_address, Error& error);
    bool read(std::uint64_t guest_address, std::span<std::byte> destination, Error& error) const;
    bool write(std::uint64_t guest_address, std::span<const std::byte> source, Error& error);
    bool fetch(std::uint64_t guest_address, std::span<std::byte> destination, Error& error) const;
    bool initialize(std::uint64_t guest_address, std::span<const std::byte> source, Error& error);
    bool mapped(std::uint64_t guest_address, std::uint64_t size) const;

private:
    struct Region {
        std::uint64_t size{};
        MemoryPermission permissions{MemoryPermission::none};
        std::vector<std::byte> bytes;
    };

    bool transfer(
        std::uint64_t guest_address,
        std::span<std::byte> destination,
        MemoryPermission required,
        Error& error
    ) const;
    bool transfer(
        std::uint64_t guest_address,
        std::span<const std::byte> source,
        MemoryPermission required,
        Error& error
    );

    std::map<std::uint64_t, Region> regions_;
};

} // namespace nexus::guest
