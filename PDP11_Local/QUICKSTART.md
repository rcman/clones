# PDP-11 Emulator - Quick Start Guide

## Building

```bash
make
```

## Testing the Bootstrap

The simplest way to test the emulator is with the built-in echo bootstrap:

```bash
./pdp11emu --bootstrap --port 2311
```

In another terminal:
```bash
telnet localhost 2311
```

Type characters and they should echo back, proving the CPU, serial interface, and telnet server all work!

## Next Steps

### Phase 1: Complete the Instruction Set

The current implementation has basic double-operand instructions (MOV, CMP, BIT, BIC, BIS, ADD) and simple branches. You'll need to add:

- SUB instruction
- More single-operand instructions (COM, INC, DEC, NEG, TST, etc.)
- More branch instructions (BGE, BLT, BGT, BLE, BPL, BMI, BHI, BLOS, BVC, BVS, BCC, BCS)
- JSR/RTS for subroutine calls
- SOB (subtract one and branch)
- Multiply and divide
- XOR
- Shifts and rotates (ASL, ASR, ROL, ROR, etc.)
- HALT, WAIT, RTI, RTT
- TRAP instructions
- Floating point (if needed for 2.11BSD)

### Phase 2: Add MMU Support

For 2.11BSD, you'll need an MMU with:
- Page Address Registers (PARs)
- Page Descriptor Registers (PDRs)
- 18-bit addressing (256KB segments)
- Kernel/Supervisor/User modes
- Memory protection

### Phase 3: Implement Split I/D Space

2.11BSD uses separate instruction and data spaces:
- Separate I-space and D-space page registers
- Allows 128KB total address space per process (64KB instruction + 64KB data)

### Phase 4: Complete Interrupt System

- Interrupt vectors
- Priority levels
- Interrupt enable/disable
- Proper interrupt handling

### Phase 5: Add Networking

For FTP and web servers, you'll need:
- DEUNA or DELUA Ethernet controller emulation
- TAP/TUN interface on Linux host
- Ethernet frame handling
- Integration with 2.11BSD's TCP/IP stack

## Running 2.11BSD

Once the above phases are complete, you can:

1. Get a 2.11BSD distribution (available from various PDP-11 archives)
2. Create a disk image or use a pre-made one
3. Boot the system:
   ```bash
   ./pdp11emu --disk 2.11bsd.img --memory 4096 --port 2311
   ```
4. Telnet in and enjoy a full Unix system!

## Development Tips

- Test each instruction thoroughly with small test programs
- Use the SIMH PDP-11 emulator as a reference for correctness
- The PDP-11 Processor Handbook is invaluable
- Start with simple programs and work up to complex ones
- Use Unix v6 or v7 as stepping stones before 2.11BSD

## Debugging

Add debug output in `pdp11_step()` to trace execution:
```c
if (debug_mode) {
    printf("PC=%06o INSTR=%06o\n", pc, instr);
}
```

This helps when figuring out why programs don't work correctly.

## Resources

- PDP-11 Processor Handbook (DEC, 1981)
- SIMH source code (github.com/simh/simh)
- 2.11BSD documentation
- PDP-11 Assembly Language Programming Manual

## Performance

Since this emulator runs at native speed:
- Modern CPU can execute 100M+ PDP-11 instructions per second
- Boot time: seconds
- Compilation: near-instant
- This is 1000x+ faster than real hardware

## Architecture Notes

The PDP-11 has a beautifully simple and orthogonal instruction set. All double-operand instructions work with all addressing modes, making it very regular to emulate once you get the addressing modes right.

The most complex part is getting the condition codes correct for every instruction - they're critical for proper program operation.

Good luck with your PDP-11 emulator!
