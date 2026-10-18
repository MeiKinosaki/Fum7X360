#include "cpu/m6809.h"

namespace fm7 {

M6809::M6809() = default;
M6809::M6809(MemoryInterface& memory) : memory_(&memory) {}

void M6809::reset(std::uint16_t startAddress) {
    regs_ = Registers{};
    regs_.pc = startAddress;
    cycles_ = 0;
    halted_ = false;
}

std::uint8_t M6809::fetch8() {
    const auto value = memory_->read8(regs_.pc);
    regs_.pc = static_cast<std::uint16_t>(regs_.pc + 1);
    return value;
}

std::uint16_t M6809::fetch16() {
    const auto high = fetch8();
    const auto low = fetch8();
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(high) << 8) | low);
}

std::uint16_t M6809::read16(std::uint16_t address) const {
    return memory_->read16(address);
}

void M6809::write16(std::uint16_t address, std::uint16_t value) {
    memory_->write16(address, value);
}

void M6809::push8(std::uint16_t& stack, std::uint8_t value) {
    stack = static_cast<std::uint16_t>(stack - 1);
    memory_->write8(stack, value);
}

std::uint8_t M6809::pull8(std::uint16_t& stack) {
    const auto value = memory_->read8(stack);
    stack = static_cast<std::uint16_t>(stack + 1);
    return value;
}

void M6809::push16(std::uint16_t& stack, std::uint16_t value) {
    push8(stack, static_cast<std::uint8_t>(value));
    push8(stack, static_cast<std::uint8_t>(value >> 8));
}

std::uint16_t M6809::pull16(std::uint16_t& stack) {
    const auto high = pull8(stack);
    const auto low = pull8(stack);
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(high) << 8) | low);
}

void M6809::setNZ8(std::uint8_t value) {
    regs_.cc = static_cast<std::uint8_t>(regs_.cc & ~(Negative | Zero));
    if (value == 0) regs_.cc = static_cast<std::uint8_t>(regs_.cc | Zero);
    if ((value & 0x80) != 0) regs_.cc = static_cast<std::uint8_t>(regs_.cc | Negative);
}

void M6809::setNZ16(std::uint16_t value) {
    regs_.cc = static_cast<std::uint8_t>(regs_.cc & ~(Negative | Zero));
    if (value == 0) regs_.cc = static_cast<std::uint8_t>(regs_.cc | Zero);
    if ((value & 0x8000) != 0) regs_.cc = static_cast<std::uint8_t>(regs_.cc | Negative);
}

void M6809::setAddFlags8(std::uint8_t lhs, std::uint8_t rhs, std::uint16_t result) {
    const auto value = static_cast<std::uint8_t>(result);
    regs_.cc = static_cast<std::uint8_t>(regs_.cc & ~(HalfCarry | Negative | Zero | Overflow | Carry));
    if (((lhs & 0x0f) + (rhs & 0x0f)) > 0x0f) regs_.cc |= HalfCarry;
    if (value == 0) regs_.cc |= Zero;
    if (value & 0x80) regs_.cc |= Negative;
    if ((~(lhs ^ rhs) & (lhs ^ value) & 0x80) != 0) regs_.cc |= Overflow;
    if (result & 0x100) regs_.cc |= Carry;
}

void M6809::setSubFlags8(std::uint8_t lhs, std::uint8_t rhs, std::uint16_t result) {
    const auto value = static_cast<std::uint8_t>(result);
    regs_.cc = static_cast<std::uint8_t>(regs_.cc & ~(Negative | Zero | Overflow | Carry));
    if (value == 0) regs_.cc |= Zero;
    if (value & 0x80) regs_.cc |= Negative;
    if (((lhs ^ rhs) & (lhs ^ value) & 0x80) != 0) regs_.cc |= Overflow;
    if (lhs < rhs) regs_.cc |= Carry;
}

bool M6809::branchCondition(std::uint8_t opcode) const {
    const bool c = (regs_.cc & Carry) != 0;
    const bool z = (regs_.cc & Zero) != 0;
    const bool v = (regs_.cc & Overflow) != 0;
    const bool n = (regs_.cc & Negative) != 0;
    switch (opcode) {
    case 0x21: return false;
    case 0x22: return !c && !z;
    case 0x23: return c || z;
    case 0x24: return !c;
    case 0x25: return c;
    case 0x26: return !z;
    case 0x27: return z;
    case 0x28: return !v;
    case 0x29: return v;
    case 0x2A: return !n;
    case 0x2B: return n;
    case 0x2C: return n == v;
    case 0x2D: return n != v;
    case 0x2E: return !z && n == v;
    case 0x2F: return z || n != v;
    default: return false;
    }
}

bool M6809::step() {
    if (halted_) return false;
    const auto opcode = fetch8();
    switch (opcode) {
    case 0x12: cycles_ += 2; return true;
    case 0x16: {
        const auto displacement = static_cast<std::int16_t>(fetch16());
        regs_.pc = static_cast<std::uint16_t>(regs_.pc + displacement);
        cycles_ += 5; return true;
    }
    case 0x17: {
        const auto displacement = static_cast<std::int16_t>(fetch16());
        push16(regs_.s, regs_.pc);
        regs_.pc = static_cast<std::uint16_t>(regs_.pc + displacement);
        cycles_ += 9; return true;
    }
    case 0x20: {
        const auto displacement = static_cast<std::int8_t>(fetch8());
        regs_.pc = static_cast<std::uint16_t>(regs_.pc + displacement);
        cycles_ += 3; return true;
    }
    case 0x21: case 0x22: case 0x23: case 0x24: case 0x25:
    case 0x26: case 0x27: case 0x28: case 0x29: case 0x2A:
    case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F: {
        const auto displacement = static_cast<std::int8_t>(fetch8());
        if (branchCondition(opcode)) regs_.pc = static_cast<std::uint16_t>(regs_.pc + displacement);
        cycles_ += 3; return true;
    }
    case 0x34: {
        const auto mask = fetch8();
        if (mask & 0x80) push16(regs_.s, regs_.pc);
        if (mask & 0x40) push16(regs_.s, regs_.u);
        if (mask & 0x20) push16(regs_.s, regs_.y);
        if (mask & 0x10) push16(regs_.s, regs_.x);
        if (mask & 0x08) push8(regs_.s, regs_.dp);
        if (mask & 0x04) push8(regs_.s, regs_.b);
        if (mask & 0x02) push8(regs_.s, regs_.a);
        if (mask & 0x01) push8(regs_.s, regs_.cc);
        cycles_ += 5; return true;
    }
    case 0x35: {
        const auto mask = fetch8();
        if (mask & 0x01) regs_.cc = pull8(regs_.s);
        if (mask & 0x02) regs_.a = pull8(regs_.s);
        if (mask & 0x04) regs_.b = pull8(regs_.s);
        if (mask & 0x08) regs_.dp = pull8(regs_.s);
        if (mask & 0x10) regs_.x = pull16(regs_.s);
        if (mask & 0x20) regs_.y = pull16(regs_.s);
        if (mask & 0x40) regs_.u = pull16(regs_.s);
        if (mask & 0x80) regs_.pc = pull16(regs_.s);
        cycles_ += 5; return true;
    }
    case 0x39: regs_.pc = pull16(regs_.s); cycles_ += 5; return true;
    case 0x3F: halted_ = true; return false;
    case 0x40: {
        const auto lhs = regs_.a;
        const auto result = static_cast<std::uint16_t>(0u - lhs);
        regs_.a = static_cast<std::uint8_t>(result);
        setSubFlags8(0, lhs, result);
        cycles_ += 2; return true;
    }
    case 0x43:
        regs_.a = static_cast<std::uint8_t>(~regs_.a);
        setNZ8(regs_.a);
        regs_.cc = static_cast<std::uint8_t>((regs_.cc & ~Overflow) | Carry);
        cycles_ += 2; return true;
    case 0x4A: {
        const auto old = regs_.a;
        regs_.a = static_cast<std::uint8_t>(old - 1);
        setNZ8(regs_.a);
        regs_.cc = static_cast<std::uint8_t>(regs_.cc & ~Overflow);
        if (old == 0x80) regs_.cc |= Overflow;
        cycles_ += 2; return true;
    }
    case 0x4C: {
        const auto old = regs_.a;
        regs_.a = static_cast<std::uint8_t>(old + 1);
        setNZ8(regs_.a);
        regs_.cc = static_cast<std::uint8_t>(regs_.cc & ~Overflow);
        if (old == 0x7F) regs_.cc |= Overflow;
        cycles_ += 2; return true;
    }
    case 0x4D:
        setNZ8(regs_.a);
        regs_.cc = static_cast<std::uint8_t>(regs_.cc & ~Overflow);
        cycles_ += 2; return true;
    case 0x50: {
        const auto lhs = regs_.b;
        const auto result = static_cast<std::uint16_t>(0u - lhs);
        regs_.b = static_cast<std::uint8_t>(result);
        setSubFlags8(0, lhs, result);
        cycles_ += 2; return true;
    }
    case 0x53:
        regs_.b = static_cast<std::uint8_t>(~regs_.b);
        setNZ8(regs_.b);
        regs_.cc = static_cast<std::uint8_t>((regs_.cc & ~Overflow) | Carry);
        cycles_ += 2; return true;
    case 0x5A: {
        const auto old = regs_.b;
        regs_.b = static_cast<std::uint8_t>(old - 1);
        setNZ8(regs_.b);
        regs_.cc = static_cast<std::uint8_t>(regs_.cc & ~Overflow);
        if (old == 0x80) regs_.cc |= Overflow;
        cycles_ += 2; return true;
    }
    case 0x5C: {
        const auto old = regs_.b;
        regs_.b = static_cast<std::uint8_t>(old + 1);
        setNZ8(regs_.b);
        regs_.cc = static_cast<std::uint8_t>(regs_.cc & ~Overflow);
        if (old == 0x7F) regs_.cc |= Overflow;
        cycles_ += 2; return true;
    }
    case 0x5D:
        setNZ8(regs_.b);
        regs_.cc = static_cast<std::uint8_t>(regs_.cc & ~Overflow);
        cycles_ += 2; return true;
    case 0x4F:
        regs_.a = 0;
        regs_.cc = static_cast<std::uint8_t>((regs_.cc & ~(Negative | Overflow | Zero | Carry)) | Zero);
        cycles_ += 2; return true;
    case 0x5F:
        regs_.b = 0;
        regs_.cc = static_cast<std::uint8_t>((regs_.cc & ~(Negative | Overflow | Zero)) | Zero);
        cycles_ += 2; return true;
    case 0x7E: regs_.pc = fetch16(); cycles_ += 3; return true;
    case 0x8D: {
        const auto displacement = static_cast<std::int8_t>(fetch8());
        push16(regs_.s, regs_.pc);
        regs_.pc = static_cast<std::uint16_t>(regs_.pc + displacement);
        cycles_ += 7; return true;
    }
    case 0x80: { const auto rhs = fetch8(); const auto lhs = regs_.a; const auto result = static_cast<std::uint16_t>(lhs - rhs); regs_.a = static_cast<std::uint8_t>(result); setSubFlags8(lhs, rhs, result); cycles_ += 2; return true; }
    case 0x81: { const auto rhs = fetch8(); setSubFlags8(regs_.a, rhs, static_cast<std::uint16_t>(regs_.a - rhs)); cycles_ += 2; return true; }
    case 0x84: regs_.a &= fetch8(); setNZ8(regs_.a); regs_.cc &= static_cast<std::uint8_t>(~Overflow); cycles_ += 2; return true;
    case 0x86: regs_.a = fetch8(); setNZ8(regs_.a); regs_.cc &= static_cast<std::uint8_t>(~Overflow); cycles_ += 2; return true;
    case 0x88: regs_.a ^= fetch8(); setNZ8(regs_.a); regs_.cc &= static_cast<std::uint8_t>(~Overflow); cycles_ += 2; return true;
    case 0x8A: regs_.a |= fetch8(); setNZ8(regs_.a); regs_.cc &= static_cast<std::uint8_t>(~Overflow); cycles_ += 2; return true;
    case 0x8B: { const auto rhs = fetch8(); const auto lhs = regs_.a; const auto result = static_cast<std::uint16_t>(lhs + rhs); regs_.a = static_cast<std::uint8_t>(result); setAddFlags8(lhs, rhs, result); cycles_ += 2; return true; }
    case 0x8E: regs_.x = fetch16(); setNZ16(regs_.x); regs_.cc &= static_cast<std::uint8_t>(~Overflow); cycles_ += 3; return true;
    case 0xBD: { const auto address = fetch16(); push16(regs_.s, regs_.pc); regs_.pc = address; cycles_ += 8; return true; }
    case 0xB6: { const auto address = fetch16(); regs_.a = memory_->read8(address); setNZ8(regs_.a); regs_.cc &= static_cast<std::uint8_t>(~Overflow); cycles_ += 5; return true; }
    case 0xB7: { const auto address = fetch16(); memory_->write8(address, regs_.a); setNZ8(regs_.a); regs_.cc &= static_cast<std::uint8_t>(~Overflow); cycles_ += 5; return true; }
    case 0xC0: { const auto rhs = fetch8(); const auto lhs = regs_.b; const auto result = static_cast<std::uint16_t>(lhs - rhs); regs_.b = static_cast<std::uint8_t>(result); setSubFlags8(lhs, rhs, result); cycles_ += 2; return true; }
    case 0xC1: { const auto rhs = fetch8(); setSubFlags8(regs_.b, rhs, static_cast<std::uint16_t>(regs_.b - rhs)); cycles_ += 2; return true; }
    case 0xC6: regs_.b = fetch8(); setNZ8(regs_.b); regs_.cc &= static_cast<std::uint8_t>(~Overflow); cycles_ += 2; return true;
    case 0xCB: { const auto rhs = fetch8(); const auto lhs = regs_.b; const auto result = static_cast<std::uint16_t>(lhs + rhs); regs_.b = static_cast<std::uint8_t>(result); setAddFlags8(lhs, rhs, result); cycles_ += 2; return true; }
    case 0xCE: regs_.u = fetch16(); setNZ16(regs_.u); regs_.cc &= static_cast<std::uint8_t>(~Overflow); cycles_ += 3; return true;
    case 0xF6: { const auto address = fetch16(); regs_.b = memory_->read8(address); setNZ8(regs_.b); regs_.cc &= static_cast<std::uint8_t>(~Overflow); cycles_ += 5; return true; }
    case 0xF7: { const auto address = fetch16(); memory_->write8(address, regs_.b); setNZ8(regs_.b); regs_.cc &= static_cast<std::uint8_t>(~Overflow); cycles_ += 5; return true; }
    default: halted_ = true; return false;
    }
}

} // namespace fm7
