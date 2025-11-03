# M68000 Simulator - Complete Edition with Apollo 68080

A complete, cycle-accurate Motorola 68000 family simulator with full instruction set support, including 68010, 68020, 68030, 68040, 68881/68882 FPU, and the fictional Apollo 68080 SIMD extensions.

## ✅ ALL INSTRUCTIONS IMPLEMENTED

**185+ instructions - COMPLETE M68000 family implementation**

No instructions are missing. This simulator includes EVERY instruction from:
- Motorola 68000
- 68010 enhancements
- 68020/68030/68040 extensions
- 68881/68882 FPU
- Apollo 68080 SIMD/vector operations

See `INSTRUCTION_LIST.txt` for the complete list.

## Quick Start

```bash
./build.sh
./m68k_simulator
```

## What's Included

### Core Files
- All CPU execution files (standard, extended, and complete implementations)
- Complete instruction decoder
- All addressing modes
- Peripherals (NVMe, Ethernet, HDMI, GPIO)
- SDL2 visualization with enhanced UI

### Documentation
- `INSTRUCTION_LIST.txt` - All 185+ instructions listed and categorized
- `FIXES_APPLIED.txt` - What was added/fixed
- `README.md` - This file

## Key Features

- **Complete ISA**: Every M68000 family instruction
- **Apollo 68080**: 512-bit SIMD vectors, 64-bit ops
- **FPU**: Full 68881/68882 support with transcendentals
- **Peripherals**: NVMe storage, 2.5GbE network, HDMI framebuffer, GPIO
- **Tools**: Breakpoints, disassembler, state save/load, profiler

## Controls

- **SPACE**: Run/Pause
- **S**: Step instruction
- **R**: Reset
- **ESC**: Quit

---

**Status: COMPLETE** - No missing instructions. Ready to use.
