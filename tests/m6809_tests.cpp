#include "cpu/m6809.h"
#include "fm7_machine.h"

#include <cassert>
#include <cstdint>

int main() {
    {
        fm7::MemoryBus bus;
        fm7::M6809 cpu(bus);
        auto& mem = bus.rawMemory();
        mem[0x0100] = 0x86; // LDA #$7f
        mem[0x0101] = 0x7f;
        mem[0x0102] = 0xB7; // STA $2000
        mem[0x0103] = 0x20;
        mem[0x0104] = 0x00;
        cpu.reset(0x0100);

        assert(cpu.step());
        assert(cpu.registers().a == 0x7f);
        assert(cpu.step());
        assert(bus.read8(0x2000) == 0x7f);
        assert(cpu.cycles() == 7);
    }

    {
        fm7::MemoryBus bus;
        fm7::M6809 cpu(bus);
        auto& mem = bus.rawMemory();
        mem[0x0200] = 0x20; // BRA +2
        mem[0x0201] = 0x02;
        mem[0x0204] = 0xC6; // LDB #$33
        mem[0x0205] = 0x33;
        cpu.reset(0x0200);

        assert(cpu.step());
        assert(cpu.registers().pc == 0x0204);
        assert(cpu.step());
        assert(cpu.registers().b == 0x33);
    }

    {
        fm7::MemoryBus bus;
        fm7::M6809 cpu(bus);
        auto& mem = bus.rawMemory();
        mem[0x0300] = 0x8D; // BSR +3; return address = $0302
        mem[0x0301] = 0x03;
        mem[0x0302] = 0xC6;
        mem[0x0303] = 0x66;
        mem[0x0305] = 0x86;
        mem[0x0306] = 0x5A;
        mem[0x0307] = 0x39;
        cpu.reset(0x0300);
        cpu.registers().s = 0x4000;

        assert(cpu.step());
        assert(cpu.registers().pc == 0x0305);
        assert(cpu.registers().s == 0x3ffe);
        assert(cpu.step());
        assert(cpu.registers().a == 0x5a);
        assert(cpu.step());
        assert(cpu.registers().pc == 0x0302);
        assert(cpu.registers().s == 0x4000);
        assert(cpu.step());
        assert(cpu.registers().b == 0x66);
    }

    {
        fm7::MemoryBus bus;
        fm7::M6809 cpu(bus);
        auto& mem = bus.rawMemory();
        mem[0x0400] = 0x17;
        mem[0x0401] = 0x00;
        mem[0x0402] = 0x03;
        mem[0x0403] = 0x12;
        mem[0x0406] = 0x4F;
        mem[0x0407] = 0x39;
        cpu.reset(0x0400);
        cpu.registers().s = 0x5000;

        assert(cpu.step());
        assert(cpu.registers().pc == 0x0406);
        assert(cpu.step());
        assert(cpu.registers().a == 0);
        assert(cpu.step());
        assert(cpu.registers().pc == 0x0403);
        assert(cpu.registers().s == 0x5000);
        assert(cpu.step());
    }

    {
        fm7::MemoryBus bus;
        fm7::M6809 cpu(bus);
        auto& mem = bus.rawMemory();
        mem[0x0500] = 0xBD;
        mem[0x0501] = 0x05;
        mem[0x0502] = 0x10;
        mem[0x0503] = 0xC6;
        mem[0x0504] = 0x77;
        mem[0x0510] = 0x86;
        mem[0x0511] = 0x22;
        mem[0x0512] = 0x39;
        cpu.reset(0x0500);
        cpu.registers().s = 0x6000;

        assert(cpu.step());
        assert(cpu.registers().pc == 0x0510);
        assert(cpu.registers().s == 0x5ffe);
        assert(cpu.step());
        assert(cpu.registers().a == 0x22);
        assert(cpu.step());
        assert(cpu.registers().pc == 0x0503);
        assert(cpu.registers().s == 0x6000);
        assert(cpu.step());
        assert(cpu.registers().b == 0x77);
    }

    {
        fm7::MemoryBus bus;
        fm7::M6809 cpu(bus);
        bus.write8(0, 0x00);
        cpu.reset(0);
        assert(!cpu.step());
        assert(cpu.halted());
    }

    {
        fm7::FM7Machine machine;
        machine.resetMemory();
        // LDA #$5A; STA $FC80 (main shared RAM); NOP
        auto& program = machine.mainBus().rawMemory();
        program[0x0100] = 0x86;
        program[0x0101] = 0x5A;
        program[0x0102] = 0xB7;
        program[0x0103] = 0xFC;
        program[0x0104] = 0x80;
        program[0x0105] = 0x12;

        fm7::M6809 cpu(machine);
        cpu.reset(0x0100);
        assert(cpu.step());
        assert(cpu.step());
        assert(machine.subRead8(fm7::FM7Machine::kSubSharedRamStart) == 0x5A);
        assert(cpu.step());
    }

    {
        // CLRA and CLRB clear N, Z, V and C appropriately while preserving
        // unrelated condition-code bits.
        fm7::MemoryBus bus;
        fm7::M6809 cpu(bus);
        auto& mem = bus.rawMemory();
        mem[0x0600] = 0x4F; // CLRA
        mem[0x0601] = 0x5F; // CLRB
        cpu.reset(0x0600);
        cpu.registers().a = 0x7F;
        cpu.registers().b = 0x7F;
        cpu.registers().cc = static_cast<std::uint8_t>(
            fm7::M6809::Carry | fm7::M6809::Overflow |
            fm7::M6809::Negative | fm7::M6809::InterruptMask);
        assert(cpu.step());
        assert(cpu.registers().a == 0);
        assert((cpu.registers().cc & fm7::M6809::Zero) != 0);
        assert((cpu.registers().cc & (fm7::M6809::Carry | fm7::M6809::Overflow |
                                      fm7::M6809::Negative)) == 0);
        assert((cpu.registers().cc & fm7::M6809::InterruptMask) != 0);
        cpu.registers().cc |= fm7::M6809::Carry | fm7::M6809::Overflow |
                              fm7::M6809::Negative;
        assert(cpu.step());
        assert(cpu.registers().b == 0);
        assert((cpu.registers().cc & fm7::M6809::Zero) != 0);
        assert((cpu.registers().cc & (fm7::M6809::Carry | fm7::M6809::Overflow |
                                      fm7::M6809::Negative)) == 0);
        assert((cpu.registers().cc & fm7::M6809::InterruptMask) != 0);
    }

    {
        // Test immediate arithmetic boundary cases and preservation of I/F.
        fm7::MemoryBus bus;
        fm7::M6809 cpu(bus);
        auto& mem = bus.rawMemory();
        mem[0x0700] = 0x8B; // ADDA #$01
        mem[0x0701] = 0x01;
        mem[0x0702] = 0x80; // SUBA #$01
        mem[0x0703] = 0x01;
        cpu.reset(0x0700);
        cpu.registers().a = 0x7F;
        cpu.registers().cc = static_cast<std::uint8_t>(
            fm7::M6809::InterruptMask | fm7::M6809::FirqMask);
        assert(cpu.step());
        assert(cpu.registers().a == 0x80);
        assert((cpu.registers().cc & fm7::M6809::Negative) != 0);
        assert((cpu.registers().cc & fm7::M6809::Overflow) != 0);
        assert((cpu.registers().cc & fm7::M6809::Carry) == 0);
        assert((cpu.registers().cc & (fm7::M6809::InterruptMask |
                                      fm7::M6809::FirqMask)) ==
               (fm7::M6809::InterruptMask | fm7::M6809::FirqMask));
        assert(cpu.step());
        assert(cpu.registers().a == 0x7F);
        assert((cpu.registers().cc & fm7::M6809::Negative) == 0);
        assert((cpu.registers().cc & fm7::M6809::Overflow) != 0);
        assert((cpu.registers().cc & (fm7::M6809::InterruptMask |
                                      fm7::M6809::FirqMask)) ==
               (fm7::M6809::InterruptMask | fm7::M6809::FirqMask));
    }

    {
        // Immediate load updates N/Z and clears V without altering C/I/F.
        fm7::MemoryBus bus;
        fm7::M6809 cpu(bus);
        auto& mem = bus.rawMemory();
        mem[0x0800] = 0x86; // LDA #$00
        mem[0x0801] = 0x00;
        cpu.reset(0x0800);
        cpu.registers().cc = static_cast<std::uint8_t>(
            fm7::M6809::Carry | fm7::M6809::Overflow |
            fm7::M6809::InterruptMask | fm7::M6809::FirqMask);
        assert(cpu.step());
        assert(cpu.registers().a == 0);
        assert((cpu.registers().cc & fm7::M6809::Zero) != 0);
        assert((cpu.registers().cc & (fm7::M6809::Negative |
                                      fm7::M6809::Overflow)) == 0);
        assert((cpu.registers().cc & (fm7::M6809::Carry |
                                      fm7::M6809::InterruptMask |
                                      fm7::M6809::FirqMask)) ==
               (fm7::M6809::Carry | fm7::M6809::InterruptMask |
                fm7::M6809::FirqMask));
    }

    return 0;
}
