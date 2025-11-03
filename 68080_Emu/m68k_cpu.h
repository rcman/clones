#ifndef M68K_CPU_H
#define M68K_CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Forward declarations
typedef struct M68K_CPU M68K_CPU;
typedef struct M68K_Breakpoint M68K_Breakpoint;
typedef struct M68K_State M68K_State;

// Apollo 68080 Extended Registers
typedef struct {
    uint64_t v[16];         // Vector registers V0-V15 (512-bit SIMD)
    uint32_t vr[8];         // Vector control registers
    double fp[8];           // Double precision FPU registers
    uint32_t fpcr;          // FP control register
    uint32_t fpsr;          // FP status register
    uint32_t fpiar;         // FP instruction address register
} Apollo_Regs;

// 68000 CPU Registers
struct M68K_CPU {
    // Standard 68000 registers
    uint32_t d[8];          // Data registers D0-D7
    uint32_t a[8];          // Address registers A0-A7 (A7 is stack pointer)
    uint32_t pc;            // Program counter
    uint16_t sr;            // Status register
    uint32_t usp;           // User stack pointer
    uint32_t ssp;           // Supervisor stack pointer
    
    // Apollo 68080 extensions
    Apollo_Regs apollo;
    bool apollo_mode;       // Enable Apollo 68080 features
    
    // Internal state
    bool halted;
    bool trace_mode;
    uint64_t cycle_count;
    uint64_t instruction_count;
    
    // Bus state for 1024-bit simulation
    uint8_t bus_data[128];  // 1024 bits = 128 bytes
    uint32_t bus_address;
    bool bus_active;
    uint32_t bus_transfers;
    
    // Cache simulation (Apollo 68080 style)
    uint8_t icache[32768];  // 32KB instruction cache
    uint8_t dcache[32768];  // 32KB data cache
    bool icache_valid[32768/128];
    bool dcache_valid[32768/128];
    uint64_t cache_hits;
    uint64_t cache_misses;
    
    // Breakpoints
    M68K_Breakpoint* breakpoints;
    int num_breakpoints;
    int max_breakpoints;
    bool breakpoint_hit;
    uint32_t breakpoint_addr;
    
    // Exception vectors
    uint32_t vbr;           // Vector Base Register (68010+)
    
    // Performance counters
    uint64_t branch_count;
    uint64_t branch_taken;
    uint64_t load_count;
    uint64_t store_count;
};

// Status Register Flags
#define SR_CARRY      0x0001
#define SR_OVERFLOW   0x0002
#define SR_ZERO       0x0004
#define SR_NEGATIVE   0x0008
#define SR_EXTEND     0x0010
#define SR_INTERRUPT  0x0700  // Interrupt mask (bits 8-10)
#define SR_SUPERVISOR 0x2000
#define SR_TRACE      0x8000

// Breakpoint structure
struct M68K_Breakpoint {
    uint32_t address;
    bool enabled;
    uint64_t hit_count;
    char condition[64];
};

// CPU state for save/load
struct M68K_State {
    M68K_CPU cpu;
    uint32_t memory_size;
    // Memory is saved separately
};

// Complete 68000 Instruction Set
typedef enum {
    // Data Movement
    OP_NOP,
    OP_MOVE, OP_MOVEA, OP_MOVEM, OP_MOVEP, OP_MOVEQ,
    OP_EXG, OP_SWAP, OP_LEA, OP_PEA,
    OP_LINK, OP_UNLK,
    
    // Arithmetic
    OP_ADD, OP_ADDA, OP_ADDI, OP_ADDQ, OP_ADDX,
    OP_SUB, OP_SUBA, OP_SUBI, OP_SUBQ, OP_SUBX,
    OP_MULS, OP_MULU, OP_DIVS, OP_DIVU,
    OP_NEG, OP_NEGX, OP_CLR, OP_EXT,
    
    // Logic
    OP_AND, OP_ANDI, OP_OR, OP_ORI, OP_EOR, OP_EORI,
    OP_NOT,
    
    // Shifts and Rotates
    OP_ASL, OP_ASR, OP_LSL, OP_LSR,
    OP_ROL, OP_ROR, OP_ROXL, OP_ROXR,
    
    // Bit Manipulation
    OP_BCHG, OP_BCLR, OP_BSET, OP_BTST,
    
    // Comparison
    OP_CMP, OP_CMPA, OP_CMPI, OP_CMPM,
    OP_TST,
    
    // Branches (conditional)
    OP_BRA, OP_BSR,
    OP_BCC, OP_BCS, OP_BEQ, OP_BNE,
    OP_BGE, OP_BGT, OP_BHI, OP_BLE,
    OP_BLS, OP_BLT, OP_BMI, OP_BPL,
    OP_BVC, OP_BVS,
    
    // Conditional Set
    OP_SCC, OP_SCS, OP_SEQ, OP_SNE,
    OP_SGE, OP_SGT, OP_SHI, OP_SLE,
    OP_SLS, OP_SLT, OP_SMI, OP_SPL,
    OP_SVC, OP_SVS, OP_ST, OP_SF,
    
    // Jumps
    OP_JMP, OP_JSR, OP_RTS, OP_RTR, OP_RTE,
    
    // Decrement and Branch
    OP_DBCC, OP_DBCS, OP_DBEQ, OP_DBNE,
    OP_DBGE, OP_DBGT, OP_DBHI, OP_DBLE,
    OP_DBLS, OP_DBLT, OP_DBMI, OP_DBPL,
    OP_DBVC, OP_DBVS, OP_DBT, OP_DBF,
    
    // System Control
    OP_TRAP, OP_TRAPV, OP_CHK,
    OP_RESET, OP_STOP,
    OP_ANDI_SR, OP_EORI_SR, OP_ORI_SR,
    OP_ANDI_CCR, OP_EORI_CCR, OP_ORI_CCR,
    OP_MOVE_SR, OP_MOVE_CCR, OP_MOVE_USP,
    
    // Special
    OP_TAS, OP_NBCD, OP_ABCD, OP_SBCD,
    
    // 68010+ additions
    OP_MOVEC, OP_MOVES, OP_RTD, OP_BKPT,
    
    // 68020+ additions
    OP_BFCHG, OP_BFCLR, OP_BFEXTS, OP_BFEXTU,
    OP_BFFFO, OP_BFINS, OP_BFSET, OP_BFTST,
    OP_CALLM, OP_CAS, OP_CAS2, OP_CMP2,
    OP_DIVSL, OP_DIVUL, OP_EXTB,
    OP_PACK, OP_UNPK,
    
    // Apollo 68080 SIMD/Vector (AMMX)
    OP_VADD, OP_VSUB, OP_VMUL, OP_VDIV,
    OP_VAND, OP_VOR, OP_VXOR, OP_VNOT,
    OP_VLOAD, OP_VSTORE, OP_VMOVE,
    OP_VDOT, OP_VCROSS, OP_VABS, OP_VSQRT,
    OP_VMIN, OP_VMAX, OP_VSUM,
    
    // Apollo 68080 64-bit operations
    OP_ADD64, OP_SUB64, OP_MUL64, OP_DIV64,
    OP_MOVE64, OP_CMP64,
    
    // FPU operations (68881/68882)
    OP_FMOVE, OP_FADD, OP_FSUB, OP_FMUL, OP_FDIV,
    OP_FSQRT, OP_FABS, OP_FNEG, OP_FCMP, OP_FTST,
    OP_FSIN, OP_FCOS, OP_FTAN,
    
    OP_ILLEGAL,
    OP_COUNT
} OpCode;

// Addressing modes
typedef enum {
    MODE_DATA_REG,
    MODE_ADDR_REG,
    MODE_ADDR_INDIRECT,
    MODE_ADDR_POSTINC,
    MODE_ADDR_PREDEC,
    MODE_ADDR_DISP,
    MODE_ADDR_INDEX,
    MODE_IMMEDIATE,
    MODE_PC_DISP,
    MODE_PC_INDEX,
    MODE_ABSOLUTE_SHORT,
    MODE_ABSOLUTE_LONG,
} AddressingMode;

// Size specifiers
typedef enum {
    SIZE_BYTE = 1,
    SIZE_WORD = 2,
    SIZE_LONG = 4,
    SIZE_QUAD = 8,      // Apollo 68080
    SIZE_VECTOR = 64,   // Apollo 68080 SIMD
} OperandSize;

// Disassembly structure
typedef struct {
    uint32_t address;
    uint16_t opcode;
    OpCode op;
    char mnemonic[16];
    char operands[64];
    uint8_t size;
    uint32_t next_pc;
} M68K_Disassembly;

// Core CPU functions
void m68k_init(M68K_CPU* cpu);
void m68k_reset(M68K_CPU* cpu);
void m68k_execute_cycle(M68K_CPU* cpu, uint8_t* memory, size_t mem_size);
uint16_t m68k_fetch_word(M68K_CPU* cpu, uint8_t* memory, size_t mem_size);
uint32_t m68k_fetch_long(M68K_CPU* cpu, uint8_t* memory, size_t mem_size);
void m68k_decode_instruction(M68K_CPU* cpu, uint16_t opcode, OpCode* op, uint8_t* params);
void m68k_set_flags(M68K_CPU* cpu, uint32_t result, OperandSize size);
void m68k_set_flags_add(M68K_CPU* cpu, uint32_t src, uint32_t dst, uint32_t result, OperandSize size);
void m68k_set_flags_sub(M68K_CPU* cpu, uint32_t src, uint32_t dst, uint32_t result, OperandSize size);

// Instruction execution
void m68k_execute_instruction(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, 
                              OpCode op, uint16_t opcode);
void m68k_execute_branches(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, 
                           OpCode op, uint16_t opcode);
void m68k_execute_special(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                         OpCode op, uint16_t opcode);
void m68k_execute_bit_ops(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                          OpCode op, uint16_t opcode);
void m68k_execute_bcd(M68K_CPU* cpu, OpCode op, uint16_t opcode);
void m68k_execute_movem(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, uint16_t opcode);
void m68k_execute_movep(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, uint16_t opcode);
void m68k_execute_cmpm(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, uint16_t opcode);
void m68k_execute_complete(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                           OpCode op, uint16_t opcode);

// Addressing modes
uint32_t m68k_get_ea_value(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                           uint8_t mode, uint8_t reg, OperandSize size);
void m68k_set_ea_value(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                       uint8_t mode, uint8_t reg, uint32_t value, OperandSize size);

// Status register helpers
bool m68k_get_flag(M68K_CPU* cpu, uint16_t flag);
void m68k_set_flag(M68K_CPU* cpu, uint16_t flag, bool value);
bool m68k_test_condition(M68K_CPU* cpu, uint8_t condition);

// Breakpoint system
void m68k_add_breakpoint(M68K_CPU* cpu, uint32_t address);
void m68k_remove_breakpoint(M68K_CPU* cpu, uint32_t address);
void m68k_clear_breakpoints(M68K_CPU* cpu);
bool m68k_check_breakpoint(M68K_CPU* cpu, uint32_t address);
void m68k_enable_breakpoint(M68K_CPU* cpu, uint32_t address, bool enabled);

// Disassembler
void m68k_disassemble(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, 
                     uint32_t address, M68K_Disassembly* dis);
const char* m68k_opcode_name(OpCode op);

// State save/load
bool m68k_save_state(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, const char* filename);
bool m68k_load_state(M68K_CPU* cpu, uint8_t** memory, size_t* mem_size, const char* filename);

// Apollo 68080 specific
void m68k_enable_apollo(M68K_CPU* cpu, bool enable);
void m68k_vector_add(M68K_CPU* cpu, int dst, int src1, int src2);
void m68k_vector_mul(M68K_CPU* cpu, int dst, int src1, int src2);
float m68k_vector_dot(M68K_CPU* cpu, int src1, int src2);

// Memory operations with cache simulation
uint32_t m68k_read_memory(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, 
                         uint32_t addr, OperandSize size);
void m68k_write_memory(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, 
                      uint32_t addr, uint32_t value, OperandSize size);
void m68k_flush_caches(M68K_CPU* cpu);

#endif // M68K_CPU_H
