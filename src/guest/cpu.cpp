#include "nexus/guest/cpu.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <span>

namespace nexus::guest {
namespace {

constexpr std::uint64_t flag_cf = 1ULL << 0;
constexpr std::uint64_t flag_pf = 1ULL << 2;
constexpr std::uint64_t flag_zf = 1ULL << 6;
constexpr std::uint64_t flag_sf = 1ULL << 7;
constexpr std::uint64_t flag_of = 1ULL << 11;
constexpr std::uint64_t arithmetic_flags = flag_cf | flag_pf | flag_zf | flag_sf | flag_of;

struct Reader {
    AddressSpace& memory;
    std::uint64_t start;
    std::uint64_t position{};

    bool read(std::uint8_t& value, Error& error) {
        if (position >= 15) {
            error = Error{ErrorCode::invalid_instruction, start, "x86-64 instruction exceeds 15 bytes"};
            return false;
        }
        std::byte byte{};
        if (position > std::numeric_limits<std::uint64_t>::max() - start ||
            !memory.fetch(start + position, std::span<std::byte>(&byte, 1), error)) {
            return false;
        }
        value = std::to_integer<std::uint8_t>(byte);
        ++position;
        return true;
    }

    bool read_unsigned(unsigned count, std::uint64_t& value, Error& error) {
        value = 0;
        for (unsigned index = 0; index < count; ++index) {
            std::uint8_t byte{};
            if (!read(byte, error)) {
                return false;
            }
            value |= static_cast<std::uint64_t>(byte) << (index * 8U);
        }
        return true;
    }

    bool read_signed(unsigned count, std::int64_t& value, Error& error) {
        std::uint64_t raw{};
        if (!read_unsigned(count, raw, error)) {
            return false;
        }
        if (count < sizeof(raw)) {
            const auto bits = count * 8U;
            const auto sign = std::uint64_t{1} << (bits - 1U);
            if ((raw & sign) != 0) {
                raw |= (~std::uint64_t{0}) << bits;
            }
        }
        value = std::bit_cast<std::int64_t>(raw);
        return true;
    }
};

struct InstructionPointerGuard {
    CpuState& state;
    std::uint64_t instruction_address;
    bool committed{};

    ~InstructionPointerGuard() {
        if (!committed) {
            state.instruction_pointer = instruction_address;
        }
    }
};

struct Operand {
    bool is_register{};
    unsigned register_index{};
    std::uint64_t address{};
    bool rip_relative{};
    std::int64_t rip_displacement{};
};

std::uint64_t width_mask(unsigned width) {
    return width == 8 ? std::numeric_limits<std::uint64_t>::max() : 0xffffffffULL;
}

std::uint64_t read_register(const CpuState& state, unsigned index, unsigned width) {
    return state.registers[index] & width_mask(width);
}

void write_register(CpuState& state, unsigned index, unsigned width, std::uint64_t value) {
    if (width == 8) {
        state.registers[index] = value;
    } else {
        state.registers[index] = static_cast<std::uint32_t>(value);
    }
}

bool read_operand(
    AddressSpace& memory,
    const CpuState& state,
    const Operand& operand,
    unsigned width,
    std::uint64_t& value,
    Error& error
) {
    if (operand.is_register) {
        value = read_register(state, operand.register_index, width);
        return true;
    }
    std::array<std::byte, 8> bytes{};
    if (!memory.read(operand.address, std::span<std::byte>(bytes.data(), width), error)) {
        return false;
    }
    value = 0;
    for (unsigned index = 0; index < width; ++index) {
        value |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(bytes[index])) <<
            (index * 8U);
    }
    return true;
}

bool write_operand(
    AddressSpace& memory,
    CpuState& state,
    const Operand& operand,
    unsigned width,
    std::uint64_t value,
    Error& error
) {
    if (operand.is_register) {
        write_register(state, operand.register_index, width, value);
        return true;
    }
    std::array<std::byte, 8> bytes{};
    for (unsigned index = 0; index < width; ++index) {
        bytes[index] = static_cast<std::byte>((value >> (index * 8U)) & 0xffU);
    }
    return memory.write(operand.address, std::span<const std::byte>(bytes.data(), width), error);
}

bool decode_modrm(
    Reader& reader,
    CpuState& state,
    std::uint8_t rex,
    unsigned& reg,
    Operand& rm,
    Error& error
) {
    std::uint8_t modrm{};
    if (!reader.read(modrm, error)) {
        return false;
    }
    const auto mod = static_cast<unsigned>(modrm >> 6U);
    const auto reg_low = static_cast<unsigned>((modrm >> 3U) & 7U);
    const auto rm_low = static_cast<unsigned>(modrm & 7U);
    reg = reg_low + ((rex & 4U) != 0 ? 8U : 0U);
    rm = {};
    if (mod == 3) {
        rm.is_register = true;
        rm.register_index = rm_low + ((rex & 1U) != 0 ? 8U : 0U);
        return true;
    }

    std::uint64_t base = 0;
    std::uint64_t index_value = 0;
    unsigned scale = 1;
    bool rip_relative = false;
    if (rm_low == 4) {
        std::uint8_t sib{};
        if (!reader.read(sib, error)) {
            return false;
        }
        scale = 1U << (sib >> 6U);
        const auto index_low = static_cast<unsigned>((sib >> 3U) & 7U);
        const auto base_low = static_cast<unsigned>(sib & 7U);
        if (index_low != 4 || (rex & 2U) != 0) {
            const auto index = index_low + ((rex & 2U) != 0 ? 8U : 0U);
            index_value = state.registers[index] * scale;
        }
        if (mod == 0 && base_low == 5) {
            std::int64_t displacement{};
            if (!reader.read_signed(4, displacement, error)) {
                return false;
            }
            base = std::bit_cast<std::uint64_t>(displacement);
        } else {
            const auto base_index = base_low + ((rex & 1U) != 0 ? 8U : 0U);
            base = state.registers[base_index];
        }
    } else if (mod == 0 && rm_low == 5) {
        rip_relative = true;
    } else {
        const auto base_index = rm_low + ((rex & 1U) != 0 ? 8U : 0U);
        base = state.registers[base_index];
    }

    std::int64_t displacement = 0;
    if (mod == 0 && rm_low == 5 && !rip_relative) {
        displacement = 0;
    } else if (mod == 1) {
        if (!reader.read_signed(1, displacement, error)) {
            return false;
        }
    } else if (mod == 2) {
        if (!reader.read_signed(4, displacement, error)) {
            return false;
        }
    } else if (rip_relative && !reader.read_signed(4, displacement, error)) {
        return false;
    }

    const auto next_instruction = reader.start + reader.position;
    const auto address_base = rip_relative ? next_instruction : base;
    rm.address = address_base + index_value + std::bit_cast<std::uint64_t>(displacement);
    rm.rip_relative = rip_relative;
    rm.rip_displacement = displacement;
    return true;
}

void set_logic_flags(CpuState& state, std::uint64_t value, unsigned width) {
    const auto masked = value & width_mask(width);
    state.flags &= ~arithmetic_flags;
    if (masked == 0) {
        state.flags |= flag_zf;
    }
    if ((std::popcount(static_cast<std::uint8_t>(masked)) & 1) == 0) {
        state.flags |= flag_pf;
    }
    if ((masked & (std::uint64_t{1} << (width * 8U - 1U))) != 0) {
        state.flags |= flag_sf;
    }
    state.flags |= 2;
}

std::uint64_t add_values(CpuState& state, std::uint64_t left, std::uint64_t right, unsigned width) {
    const auto mask = width_mask(width);
    const auto sign = std::uint64_t{1} << (width * 8U - 1U);
    left &= mask;
    right &= mask;
    const auto result = (left + right) & mask;
    state.flags &= ~arithmetic_flags;
    if (result < left || (width == 4 && left + right > mask)) {
        state.flags |= flag_cf;
    }
    if (result == 0) {
        state.flags |= flag_zf;
    }
    if ((std::popcount(static_cast<std::uint8_t>(result)) & 1) == 0) {
        state.flags |= flag_pf;
    }
    if ((result & sign) != 0) {
        state.flags |= flag_sf;
    }
    if (((~(left ^ right) & (left ^ result)) & sign) != 0) {
        state.flags |= flag_of;
    }
    state.flags |= 2;
    return result;
}

std::uint64_t subtract_values(
    CpuState& state,
    std::uint64_t left,
    std::uint64_t right,
    unsigned width
) {
    const auto mask = width_mask(width);
    const auto sign = std::uint64_t{1} << (width * 8U - 1U);
    left &= mask;
    right &= mask;
    const auto result = (left - right) & mask;
    state.flags &= ~arithmetic_flags;
    if (left < right) {
        state.flags |= flag_cf;
    }
    if (result == 0) {
        state.flags |= flag_zf;
    }
    if ((std::popcount(static_cast<std::uint8_t>(result)) & 1) == 0) {
        state.flags |= flag_pf;
    }
    if ((result & sign) != 0) {
        state.flags |= flag_sf;
    }
    if ((((left ^ right) & (left ^ result)) & sign) != 0) {
        state.flags |= flag_of;
    }
    state.flags |= 2;
    return result;
}

bool condition_holds(const CpuState& state, unsigned condition) {
    const bool cf = (state.flags & flag_cf) != 0;
    const bool pf = (state.flags & flag_pf) != 0;
    const bool zf = (state.flags & flag_zf) != 0;
    const bool sf = (state.flags & flag_sf) != 0;
    const bool of = (state.flags & flag_of) != 0;
    switch (condition) {
    case 0: return of;
    case 1: return !of;
    case 2: return cf;
    case 3: return !cf;
    case 4: return zf;
    case 5: return !zf;
    case 6: return cf || zf;
    case 7: return !cf && !zf;
    case 8: return sf;
    case 9: return !sf;
    case 10: return pf;
    case 11: return !pf;
    case 12: return sf != of;
    case 13: return sf == of;
    case 14: return zf || (sf != of);
    case 15: return !zf && (sf == of);
    default: return false;
    }
}

std::uint64_t add_displacement(std::uint64_t address, std::int64_t displacement) {
    return address + std::bit_cast<std::uint64_t>(displacement);
}

bool push(AddressSpace& memory, CpuState& state, std::uint64_t value, Error& error) {
    const auto stack_pointer = state.registers[4] - 8;
    std::array<std::byte, 8> bytes{};
    for (unsigned index = 0; index < 8; ++index) {
        bytes[index] = static_cast<std::byte>((value >> (index * 8U)) & 0xffU);
    }
    if (!memory.write(stack_pointer, bytes, error)) {
        return false;
    }
    state.registers[4] = stack_pointer;
    return true;
}

bool pop(AddressSpace& memory, CpuState& state, std::uint64_t& value, Error& error) {
    std::array<std::byte, 8> bytes{};
    if (!memory.read(state.registers[4], bytes, error)) {
        return false;
    }
    value = 0;
    for (unsigned index = 0; index < 8; ++index) {
        value |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(bytes[index])) <<
            (index * 8U);
    }
    state.registers[4] += 8;
    return true;
}

CpuResult fault(std::uint64_t count, Error error) {
    return CpuResult{StopReason::fault, count, 0, std::move(error)};
}

} // namespace

CpuResult X64Interpreter::run(
    AddressSpace& memory,
    CpuState& state,
    std::uint64_t instruction_limit
) const {
    std::uint64_t executed = 0;
    while (executed < instruction_limit) {
        const auto instruction_address = state.instruction_pointer;
        InstructionPointerGuard instruction_guard{state, instruction_address};
        Reader reader{memory, instruction_address};
        Error error;
        std::uint8_t opcode{};
        std::uint8_t rex = 0;
        if (!reader.read(opcode, error)) {
            return fault(executed, std::move(error));
        }
        while (opcode >= 0x40 && opcode <= 0x4f) {
            rex = opcode;
            if (!reader.read(opcode, error)) {
                return fault(executed, std::move(error));
            }
        }
        if (opcode == 0x66) {
            return fault(executed, Error{
                ErrorCode::unsupported_instruction,
                instruction_address,
                "Operand-size override prefix is not supported",
            });
        }

        ++executed;
        const unsigned width = (rex & 8U) != 0 ? 8U : 4U;
        const auto next_ip = [&reader] { return reader.start + reader.position; };
        bool completed = false;

        if (opcode == 0x90) {
            state.instruction_pointer = next_ip();
            completed = true;
        } else if (opcode >= 0x50 && opcode <= 0x57) {
            const auto index = static_cast<unsigned>(opcode - 0x50U) + ((rex & 1U) != 0 ? 8U : 0U);
            if (!push(memory, state, state.registers[index], error)) {
                return fault(executed, std::move(error));
            }
            state.instruction_pointer = next_ip();
            completed = true;
        } else if (opcode >= 0x58 && opcode <= 0x5f) {
            const auto index = static_cast<unsigned>(opcode - 0x58U) + ((rex & 1U) != 0 ? 8U : 0U);
            std::uint64_t value{};
            if (!pop(memory, state, value, error)) {
                return fault(executed, std::move(error));
            }
            state.registers[index] = value;
            state.instruction_pointer = next_ip();
            completed = true;
        } else if (opcode >= 0xb8 && opcode <= 0xbf) {
            const auto index = static_cast<unsigned>(opcode - 0xb8U) + ((rex & 1U) != 0 ? 8U : 0U);
            std::uint64_t value{};
            if (!reader.read_unsigned(width, value, error)) {
                return fault(executed, std::move(error));
            }
            write_register(state, index, width, value);
            state.instruction_pointer = next_ip();
            completed = true;
        } else if (opcode == 0xc3) {
            std::uint64_t target{};
            if (!pop(memory, state, target, error)) {
                return fault(executed, std::move(error));
            }
            state.instruction_pointer = target;
            if (target == return_sentinel) {
                instruction_guard.committed = true;
                return CpuResult{StopReason::returned, executed, state.registers[0], std::nullopt};
            }
            completed = true;
        } else if (opcode >= 0x70 && opcode <= 0x7f) {
            std::int64_t displacement{};
            if (!reader.read_signed(1, displacement, error)) {
                return fault(executed, std::move(error));
            }
            const auto following = next_ip();
            state.instruction_pointer = condition_holds(state, opcode & 0x0fU)
                ? add_displacement(following, displacement)
                : following;
            completed = true;
        } else if (opcode == 0xe8 || opcode == 0xe9 || opcode == 0xeb) {
            std::int64_t displacement{};
            if (!reader.read_signed(opcode == 0xeb ? 1U : 4U, displacement, error)) {
                return fault(executed, std::move(error));
            }
            const auto target = add_displacement(next_ip(), displacement);
            if (opcode == 0xe8 && !push(memory, state, next_ip(), error)) {
                return fault(executed, std::move(error));
            }
            state.instruction_pointer = target;
            completed = true;
        } else if (opcode == 0x68 || opcode == 0x6a) {
            std::int64_t immediate{};
            if (!reader.read_signed(opcode == 0x68 ? 4U : 1U, immediate, error)) {
                return fault(executed, std::move(error));
            }
            if (!push(memory, state, std::bit_cast<std::uint64_t>(immediate), error)) {
                return fault(executed, std::move(error));
            }
            state.instruction_pointer = next_ip();
            completed = true;
        } else if (opcode == 0x0f) {
            std::uint8_t second{};
            if (!reader.read(second, error)) {
                return fault(executed, std::move(error));
            }
            if (second >= 0x80 && second <= 0x8f) {
                std::int64_t displacement{};
                if (!reader.read_signed(4, displacement, error)) {
                    return fault(executed, std::move(error));
                }
                const auto following = next_ip();
                state.instruction_pointer = condition_holds(state, second & 0x0fU)
                    ? add_displacement(following, displacement)
                    : following;
                completed = true;
            }
        } else if (opcode == 0x05 || opcode == 0x2d || opcode == 0x35 || opcode == 0x3d) {
            std::int64_t immediate{};
            if (!reader.read_signed(width == 8 ? 4U : 4U, immediate, error)) {
                return fault(executed, std::move(error));
            }
            const auto left = read_register(state, 0, width);
            const auto right = static_cast<std::uint64_t>(immediate) & width_mask(width);
            std::uint64_t result{};
            if (opcode == 0x05) {
                result = add_values(state, left, right, width);
            } else if (opcode == 0x2d || opcode == 0x3d) {
                result = subtract_values(state, left, right, width);
            } else {
                result = left ^ right;
                set_logic_flags(state, result, width);
            }
            if (opcode != 0x3d) {
                write_register(state, 0, width, result);
            }
            state.instruction_pointer = next_ip();
            completed = true;
        } else if (opcode == 0x89 || opcode == 0x8b || opcode == 0x8d ||
            opcode == 0x01 || opcode == 0x03 || opcode == 0x29 || opcode == 0x2b ||
            opcode == 0x31 || opcode == 0x33 || opcode == 0x39 || opcode == 0x3b ||
            opcode == 0x85 || opcode == 0xc7 || opcode == 0x81 || opcode == 0x83 ||
            opcode == 0xff) {
            unsigned reg{};
            Operand rm;
            if (!decode_modrm(reader, state, rex, reg, rm, error)) {
                return fault(executed, std::move(error));
            }
            state.instruction_pointer = next_ip();
            if (opcode == 0x8d) {
                if (rm.is_register) {
                    return fault(executed, Error{
                        ErrorCode::invalid_instruction, instruction_address, "LEA requires a memory operand",
                    });
                }
                write_register(state, reg, width, rm.address);
                completed = true;
            } else if (opcode == 0x89) {
                completed = write_operand(memory, state, rm, width, state.registers[reg], error);
            } else if (opcode == 0x8b) {
                std::uint64_t value{};
                completed = read_operand(memory, state, rm, width, value, error);
                if (completed) {
                    write_register(state, reg, width, value);
                }
            } else if (opcode == 0xc7) {
                if (reg != 0) {
                    return fault(executed, Error{
                        ErrorCode::unsupported_instruction, instruction_address, "C7 group operation is not supported",
                    });
                }
                std::int64_t immediate{};
                if (!reader.read_signed(4, immediate, error)) {
                    return fault(executed, std::move(error));
                }
                state.instruction_pointer = next_ip();
                if (rm.rip_relative) {
                    rm.address = next_ip() + std::bit_cast<std::uint64_t>(rm.rip_displacement);
                }
                completed = write_operand(
                    memory, state, rm, width,
                    static_cast<std::uint64_t>(immediate) & width_mask(width), error
                );
            } else if (opcode == 0xff) {
                if (reg == 2 || reg == 4 || reg == 6) {
                    std::uint64_t target{};
                    if (!read_operand(memory, state, rm, 8, target, error)) {
                        return fault(executed, std::move(error));
                    }
                    if (reg == 2 && !push(memory, state, state.instruction_pointer, error)) {
                        return fault(executed, std::move(error));
                    }
                    state.instruction_pointer = target;
                    completed = true;
                } else if (reg == 0 || reg == 1) {
                    std::uint64_t value{};
                    if (!read_operand(memory, state, rm, width, value, error)) {
                        return fault(executed, std::move(error));
                    }
                    const auto old_cf = state.flags & flag_cf;
                    const auto result = reg == 0
                        ? add_values(state, value, 1, width)
                        : subtract_values(state, value, 1, width);
                    state.flags = (state.flags & ~flag_cf) | old_cf;
                    completed = write_operand(memory, state, rm, width, result, error);
                }
            } else if (opcode == 0x81 || opcode == 0x83) {
                std::int64_t immediate{};
                if (!reader.read_signed(opcode == 0x81 ? 4U : 1U, immediate, error)) {
                    return fault(executed, std::move(error));
                }
                state.instruction_pointer = next_ip();
                if (rm.rip_relative) {
                    rm.address = next_ip() + std::bit_cast<std::uint64_t>(rm.rip_displacement);
                }
                std::uint64_t value{};
                if (!read_operand(memory, state, rm, width, value, error)) {
                    return fault(executed, std::move(error));
                }
                const auto operand = static_cast<std::uint64_t>(immediate) & width_mask(width);
                std::uint64_t result{};
                bool write = true;
                switch (reg) {
                case 0: result = add_values(state, value, operand, width); break;
                case 1: result = value | operand; set_logic_flags(state, result, width); break;
                case 4: result = value & operand; set_logic_flags(state, result, width); break;
                case 5: result = subtract_values(state, value, operand, width); break;
                case 6: result = value ^ operand; set_logic_flags(state, result, width); break;
                case 7: result = subtract_values(state, value, operand, width); write = false; break;
                default:
                    return fault(executed, Error{
                        ErrorCode::unsupported_instruction, instruction_address, "Immediate arithmetic group operation is not supported",
                    });
                }
                completed = !write || write_operand(memory, state, rm, width, result, error);
            } else {
                const bool reverse = opcode == 0x03 || opcode == 0x2b || opcode == 0x33 || opcode == 0x3b;
                const Operand& destination = reverse ? Operand{true, reg, 0} : rm;
                const Operand& source = reverse ? rm : Operand{true, reg, 0};
                std::uint64_t left{};
                std::uint64_t right{};
                if (!read_operand(memory, state, destination, width, left, error) ||
                    !read_operand(memory, state, source, width, right, error)) {
                    return fault(executed, std::move(error));
                }
                std::uint64_t result{};
                bool write = true;
                if (opcode == 0x01 || opcode == 0x03) {
                    result = add_values(state, left, right, width);
                } else if (opcode == 0x29 || opcode == 0x2b || opcode == 0x39 || opcode == 0x3b) {
                    result = subtract_values(state, left, right, width);
                    write = opcode != 0x39 && opcode != 0x3b;
                } else if (opcode == 0x31 || opcode == 0x33) {
                    result = left ^ right;
                    set_logic_flags(state, result, width);
                } else {
                    result = left & right;
                    set_logic_flags(state, result, width);
                    write = false;
                }
                completed = !write || write_operand(memory, state, destination, width, result, error);
            }
        }

        if (!completed) {
            if (error.message.empty()) {
                error = Error{
                    ErrorCode::unsupported_instruction,
                    instruction_address,
                    "Instruction encoding is outside the implemented x86-64 subset",
                };
            }
            return fault(executed, std::move(error));
        }
        instruction_guard.committed = true;
    }
    return CpuResult{
        StopReason::instruction_limit,
        executed,
        0,
        Error{ErrorCode::instruction_limit, state.instruction_pointer, "Instruction limit reached"},
    };
}

} // namespace nexus::guest
