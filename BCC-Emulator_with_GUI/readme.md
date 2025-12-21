# BCC-500 Computer System Emulator

A multi-processor emulator for the fictional BCC-500 computer system, inspired by the SDS 940 architecture. Features 24-bit word size, memory-mapped I/O, an interactive debugger, and support for up to 6 parallel processors.

## ✨ New Features

- **🎨 GUI Interface** — Beautiful ncurses-based terminal UI with visual displays
- **📝 Assembly Language** — Write programs in human-readable assembly instead of hex
- **📁 File Browser** — Visual file picker to load programs easily
- **📚 Example Programs** — 5+ ready-to-run test programs included
- **⚡ Live Updates** — Real-time processor state and terminal display
- **🔨 Build System** — Simple Makefile for easy compilation

## Core Features

- **Multi-processor support** — Emulates up to 6 independent processors with shared memory
- **24-bit architecture** — Authentic word size with 18-bit addressing (256K words)
- **Extended instruction set** — 52 opcodes based on the SDS 940
- **Memory-mapped I/O** — Tape drive, card reader, console, and terminal devices
- **Interactive debugger** — Breakpoints, single-stepping, memory inspection
- **Binary file loader** — Load and save programs in native 24-bit format
- **Thread-safe design** — Proper synchronization for concurrent processor execution

## Building

### Quick Build (Recommended)

```bash
make all
```

This builds both emulator versions:
- `emulator` — Original command-line version
- `gui-emulator` — New GUI version with assembler

### Requirements

- C++17 compatible compiler
- pthreads
- ncurses development library (for GUI version)

**Install ncurses:**

On Fedora/RHEL:
```bash
sudo dnf install ncurses-devel
```

On Ubuntu/Debian:
```bash
sudo apt-get install libncurses-dev
```

### Manual Build

```bash
# GUI emulator (recommended)
g++ -std=c++17 -Wall -Wextra -pthread -O2 -o gui-emulator GuiEmulator.cpp -lncurses

# Original emulator
g++ -std=c++17 -Wall -Wextra -pthread -O2 -o emulator Emulator.cpp
```

## Quick Start

### GUI Emulator (Recommended)

```bash
./gui-emulator
```

**Using the GUI:**
1. Press **F1** to open file browser
2. Navigate to `examples` directory
3. Select a program (e.g., `hello.asm`)
4. Press **Enter** to load
5. Press **F3** to run!

See **QUICKSTART.md** for detailed GUI guide.

### Original Emulator

```bash
./emulator
```

This launches a demo program that prints "Hello, BCC-500!" to the terminal display, then enters the interactive debugger.

## Architecture

### Registers

| Register | Size | Description |
|----------|------|-------------|
| ACC (A) | 24-bit | Primary accumulator |
| B | 24-bit | Secondary accumulator |
| X | 24-bit | Index register |
| PC | 18-bit | Program counter |
| SP | 18-bit | Stack pointer |
| IR | 24-bit | Instruction register |

### Flags

| Flag | Description |
|------|-------------|
| Z | Zero — Set when result is zero |
| N | Negative — Set when sign bit is set |
| O | Overflow — Set on arithmetic overflow |
| C | Carry — Set on unsigned overflow |
| I | Interrupt Enable — Controls interrupt handling |

### Memory Map

| Address Range | Size | Description |
|---------------|------|-------------|
| 0x00000–0x3EFFF | 254K words | Main memory |
| 0x3F000–0x3FFFF | 4K words | Memory-mapped I/O |

### Instruction Format

Each instruction is a 24-bit word:

```
[23:18] Opcode (6 bits)
[17:0]  Address/Operand (18 bits)
```

## Instruction Set

### Basic Operations

| Opcode | Hex | Mnemonic | Description |
|--------|-----|----------|-------------|
| 0x00 | HALT | Halt processor |
| 0x01 | NOP | No operation |
| 0x02 | LOAD | Load ACC from memory |
| 0x03 | STORE | Store ACC to memory |
| 0x04 | ADD | Add memory to ACC |
| 0x05 | SUB | Subtract memory from ACC |
| 0x06 | AND | Bitwise AND with memory |
| 0x07 | OR | Bitwise OR with memory |
| 0x08 | XOR | Bitwise XOR with memory |

### Control Flow

| Opcode | Hex | Mnemonic | Description |
|--------|-----|----------|-------------|
| 0x09 | JUMP | Unconditional jump |
| 0x0A | JZE | Jump if zero |
| 0x0B | JNZ | Jump if not zero |
| 0x0C | JGT | Jump if greater than zero |
| 0x0D | JLT | Jump if less than zero |
| 0x0E | CALL | Call subroutine |
| 0x0F | RET | Return from subroutine |
| 0x2E | JOV | Jump on overflow |
| 0x2F | BRU | Branch unconditional |

### Stack Operations

| Opcode | Hex | Mnemonic | Description |
|--------|-----|----------|-------------|
| 0x10 | PUSH | Push ACC onto stack |
| 0x11 | POP | Pop stack into ACC |

### Shift and Rotate

| Opcode | Hex | Mnemonic | Description |
|--------|-----|----------|-------------|
| 0x12 | SHIFT_L | Shift left by operand bits |
| 0x13 | SHIFT_R | Shift right by operand bits |
| 0x28 | ROL | Rotate left |
| 0x29 | ROR | Rotate right |

### Arithmetic

| Opcode | Hex | Mnemonic | Description |
|--------|-----|----------|-------------|
| 0x14 | MUL | Multiply ACC by memory |
| 0x15 | DIV | Divide ACC by memory |
| 0x16 | MOD | Modulo ACC by memory |
| 0x17 | CMP | Compare ACC with memory (sets flags only) |
| 0x22 | ADC | Add with carry |
| 0x23 | SBC | Subtract with carry |
| 0x24 | NEG | Negate ACC (two's complement) |
| 0x25 | COM | Complement ACC (one's complement) |
| 0x26 | INC | Increment ACC |
| 0x27 | DEC | Decrement ACC |

### Data Movement

| Opcode | Hex | Mnemonic | Description |
|--------|-----|----------|-------------|
| 0x18 | MOVE | Move memory to memory |
| 0x19 | INPUT | Read from console to ACC |
| 0x1A | OUTPUT | Write ACC to console |
| 0x1B | LDA | Load A register |
| 0x1C | LDB | Load B register |
| 0x1D | STA | Store A register |
| 0x1E | STB | Store B register |
| 0x1F | LDX | Load index register |
| 0x20 | STX | Store index register |

### Skip Instructions

| Opcode | Hex | Mnemonic | Description |
|--------|-----|----------|-------------|
| 0x2A | SKE | Skip next if equal (zero flag set) |
| 0x2B | SKN | Skip next if not equal |
| 0x2C | SKG | Skip next if greater |
| 0x2D | SKL | Skip next if less |

### Advanced

| Opcode | Hex | Mnemonic | Description |
|--------|-----|----------|-------------|
| 0x21 | EOR | Exclusive OR (alias for XOR) |
| 0x30 | EXU | Execute instruction at address |
| 0x31 | MIY | Add index register to memory |
| 0x32 | INT | Software interrupt |
| 0x33 | RTI | Return from interrupt |

## I/O Devices

### Device Registers

| Address | Device | Register |
|---------|--------|----------|
| 0x3F000 | Tape Drive | Status |
| 0x3F001 | Tape Drive | Data |
| 0x3F002 | Tape Drive | Command |
| 0x3F010 | Card Reader | Status |
| 0x3F011 | Card Reader | Data |
| 0x3F020 | Console | Status |
| 0x3F021 | Console | Data |
| 0x3F030 | Terminal | Status |
| 0x3F031 | Terminal | Data |
| 0x3F032 | Terminal | Command |

### Tape Drive

Simulates a magnetic tape unit with read, write, and positioning capabilities.

**Status bits:**
- Bit 0: Mounted
- Bit 1: Ready
- Bit 2: End of tape

**Commands:**
- 0: Rewind
- 1: Backward one record
- 2: Forward one record

### Card Reader

Simulates a punch card reader (read-only).

**Status bits:**
- Bit 0: Ready
- Bit 1: Card available

### Console

Simple character I/O device that echoes to stdout.

**Status bits:**
- Bit 0: Ready
- Bit 1: Input available
- Bit 2: Output pending

### Terminal Display

80×24 character terminal with cursor control and screen buffer.

**Status bits:**
- Bit 0: Ready
- Bit 1: Input available
- Bits 8–15: Cursor X position
- Bits 16–23: Cursor Y position

**Commands:**
- 0: Clear screen
- 1: Set cursor X (position in bits 8–15)
- 2: Set cursor Y (position in bits 8–15)
- 3: Set cursor X,Y (X in bits 8–15, Y in bits 16–23)
- 4: Set cursor visibility (bit 8)

## Interactive Debugger

The emulator includes a built-in debugger with the following commands:

| Command | Alias | Description |
|---------|-------|-------------|
| `help` | | Show command list |
| `break <addr>` | `b` | Set breakpoint at hex address |
| `delete <addr>` | `d` | Remove breakpoint |
| `list` | | List all breakpoints |
| `continue` | `c` | Continue execution |
| `step` | `s` | Execute one instruction |
| `run <id>` | `r` | Start processor (0–5) |
| `stop` | | Stop all processors |
| `print [id]` | `p` | Print processor or system state |
| `memory <addr>` | `m` | Examine memory at hex address |
| `display` | `disp` | Render terminal screen |
| `load <file> [id] [addr]` | | Load binary file |
| `quit` | `q` | Exit emulator |

### Example Session

```
dbg> break 0x10
Breakpoint set at 0x10
dbg> run 0
Running processor 0
dbg> print 0
Processor 0 State:
  State: BREAKPOINT
  PC: 0x00010
  ACC: 0x000048
  ...
dbg> memory 200
Memory[0x200] = 0x000048
dbg> continue
dbg> display
```

## Binary File Format

Programs are stored as raw 24-bit words in big-endian format (3 bytes per word):

```
Byte 0: bits 23–16 (high byte)
Byte 1: bits 15–8  (middle byte)
Byte 2: bits 7–0   (low byte)
```

### Loading Programs

From the debugger:
```
dbg> load program.bin 0 0x100
Loaded 42 words from program.bin
```

This loads `program.bin` into processor 0 starting at address 0x100.

## Assembly Language

### Syntax

Write programs in human-readable assembly:

```assembly
; Comments start with semicolon
LABEL: OPCODE OPERAND

; Data directives
.WORD value     ; 24-bit word
.BYTE value     ; 8-bit byte
.STRING "text"  ; String data
```

### Operand Formats

```assembly
LOAD 0x200      ; Hex number
LOAD 512        ; Decimal number
LOAD 'A'        ; Character literal
LOAD MYLABEL    ; Label reference
```

### Example Programs

All examples are in the `examples/` directory and can be loaded via the GUI.

#### Hello World (`examples/hello.asm`)

```assembly
; Print "Hello, World!" to terminal
START:
    LOAD CHAR_H
    OUTPUT 0x3F031      ; Terminal data register

    LOAD CHAR_E
    OUTPUT 0x3F031
    ; ... (more characters)

    HALT

CHAR_H: .BYTE 'H'
CHAR_E: .BYTE 'e'
; ... (more data)
```

#### Countdown Loop (`examples/counter.asm`)

```assembly
; Count down from 10 to 0
START:
    LOAD COUNT

LOOP:
    ADD ASCII_ZERO      ; Convert to ASCII
    OUTPUT 0x3F031      ; Display digit

    LOAD SPACE
    OUTPUT 0x3F031

    LOAD COUNT
    DEC
    STORE COUNT
    JNZ LOOP           ; Loop if not zero

    HALT

COUNT:      .WORD 10
ASCII_ZERO: .WORD 48
SPACE:      .WORD 32
```

#### Fibonacci Sequence (`examples/fibonacci.asm`)

```assembly
; Generate first 10 Fibonacci numbers
START:
    LOAD ZERO
    STORE FIB_A

    LOAD ONE
    STORE FIB_B

    LOAD TEN
    STORE COUNTER

LOOP:
    LOAD FIB_A
    OUTPUT 0x3F031

    ; Calculate next: FIB_C = FIB_A + FIB_B
    LOAD FIB_A
    ADD  FIB_B
    STORE FIB_C

    ; Shift: A=B, B=C
    LOAD FIB_B
    STORE FIB_A
    LOAD FIB_C
    STORE FIB_B

    LOAD COUNTER
    DEC
    STORE COUNTER
    JNZ LOOP

    HALT

ZERO:    .WORD 0
ONE:     .WORD 1
TEN:     .WORD 10
FIB_A:   .WORD 0
FIB_B:   .WORD 0
FIB_C:   .WORD 0
COUNTER: .WORD 0
```

#### Stack Operations (`examples/stack_test.asm`)

```assembly
; Test PUSH, POP, CALL, RET
START:
    LOAD VAL1
    PUSH

    LOAD VAL2
    PUSH

    LOAD VAL3
    PUSH

    CALL DISPLAY_SUM
    HALT

DISPLAY_SUM:
    POP
    STORE TEMP1
    POP
    STORE TEMP2
    POP
    ADD TEMP1
    ADD TEMP2
    OUTPUT 0x3F031
    RET

VAL1:  .WORD 100
VAL2:  .WORD 200
VAL3:  .WORD 300
TEMP1: .WORD 0
TEMP2: .WORD 0
```

### C++ Programming (Legacy Method)

For direct machine code generation:

```cpp
// Encode instructions as: (opcode << 18) | address
std::vector<Word24> program = {
    (0x02 << 18) | 0x200,        // LOAD 0x200
    (0x1A << 18) | 0x3F031,      // OUTPUT to terminal
    (0x00 << 18) | 0x00          // HALT
};

// Store data
memory.write(0x200, 'H');
```

## GUI Emulator Interface

### Screen Layout

```
┌──────────────────────────────────────────────────────────────────┐
│ [F1]Load [F2]Assemble [F3]Run [F4]Step [F5]Stop [F6]Reset [F10]Quit │
├────────────────────────┬─────────────────────────────────────────┤
│                        │                                         │
│   Terminal Display     │      Processor 0 State                  │
│   (Program output      │      State: IDLE                        │
│    appears here)       │      PC:  0x00000                       │
│                        │      ACC: 0x000000                      │
│                        │      B:   0x000000                      │
│                        │      X:   0x000000                      │
│                        │      SP:  0x0FFFF                       │
│                        │      Flags: ----                        │
│                        │      Cycles: 0                          │
│                        │      Instructions: 0                    │
├────────────────────────┴─────────────────────────────────────────┤
│                      Memory View (0x0000-0x007F)                 │
│  0000: 000000 000000 000000 000000 000000 000000 000000 000000   │
│  0008: 000000 000000 000000 000000 000000 000000 000000 000000   │
│  ...                                                              │
├───────────────────────────────────────────────────────────────────┤
│ Status: Welcome to BCC-500! Press F1 to load a program...        │
└───────────────────────────────────────────────────────────────────┘
```

### Function Keys

| Key | Action | Description |
|-----|--------|-------------|
| **F1** | Load | Browse and load assembly files |
| **F2** | Assemble | Reload/reassemble current program |
| **F3** | Run | Execute the loaded program |
| **F4** | Step | Execute single instruction |
| **F5** | Stop | Halt execution |
| **F6** | Reset | Reset processor state |
| **F10** | Quit | Exit emulator |

### File Browser

When you press **F1**:
- Use **↑**/**↓** arrow keys to navigate
- Press **Enter** on directories to open them
- Press **Enter** on `.asm` files to load them
- Press **ESC** to cancel

The assembler automatically compiles the program and loads it into memory.

## API Reference

### Assembler Class

```cpp
#include "Assembler.h"

Assembler assembler;
std::string source = "START: LOAD 0x100\n       HALT";

// Assemble the program
if (assembler.assemble(source)) {
    const std::vector<Word24>& machineCode = assembler.getMachineCode();
    const std::map<std::string, Addr18>& labels = assembler.getLabels();

    // Load into system
    system.loadProgram(machineCode, 0, 0);
} else {
    // Handle errors
    const std::vector<std::string>& errors = assembler.getErrors();
    for (const auto& error : errors) {
        std::cerr << error << std::endl;
    }
}
```

### BCC500System

Main system class that manages processors, memory, and I/O.

```cpp
BCC500System system;

// Load and run a program
system.loadProgram(program, processorId, startAddress);
system.runProcessor(processorId, maxCycles);

// Async execution
system.runProcessorAsync(processorId);
system.runAllProcessorsAsync();
system.stopAllProcessors();
system.joinAllProcessors();

// Access components
system.getProcessor(id);
system.getMemory();
system.getDebugger();
system.getIOController();
```

### BinaryLoader

Utility class for loading and saving binary programs.

```cpp
// Load a binary file
auto program = BinaryLoader::loadBinary("program.bin");

// Save a program to binary
BinaryLoader::saveBinary("output.bin", program);
```

## Thread Safety

The emulator uses the following synchronization:

- `SharedMemory`: Mutex-protected read/write operations
- `IOController`: Mutex-protected device access
- `Debugger`: Condition variable for breakpoint synchronization
- `ProcessorState`: Atomic operations for state changes

Multiple processors can safely execute concurrently with shared memory access properly serialized.

## Project Structure

```
bccemulator/
├── emulator              # Original CLI emulator
├── gui-emulator          # New GUI emulator (recommended)
├── Emulator.cpp          # Original source with demo
├── EmulatorCore.h        # Core classes (no main)
├── Assembler.h           # Assembly language parser
├── GuiEmulator.cpp       # GUI implementation
├── Makefile              # Build system
├── readme.md             # This file
├── QUICKSTART.md         # User-friendly guide
├── CLAUDE.md             # Developer documentation
└── examples/             # Example programs
    ├── hello.asm         # Hello World
    ├── counter.asm       # Countdown demo
    ├── fibonacci.asm     # Fibonacci sequence
    ├── multiply.asm      # Multiplication
    └── stack_test.asm    # Stack operations
```

## Included Example Programs

| File | Description | Demonstrates |
|------|-------------|--------------|
| `hello.asm` | Prints "Hello, World!" | Basic I/O, data storage |
| `counter.asm` | Counts down from 10 | Loops, conditionals |
| `fibonacci.asm` | First 10 Fibonacci numbers | Arithmetic, loops |
| `multiply.asm` | Multiply 7 × 6 by repeated addition | Arithmetic loops |
| `stack_test.asm` | Tests stack operations | PUSH, POP, CALL, RET |

All examples can be loaded through the GUI file browser (F1).

## Makefile Targets

```bash
make all            # Build both emulator versions
make emulator       # Build original CLI version
make gui-emulator   # Build GUI version
make run            # Run original emulator
make run-gui        # Run GUI emulator
make clean          # Remove build artifacts
make debug          # Build with debug symbols
```

## Documentation

- **QUICKSTART.md** — Quick start guide for GUI emulator with examples
- **CLAUDE.md** — Technical architecture and developer guide
- **readme.md** — This file, complete reference

## Tips and Best Practices

### Writing Assembly Programs

1. **Use meaningful labels** — `LOOP:` instead of `L1:`
2. **Add comments** — Explain what your code does
3. **Organize data** — Put data sections at the end
4. **Test incrementally** — Use F4 (step) to debug

### Using the GUI

1. **Start simple** — Load and run the example programs first
2. **Watch the registers** — Monitor PC, ACC, and flags during execution
3. **Single-step for debugging** — Press F4 to step through instructions
4. **Check the terminal** — Program output appears in the Terminal Display

### Debugging

1. **Check the status bar** — Error messages appear there
2. **Verify addresses** — Terminal is 0x3F031, Console is 0x3F021
3. **Watch for HALT** — Programs should end with HALT instruction
4. **Reset when needed** — Press F6 to reset and F2 to reassemble

## Limitations

- No floating-point support
- No virtual memory or memory protection
- Simplified interrupt model
- Terminal display uses ANSI escape codes (may not work on all terminals)
- GUI requires ncurses library

## Troubleshooting

**GUI won't compile:**
```bash
# Install ncurses development library
# Fedora/RHEL:
sudo dnf install ncurses-devel
# Ubuntu/Debian:
sudo apt-get install libncurses-dev
```

**Assembly errors:**
- Check syntax: `OPCODE OPERAND`
- Verify label names are defined
- Ensure data directives have values

**Program doesn't run:**
- Make sure it's loaded (F1)
- Check that it ends with HALT
- Verify OUTPUT addresses are correct

**Terminal output not showing:**
- Use address 0x3F031 for terminal
- Make sure you press F3 to run
- Check Terminal Display window (top-left)

## License

This project is provided as-is for educational purposes.

