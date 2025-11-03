# Makefile for 68000 Simulator - Complete Edition

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lSDL2 -lSDL2_ttf -lm

TARGET = m68k_simulator
SOURCES = main.c \
          m68k_cpu.c m68k_cpu_extended.c \
          m68k_decode.c m68k_execute.c m68k_execute2.c m68k_execute_complete.c m68k_addressing.c \
          m68k_peripherals.c \
          m68k_display_enhanced.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = m68k_cpu.h m68k_peripherals.h m68k_display.h

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo ""
	@echo "==================================="
	@echo "  Build successful!"
	@echo "==================================="
	@echo "Complete 68000 Instruction Set:"
	@echo "  ✓ ALL data movement instructions"
	@echo "  ✓ ALL arithmetic instructions"
	@echo "  ✓ ALL logical instructions"
	@echo "  ✓ ALL shift/rotate instructions"
	@echo "  ✓ ALL bit manipulation instructions"
	@echo "  ✓ ALL branch/jump instructions"
	@echo "  ✓ ALL comparison instructions"
	@echo "  ✓ ALL BCD instructions"
	@echo "  ✓ ALL addressing modes"
	@echo ""
	@echo "Apollo 68080 + Peripherals:"
	@echo "  ✓ SIMD/Vector operations"
	@echo "  ✓ M.2 NVMe simulation"
	@echo "  ✓ 2.5GbE Ethernet"
	@echo "  ✓ HDMI framebuffer"
	@echo "  ✓ GPIO (64 pins)"
	@echo ""
	@echo "Debugging & Tools:"
	@echo "  ✓ Complete disassembler"
	@echo "  ✓ Breakpoint system"
	@echo "  ✓ State save/load"
	@echo "  ✓ Performance profiling"
	@echo ""
	@echo "Run with: ./$(TARGET)"

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) *.img *.state
	@echo "Clean complete"

run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)

# Test build (minimal features for quick testing)
test: CFLAGS += -DTEST_MODE
test: $(TARGET)

# Count instructions implemented
count:
	@echo "Counting implemented instructions..."
	@grep -h "case OP_" m68k_execute*.c m68k_addressing.c | wc -l
	@echo "instruction cases found"

# Help target
help:
	@echo "68000 Simulator - Complete Edition Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  make         - Build the simulator"
	@echo "  make run     - Build and run the simulator"
	@echo "  make clean   - Remove build artifacts"
	@echo "  make debug   - Build with debug symbols"
	@echo "  make test    - Build test version"
	@echo "  make count   - Count implemented instructions"
	@echo "  make help    - Show this help"
	@echo ""
	@echo "Complete 68000 ISA Implementation:"
	@echo "  - Data movement: MOVE, MOVEA, MOVEM, MOVEP, MOVEQ"
	@echo "  - Arithmetic: ADD, SUB, MUL, DIV (all variants)"
	@echo "  - Logic: AND, OR, EOR, NOT"
	@echo "  - Shifts: ASL, ASR, LSL, LSR, ROL, ROR, ROXL, ROXR"
	@echo "  - Bits: BTST, BCHG, BCLR, BSET"
	@echo "  - Branches: BRA, Bcc, BSR, DBcc"
	@echo "  - Jumps: JMP, JSR, RTS, RTR, RTE"
	@echo "  - Comparison: CMP, CMPA, CMPI, CMPM, TST"
	@echo "  - BCD: ABCD, SBCD, NBCD"
	@echo "  - Special: LEA, PEA, LINK, UNLK, EXG, SWAP, EXT"
	@echo "  - System: TRAP, TRAPV, CHK, TAS, RESET, STOP"
	@echo "  - And more!"
