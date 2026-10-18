# Fum7X360

Experimental Fujitsu FM-7 emulator project targeting Xbox 360.

> **Status:** early emulator core. Check the latest GitHub Actions run for validation of the current commit. This is not yet a complete FM-7 emulator and does not produce a working Xbox 360 `default.xex`.

## Current implementation

- Portable C++17 core built with CMake.
- Initial Motorola 6809 instruction subset and basic cycle accounting, including unary accumulator operations (NEG, COM, INC, DEC and TST) and conditional-flag handling.
- PSHS/PULS cycle accounting includes the number of transferred bytes; host tests cover the two-byte PC transfer case.
- `MemoryInterface` abstraction for CPU byte/word memory operations, with a flat `MemoryBus` implementation.
- `FM7Machine` main CPU memory interface and a `SubsystemMemoryView` for the subsystem address space.
- Both CPU views map their shared-RAM windows to the same 128-byte backing array.
- `FM7Scheduler` deterministic cooperative scheduler scaffold with independent cycle counters, configurable subsystem-to-main cycle ratio, and reset support.
- `stepM6809` adapter allows scheduler callbacks to step an M6809 and report consumed cycles.
- Automated tests cover CPU operations, machine memory views, shared-RAM visibility, and scheduler cycle budgets.
- Host executable named `fum7x360`.

**Hardware-accuracy note:** this remains a development scaffold. The address map and CPU timing need validation against hardware documentation and a reference implementation. The scheduler's default ratio is only a placeholder, not a claim about FM-7 clock ratios. It does not yet provide cycle-exact bus arbitration, interrupt scheduling, or simultaneous CPU execution. ROM banking, peripheral decoding, video hardware, I/O devices and complete CPU instruction coverage are incomplete. This cannot yet run FM-7 firmware or commercial software.

## Planned work

1. Validate the memory map, then implement ROM/RAM banking and peripheral decode.
2. Verify CPU clocks and add shared-memory arbitration, interrupts, and processor coordination.
3. Expand and validate 6809 instruction coverage, flags and cycle timing.
4. Implement video RAM/planes, palette and scanout.
5. Add sound, keyboard/joystick, cassette and disk devices.
6. Integrate the Xbox 360 frontend and select a compatible SDK/toolchain.
7. Build and test a genuine Xbox 360 `default.xex` on supported hardware or a suitable test environment.

## Host build

Requires a C++17 compiler and CMake:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/fum7x360
```

This builds a host development executable, not an Xbox 360 executable.

## References

- [MAME FM-7 driver](https://github.com/mamedev/mame/blob/master/src/mame/fujitsu/fm7.cpp)
- [FM-7 technical notes](https://github.com/0cjs/sedoc/blob/main/8bit/fm7/fm-7.md)

Firmware/BIOS images and commercial software are not included. Use legally obtained files and keep them separate from the source tree.
