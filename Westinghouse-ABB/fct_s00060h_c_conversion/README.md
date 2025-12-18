# S00060-H FCT - C Conversion

## Westinghouse Field Configuration Terminal

This is a C language conversion of the MC68HC11 assembly code for the Westinghouse 
Field Configuration Terminal (FCT), unit 1993D40G06 (12.5kHz variant).

### Original Hardware

- **MPU**: SC44125P (MC68HC11 variant)
- **EPROMs**: 2732 (U11: S00060G-U11, U17: S00060G-U17)
- **RAM**: MC6810 (128 bytes low RAM), HM6561 x2 (NVRAM, high RAM)
- **Timer**: MC6840 Programmable Timer Module
- **I/O**: MC6821 Peripheral Interface Adapter
- **Display**: 8279 Keyboard/Display Controller

### Product Description

The FCT is a portable unit for servicing:
- LMT-1xx (1-way load management terminals)
- LMT-2 / MCT-2xx (2-way load management/meter control terminals)
- DCT-501 (distribution control terminal)

The unit contains an LMT-2 board set configured with interface and display boards.
Communication uses Coherent Phase-Shift Keying (CPSK) modulation as described in 
US Patent 4311964.

### File Structure

```
fct_s00060h.h        - Header file with all definitions and prototypes
fct_s00060h_part1.c  - Initialization, display, and utility functions
fct_s00060h_part2.c  - Main loop and test suites
fct_s00060h_part3.c  - NMI interrupt handler (CPSK signal processing)
fct_hal.c            - Hardware abstraction layer (platform-specific)
Makefile             - Build configuration
README.md            - This file
```

### Building

```bash
# Build simulator executable
make

# Build with debug symbols
make debug

# Build object files only (for embedding in other projects)
make objects

# Clean build artifacts
make clean
```

### Porting to Your Platform

1. **Modify `fct_hal.c`**: Implement the hardware access functions for your target:
   - `hw_write_byte(addr, value)` - Write byte to hardware address
   - `hw_read_byte(addr)` - Read byte from hardware address  
   - `hw_write_word(addr, value)` - Write 16-bit word
   - `hw_read_word(addr)` - Read 16-bit word

2. **Set up interrupts**: The `nmi_interrupt()` function should be called at the
   appropriate rate (driven by MC6840 Timer 2, approximately 4800 Hz for 12.5kHz 
   carrier, or 3686 Hz for 9.615kHz carrier).

3. **Initialize hardware**: Ensure your platform can access the equivalent of:
   - MC6840 timer registers
   - MC6821 PIA ports
   - 8279 keyboard/display controller
   - Configuration switches and relay status inputs

### Key Constants

| Constant | Value | Description |
|----------|-------|-------------|
| LED_OFF | 0x0F | Turn off LED segment |
| LED_FAIL | 0x04 | Failure indication |
| LED_PASS | 0x07 | Pass indication |
| LED_ALL_ON | 0x08 | All segments lit |
| UUT_LMT1XX | 1 | 1-way unit type |
| UUT_LMT2_MCT | 2 | 2-way unit type |
| UUT_DCT | 4 | DCT unit type |

### Test Sequence Overview

1. **Test 01**: Validate switch and jumper settings
2. **Test 15**: Download address to UUT (2-way, install mode)
3. **Test 20**: Read/verify UUT address
4. **Test 25**: Get hardware ID and firmware info
5. **Test 30**: Setup/configuration tests
6. **Test 35**: Timed relay tests (A-D)
7. **Test 40**: Latched relay D test
8. **Test 50**: Toggle test mode
9. **Tests 61-80**: DCT-specific analog and status tests

### Error Codes

| Code | Meaning |
|------|---------|
| 1 | No transmit from FCT |
| 2 | No response from UUT |
| 3 | Incomplete response from UUT |
| 4 | BCH checksum error |
| 5 | Other data error |
| 6 | FCT number / hardware mismatch |
| 7 | Firmware not in lookup table |

### Signal Processing

The CPSK demodulation algorithm in `nmi_interrupt()` implements the technique from
US Patent 4311964:

1. Sample incoming bit stream at 8x carrier rate
2. Detect bit transitions using XOR comparison
3. Calculate instantaneous I/Q vector values using sine lookup tables
4. Compute rolling averages with 15/16 IIR filter
5. Determine signal level and threshold check
6. Calculate vector angle for phase detection
7. Update M-tables for bit decision making

### License

This code is a conversion of Westinghouse proprietary firmware for educational
and historical preservation purposes. Use at your own risk.

### References

- US Patent 4311964 - "Coherent Phase Shift Keyed Demodulator"
- MC68HC11 Reference Manual
- MC6840 Programmable Timer Module Datasheet
- MC6821 Peripheral Interface Adapter Datasheet
- Intel 8279 Keyboard/Display Controller Datasheet
