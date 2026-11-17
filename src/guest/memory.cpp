#include "nexus/guest/memory.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>

namespace nexus::guest {
namespace {

bool range_overflows(std::uint64_t address, std::uint64_t size) {
    return size > std::numeric_limits<std::uint64_t>::max() - address;
}

void set_error(Error& error, ErrorCode code, std::uint64_t address, const char* message) {
    error = Error{code, address, message};
}

} // namespace

bool AddressSpace::map(
    std::uint64_t guest_address,
    std::uint64_t size,
    MemoryPermission permissions,
    Error& error
) {
    if (size == 0 || guest_address % page_size != 0 || size % page_size != 0) {
        set_error(error, ErrorCode::invalid_argument, guest_address, "Mapping must be nonempty and page-aligned");
        return false;
    }
    if (range_overflows(guest_address, size)) {
        set_error(error, ErrorCode::address_overflow, guest_address, "Mapping address range overflows");
        return false;
    }
    if (size > std::numeric_limits<std::size_t>::max()) {
        set_error(error, ErrorCode::resource_exhausted, guest_address, "Mapping is too large for this host");
        return false;
    }

    const auto end = guest_address + size;
    auto next = regions_.lower_bound(guest_address);
    if (next != regions_.end() && next->first < end) {
        set_error(error, ErrorCode::invalid_argument, guest_address, "Mapping overlaps an existing region");
        return false;
    }
    if (next != regions_.begin()) {
        const auto previous = std::prev(next);
        if (previous->first + previous->second.size > guest_address) {
            set_error(error, ErrorCode::invalid_argument, guest_address, "Mapping overlaps an existing region");
            return false;
        }
    }

    try {
        Region region;
        region.size = size;
        region.permissions = permissions;
        region.bytes.resize(static_cast<std::size_t>(size));
        regions_.emplace(guest_address, std::move(region));
    } catch (const std::bad_alloc&) {
        set_error(error, ErrorCode::resource_exhausted, guest_address, "Host allocation for guest mapping failed");
        return false;
    } catch (const std::length_error&) {
        set_error(error, ErrorCode::resource_exhausted, guest_address, "Guest mapping exceeds host container limits");
        return false;
    }
    error = {};
    return true;
}

bool AddressSpace::unmap(std::uint64_t guest_address, Error& error) {
    const auto region = regions_.find(guest_address);
    if (region == regions_.end()) {
        set_error(error, ErrorCode::unmapped_memory, guest_address, "No mapping begins at this guest address");
        return false;
    }
    regions_.erase(region);
    error = {};
    return true;
}

bool AddressSpace::read(
    std::uint64_t guest_address,
    std::span<std::byte> destination,
    Error& error
) const {
    return transfer(guest_address, destination, MemoryPermission::read, error);
}

bool AddressSpace::write(
    std::uint64_t guest_address,
    std::span<const std::byte> source,
    Error& error
) {
    return transfer(guest_address, source, MemoryPermission::write, error);
}

bool AddressSpace::fetch(
    std::uint64_t guest_address,
    std::span<std::byte> destination,
    Error& error
) const {
    return transfer(guest_address, destination, MemoryPermission::execute, error);
}

bool AddressSpace::initialize(
    std::uint64_t guest_address,
    std::span<const std::byte> source,
    Error& error
) {
    return transfer(guest_address, source, MemoryPermission::none, error);
}

bool AddressSpace::mapped(std::uint64_t guest_address, std::uint64_t size) const {
    if (range_overflows(guest_address, size)) {
        return false;
    }
    auto current = guest_address;
    const auto end = guest_address + size;
    while (current < end) {
        auto region = regions_.upper_bound(current);
        if (region == regions_.begin()) {
            return false;
        }
        --region;
        const auto offset = current - region->first;
        if (offset >= region->second.size) {
            return false;
        }
        current += std::min(end - current, region->second.size - offset);
    }
    return true;
}

bool AddressSpace::transfer(
    std::uint64_t guest_address,
    std::span<std::byte> destination,
    MemoryPermission required,
    Error& error
) const {
    if (range_overflows(guest_address, destination.size())) {
        set_error(error, ErrorCode::address_overflow, guest_address, "Guest read address range overflows");
        return false;
    }
    if (destination.empty()) {
        error = {};
        return true;
    }

    auto validated = guest_address;
    std::size_t checked = 0;
    while (checked < destination.size()) {
        auto region = regions_.upper_bound(validated);
        if (region == regions_.begin()) {
            set_error(error, ErrorCode::unmapped_memory, validated, "Guest read from unmapped memory");
            return false;
        }
        --region;
        const auto offset = validated - region->first;
        if (offset >= region->second.size) {
            set_error(error, ErrorCode::unmapped_memory, validated, "Guest read from unmapped memory");
            return false;
        }
        if (!has_permission(region->second.permissions, required)) {
            set_error(error, ErrorCode::permission_denied, validated, "Guest read permission denied");
            return false;
        }
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(
            destination.size() - checked,
            region->second.size - offset
        ));
        checked += count;
        validated += count;
    }

    auto current = guest_address;
    std::size_t copied = 0;
    while (copied < destination.size()) {
        auto region = regions_.upper_bound(current);
        --region;
        const auto offset = current - region->first;
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(
            destination.size() - copied,
            region->second.size - offset
        ));
        std::copy_n(
            region->second.bytes.begin() + static_cast<std::ptrdiff_t>(offset),
            count,
            destination.begin() + static_cast<std::ptrdiff_t>(copied)
        );
        copied += count;
        current += count;
    }
    error = {};
    return true;
}

bool AddressSpace::transfer(
    std::uint64_t guest_address,
    std::span<const std::byte> source,
    MemoryPermission required,
    Error& error
) {
    if (range_overflows(guest_address, source.size())) {
        set_error(error, ErrorCode::address_overflow, guest_address, "Guest write address range overflows");
        return false;
    }
    if (source.empty()) {
        error = {};
        return true;
    }

    auto validated = guest_address;
    std::size_t checked = 0;
    while (checked < source.size()) {
        auto region = regions_.upper_bound(validated);
        if (region == regions_.begin()) {
            set_error(error, ErrorCode::unmapped_memory, validated, "Guest write to unmapped memory");
            return false;
        }
        --region;
        const auto offset = validated - region->first;
        if (offset >= region->second.size) {
            set_error(error, ErrorCode::unmapped_memory, validated, "Guest write to unmapped memory");
            return false;
        }
        if (required != MemoryPermission::none &&
            !has_permission(region->second.permissions, required)) {
            set_error(error, ErrorCode::permission_denied, validated, "Guest write permission denied");
            return false;
        }
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(
            source.size() - checked,
            region->second.size - offset
        ));
        checked += count;
        validated += count;
    }

    auto current = guest_address;
    std::size_t copied = 0;
    while (copied < source.size()) {
        auto region = regions_.upper_bound(current);
        --region;
        const auto offset = current - region->first;
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(
            source.size() - copied,
            region->second.size - offset
        ));
        std::copy_n(
            source.begin() + static_cast<std::ptrdiff_t>(copied),
            count,
            region->second.bytes.begin() + static_cast<std::ptrdiff_t>(offset)
        );
        copied += count;
        current += count;
    }
    error = {};
    return true;
}

} // namespace nexus::guest
