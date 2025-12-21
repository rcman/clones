# BCC-500 Emulator Quick Start Guide

Welcome to the BCC-500 Emulator! This guide will get you up and running quickly.

## What's New?

The emulator now includes:
- **Human-readable assembly language** - No more cryptic hex encoding!
- **GUI interface** - Easy-to-use ncurses-based interface with menus
- **File browser** - Load programs with a visual file picker
- **Example programs** - Pre-written test programs to try immediately

## Building

### Quick Build
```bash
make all
```

This builds both versions:
- `emulator` - Original command-line version
- `gui-emulator` - New GUI version (recommended!)

### Requirements
- C++17 compatible compiler
- ncurses library (for GUI version)

On Fedora/RHEL:
```bash
sudo dnf install ncurses-devel
```

On Ubuntu/Debian:
```bash
sudo apt-get install libncurses-dev
```

## Running the GUI Emulator

### Quick Start
```bash
./gui-emulator
```

Or use the Makefile:
```bash
make run-gui
```

### GUI Interface Overview

```
┌─────────────────────────────────────────────────────────────────┐
│ [F1]Load [F2]Assemble [F3]Run [F4]Step [F5]Stop [F6]Reset ...  │ Menu
├─────────────────────────┬───────────────────────────────────────┤
│                         │                                       │
│   Terminal Display      │      Processor 0 State                │
│   (Program output       │      PC:  0x00000                     │
│    appears here)        │      ACC: 0x000000                    │
│                         │      B:   0x000000                    │
│                         │      X:   0x000000                    │
│                         │      SP:  0x0FFFF                     │
│                         │      Flags: ----                      │
│                         │      Cycles: 0                        │
├─────────────────────────┴───────────────────────────────────────┤
│                      Memory View                                 │
│  0000: 000000 000000 000000 000000 000000 000000 000000 000000  │
│  ...                                                             │
├─────────────────────────────────────────────────────────────────┤
│ Status messages appear here...                                  │ Status
└─────────────────────────────────────────────────────────────────┘
```

### Function Keys

| Key | Action | Description |
|-----|--------|-------------|
| **F1** | Load | Browse and load assembly files |
| **F2** | Assemble | Reload current program |
| **F3** | Run | Execute the program |
| **F4** | Step | Execute one instruction |
| **F5** | Stop | Halt execution |
| **F6** | Reset | Reset processor |
| **ESC** or **Q** | Quit | Exit emulator |

## Writing Assembly Programs

### Assembly Language Syntax

```assembly
; Comments start with semicolon
LABEL: OPCODE OPERAND  ; end-of-line comments work too

; Labels are optional
       LOAD   VALUE
       ADD    0x100
       STORE  RESULT
       HALT

VALUE:  .WORD 42
RESULT: .WORD 0
```

### Instruction Format

**Mnemonics** (case-insensitive):
```assembly
LOAD address    ; Load from memory to ACC
STORE address   ; Store ACC to memory
ADD address     ; Add memory to ACC
SUB address     ; Subtract memory from ACC
JUMP label      ; Unconditional jump
JZE label       ; Jump if zero
CALL label      ; Call subroutine
RET             ; Return from subroutine
HALT            ; Stop processor
```

**Operand Formats**:
```assembly
LOAD 0x200      ; Hex number
LOAD 512        ; Decimal number
LOAD 'A'        ; Character literal
LOAD MYLABEL    ; Label reference
```

**Data Directives**:
```assembly
.WORD value     ; 24-bit word
.BYTE value     ; 8-bit byte
.STRING "text"  ; String data
```

### Example Program

Create `myprogram.asm`:

```assembly
; Print "Hi!" to terminal
START:
    LOAD CHAR_H
    OUTPUT 0x3F031      ; Terminal data register

    LOAD CHAR_I
    OUTPUT 0x3F031

    LOAD CHAR_EXCLAIM
    OUTPUT 0x3F031

    LOAD NEWLINE
    OUTPUT 0x3F031

    HALT

; Data
CHAR_H:       .BYTE 'H'
CHAR_I:       .BYTE 'i'
CHAR_EXCLAIM: .BYTE '!'
NEWLINE:      .BYTE 10
```

### Running Your Program

1. Start the GUI emulator: `./gui-emulator`
2. Press **F1** to open the file browser
3. Navigate to your `.asm` file using **↑**/**↓** arrows
4. Press **Enter** to load and assemble
5. Press **F3** to run
6. Watch the output in the Terminal Display!

## Example Programs

Pre-built examples in the `examples/` directory:

| File | Description |
|------|-------------|
| `hello.asm` | Prints "Hello, World!" |
| `counter.asm` | Counts down from 10 to 0 |
| `fibonacci.asm` | Generates Fibonacci sequence |
| `multiply.asm` | Multiply two numbers (7 × 6 = 42) |
| `stack_test.asm` | Tests PUSH/POP/CALL/RET |

Try them out:
1. Press **F1**
2. Navigate to `examples` directory
3. Press **Enter** to open directory
4. Select a program and press **Enter**
5. Press **F3** to run!

## Common Operations

### Loading a Program
```
F1 → Navigate to file → Enter → Program loads automatically
```

### Running Step-by-Step
```
F4 (Step) → Watch registers update → F4 again → ...
```

### Debugging
```
Watch the Processor State panel for:
- Current PC (program counter)
- Register values (ACC, B, X, SP)
- Processor flags (Z, N, O, C)
- Instruction count and cycles
```

### Viewing Output
Terminal output appears in the "Terminal Display" window (top-left).

## Device Addresses

For OUTPUT/INPUT instructions:

| Device | Address | Description |
|--------|---------|-------------|
| Console | 0x3F021 | Character I/O |
| Terminal | 0x3F031 | 80×24 display |
| Tape | 0x3F001 | Tape drive data |
| Card | 0x3F011 | Card reader data |

## Tips

1. **Start simple** - Try the example programs first
2. **Use labels** - Much easier than hex addresses
3. **Comments** - Document your code with `;` comments
4. **Single-step** - Use F4 to debug instruction-by-instruction
5. **Terminal output** - Use `OUTPUT 0x3F031` to display results

## Troubleshooting

**"Assembly errors" message**
- Check syntax: `OPCODE OPERAND`
- Verify label names match
- Ensure proper data types (.WORD, .BYTE)

**Program doesn't output anything**
- Make sure you're using the correct device address (0x3F031 for terminal)
- Check that OUTPUT instruction is before HALT

**"Terminal too small!" message**
- The GUI requires a minimum size of 80 columns × 24 rows
- Resize your terminal window to meet the minimum requirements
- The GUI will automatically adapt to larger window sizes
- Dynamic resizing is supported - just resize your terminal and the GUI will adjust

## Next Steps

- Read `readme.md` for complete instruction set
- Check `CLAUDE.md` for architecture details
- Modify example programs to learn
- Write your own programs!

## Quick Reference Card

```
Instructions:
  LOAD, STORE, ADD, SUB, MUL, DIV
  AND, OR, XOR, NOT
  JUMP, JZE, JNZ, JGT, JLT
  CALL, RET, PUSH, POP
  HALT

Aliases:
  JMP = JUMP, JZ = JZE
  IN = INPUT, OUT = OUTPUT

Data:
  .WORD value
  .BYTE value
  .STRING "text"

Comments:
  ; This is a comment
```

Happy emulating! 🚀
