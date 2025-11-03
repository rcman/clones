#include "m68k_cpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Opcode name lookup table
static const char* opcode_names[] = {
    [OP_NOP] = "NOP",
    [OP_MOVE] = "MOVE", [OP_MOVEA] = "MOVEA", [OP_MOVEM] = "MOVEM",
    [OP_MOVEP] = "MOVEP", [OP_MOVEQ] = "MOVEQ",
    [OP_ADD] = "ADD", [OP_ADDA] = "ADDA", [OP_ADDI] = "ADDI",
    [OP_ADDQ] = "ADDQ", [OP_ADDX] = "ADDX",
    [OP_SUB] = "SUB", [OP_SUBA] = "SUBA", [OP_SUBI] = "SUBI",
    [OP_SUBQ] = "SUBQ", [OP_SUBX] = "SUBX",
    [OP_MULS] = "MULS", [OP_MULU] = "MULU",
    [OP_DIVS] = "DIVS", [OP_DIVU] = "DIVU",
    [OP_AND] = "AND", [OP_ANDI] = "ANDI",
    [OP_OR] = "OR", [OP_ORI] = "ORI",
    [OP_EOR] = "EOR", [OP_EORI] = "EORI",
    [OP_NOT] = "NOT", [OP_NEG] = "NEG", [OP_CLR] = "CLR",
    [OP_CMP] = "CMP", [OP_CMPA] = "CMPA", [OP_CMPI] = "CMPI",
    [OP_TST] = "TST",
    [OP_BRA] = "BRA", [OP_BSR] = "BSR",
    [OP_BEQ] = "BEQ", [OP_BNE] = "BNE",
    [OP_BCC] = "BCC", [OP_BCS] = "BCS",
    [OP_BGE] = "BGE", [OP_BLT] = "BLT",
    [OP_BGT] = "BGT", [OP_BLE] = "BLE",
    [OP_JMP] = "JMP", [OP_JSR] = "JSR", [OP_RTS] = "RTS",
    [OP_VADD] = "VADD", [OP_VSUB] = "VSUB",
    [OP_VMUL] = "VMUL", [OP_VDIV] = "VDIV",
    [OP_ILLEGAL] = "ILLEGAL",
};

const char* m68k_opcode_name(OpCode op) {
    if (op >= 0 && op < OP_COUNT && opcode_names[op]) {
        return opcode_names[op];
    }
    return "UNKNOWN";
}

void m68k_init(M68K_CPU* cpu) {
    memset(cpu, 0, sizeof(M68K_CPU));
    cpu->sr = SR_SUPERVISOR;
    cpu->ssp = 0x10000;
    cpu->apollo_mode = true;  // Enable Apollo 68080 by default
    
    // Allocate breakpoint array
    cpu->max_breakpoints = 32;
    cpu->breakpoints = (M68K_Breakpoint*)calloc(cpu->max_breakpoints, 
                                                sizeof(M68K_Breakpoint));
}

void m68k_reset(M68K_CPU* cpu) {
    uint64_t cycles = cpu->cycle_count;
    uint64_t instrs = cpu->instruction_count;
    bool apollo = cpu->apollo_mode;
    M68K_Breakpoint* bp = cpu->breakpoints;
    int num_bp = cpu->num_breakpoints;
    int max_bp = cpu->max_breakpoints;
    
    m68k_init(cpu);
    
    cpu->cycle_count = cycles;
    cpu->instruction_count = instrs;
    cpu->apollo_mode = apollo;
    cpu->breakpoints = bp;
    cpu->num_breakpoints = num_bp;
    cpu->max_breakpoints = max_bp;
}

bool m68k_get_flag(M68K_CPU* cpu, uint16_t flag) {
    return (cpu->sr & flag) != 0;
}

void m68k_set_flag(M68K_CPU* cpu, uint16_t flag, bool value) {
    if (value) {
        cpu->sr |= flag;
    } else {
        cpu->sr &= ~flag;
    }
}

void m68k_set_flags(M68K_CPU* cpu, uint32_t result, OperandSize size) {
    bool is_zero = false;
    bool is_neg = false;
    
    switch (size) {
        case SIZE_BYTE:
            is_zero = ((uint8_t)result == 0);
            is_neg = ((int8_t)result < 0);
            break;
        case SIZE_WORD:
            is_zero = ((uint16_t)result == 0);
            is_neg = ((int16_t)result < 0);
            break;
        case SIZE_LONG:
            is_zero = (result == 0);
            is_neg = ((int32_t)result < 0);
            break;
        default:
            break;
    }
    
    m68k_set_flag(cpu, SR_ZERO, is_zero);
    m68k_set_flag(cpu, SR_NEGATIVE, is_neg);
}

bool m68k_test_condition(M68K_CPU* cpu, uint8_t condition) {
    bool c = m68k_get_flag(cpu, SR_CARRY);
    bool v = m68k_get_flag(cpu, SR_OVERFLOW);
    bool z = m68k_get_flag(cpu, SR_ZERO);
    bool n = m68k_get_flag(cpu, SR_NEGATIVE);
    
    switch (condition) {
        case 0x0: return true;              // T (true)
        case 0x1: return false;             // F (false)
        case 0x2: return !c && !z;          // HI
        case 0x3: return c || z;            // LS
        case 0x4: return !c;                // CC/HS
        case 0x5: return c;                 // CS/LO
        case 0x6: return !z;                // NE
        case 0x7: return z;                 // EQ
        case 0x8: return !v;                // VC
        case 0x9: return v;                 // VS
        case 0xA: return !n;                // PL
        case 0xB: return n;                 // MI
        case 0xC: return (n && v) || (!n && !v);  // GE
        case 0xD: return (n && !v) || (!n && v);  // LT
        case 0xE: return !z && ((n && v) || (!n && !v));  // GT
        case 0xF: return z || (n && !v) || (!n && v);      // LE
        default: return false;
    }
}

// ============================================================================
// Breakpoint System
// ============================================================================

void m68k_add_breakpoint(M68K_CPU* cpu, uint32_t address) {
    if (cpu->num_breakpoints >= cpu->max_breakpoints) {
        cpu->max_breakpoints *= 2;
        cpu->breakpoints = (M68K_Breakpoint*)realloc(cpu->breakpoints,
                                cpu->max_breakpoints * sizeof(M68K_Breakpoint));
    }
    
    cpu->breakpoints[cpu->num_breakpoints].address = address;
    cpu->breakpoints[cpu->num_breakpoints].enabled = true;
    cpu->breakpoints[cpu->num_breakpoints].hit_count = 0;
    cpu->num_breakpoints++;
    
    printf("Breakpoint added at 0x%08X\n", address);
}

void m68k_remove_breakpoint(M68K_CPU* cpu, uint32_t address) {
    for (int i = 0; i < cpu->num_breakpoints; i++) {
        if (cpu->breakpoints[i].address == address) {
            // Shift remaining breakpoints
            for (int j = i; j < cpu->num_breakpoints - 1; j++) {
                cpu->breakpoints[j] = cpu->breakpoints[j + 1];
            }
            cpu->num_breakpoints--;
            printf("Breakpoint removed at 0x%08X\n", address);
            return;
        }
    }
}

void m68k_clear_breakpoints(M68K_CPU* cpu) {
    cpu->num_breakpoints = 0;
    printf("All breakpoints cleared\n");
}

bool m68k_check_breakpoint(M68K_CPU* cpu, uint32_t address) {
    for (int i = 0; i < cpu->num_breakpoints; i++) {
        if (cpu->breakpoints[i].enabled && 
            cpu->breakpoints[i].address == address) {
            cpu->breakpoints[i].hit_count++;
            cpu->breakpoint_hit = true;
            cpu->breakpoint_addr = address;
            return true;
        }
    }
    return false;
}

void m68k_enable_breakpoint(M68K_CPU* cpu, uint32_t address, bool enabled) {
    for (int i = 0; i < cpu->num_breakpoints; i++) {
        if (cpu->breakpoints[i].address == address) {
            cpu->breakpoints[i].enabled = enabled;
            return;
        }
    }
}

// ============================================================================
// Disassembler
// ============================================================================

void m68k_disassemble(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                     uint32_t address, M68K_Disassembly* dis) {
    memset(dis, 0, sizeof(M68K_Disassembly));
    dis->address = address;
    
    if (address >= mem_size - 1) {
        strcpy(dis->mnemonic, "???");
        strcpy(dis->operands, "");
        return;
    }
    
    uint16_t opcode = (memory[address] << 8) | memory[address + 1];
    dis->opcode = opcode;
    dis->next_pc = address + 2;
    
    // Decode instruction
    OpCode op;
    uint8_t params[8] = {0};
    m68k_decode_instruction(cpu, opcode, &op, params);
    dis->op = op;
    
    // Format mnemonic and operands
    strncpy(dis->mnemonic, m68k_opcode_name(op), sizeof(dis->mnemonic) - 1);
    
    // Simplified operand formatting
    switch (op) {
        case OP_MOVEQ:
            snprintf(dis->operands, sizeof(dis->operands), 
                    "#$%02X, D%d", params[0], (opcode >> 9) & 7);
            break;
        case OP_ADD:
        case OP_SUB:
            snprintf(dis->operands, sizeof(dis->operands), 
                    "D%d, D%d", opcode & 7, (opcode >> 9) & 7);
            break;
        case OP_BRA:
        case OP_BEQ:
        case OP_BNE:
            snprintf(dis->operands, sizeof(dis->operands), 
                    "$%08X", address + 2 + (int8_t)params[0]);
            break;
        case OP_JMP:
        case OP_JSR:
            strcpy(dis->operands, "(A0)");
            break;
        default:
            strcpy(dis->operands, "");
            break;
    }
}

// ============================================================================
// State Save/Load
// ============================================================================

bool m68k_save_state(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, 
                    const char* filename) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("Failed to save state to %s\n", filename);
        return false;
    }
    
    // Write header
    uint32_t magic = 0x68000000;
    uint32_t version = 1;
    fwrite(&magic, sizeof(uint32_t), 1, f);
    fwrite(&version, sizeof(uint32_t), 1, f);
    fwrite(&mem_size, sizeof(size_t), 1, f);
    
    // Write CPU state
    fwrite(cpu, sizeof(M68K_CPU), 1, f);
    
    // Write memory
    fwrite(memory, 1, mem_size, f);
    
    fclose(f);
    printf("State saved to %s\n", filename);
    return true;
}

bool m68k_load_state(M68K_CPU* cpu, uint8_t** memory, size_t* mem_size,
                    const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("Failed to load state from %s\n", filename);
        return false;
    }
    
    // Read header
    uint32_t magic, version;
    size_t saved_mem_size;
    fread(&magic, sizeof(uint32_t), 1, f);
    fread(&version, sizeof(uint32_t), 1, f);
    fread(&saved_mem_size, sizeof(size_t), 1, f);
    
    if (magic != 0x68000000) {
        printf("Invalid save file format\n");
        fclose(f);
        return false;
    }
    
    // Read CPU state
    fread(cpu, sizeof(M68K_CPU), 1, f);
    
    // Reallocate memory if needed
    if (saved_mem_size != *mem_size) {
        *memory = (uint8_t*)realloc(*memory, saved_mem_size);
        *mem_size = saved_mem_size;
    }
    
    // Read memory
    fread(*memory, 1, saved_mem_size, f);
    
    fclose(f);
    printf("State loaded from %s\n", filename);
    return true;
}

// ============================================================================
// Apollo 68080 Extensions
// ============================================================================

void m68k_enable_apollo(M68K_CPU* cpu, bool enable) {
    cpu->apollo_mode = enable;
    printf("Apollo 68080 mode: %s\n", enable ? "enabled" : "disabled");
}

void m68k_vector_add(M68K_CPU* cpu, int dst, int src1, int src2) {
    if (!cpu->apollo_mode || dst >= 16 || src1 >= 16 || src2 >= 16) return;
    
    // Treat as 8x 64-bit values
    for (int i = 0; i < 8; i++) {
        uint64_t* v_dst = (uint64_t*)&cpu->apollo.v[dst];
        uint64_t* v_src1 = (uint64_t*)&cpu->apollo.v[src1];
        uint64_t* v_src2 = (uint64_t*)&cpu->apollo.v[src2];
        v_dst[i] = v_src1[i] + v_src2[i];
    }
}

void m68k_vector_mul(M68K_CPU* cpu, int dst, int src1, int src2) {
    if (!cpu->apollo_mode || dst >= 16 || src1 >= 16 || src2 >= 16) return;
    
    // Treat as 8x 64-bit values
    for (int i = 0; i < 8; i++) {
        uint64_t* v_dst = (uint64_t*)&cpu->apollo.v[dst];
        uint64_t* v_src1 = (uint64_t*)&cpu->apollo.v[src1];
        uint64_t* v_src2 = (uint64_t*)&cpu->apollo.v[src2];
        v_dst[i] = v_src1[i] * v_src2[i];
    }
}

float m68k_vector_dot(M68K_CPU* cpu, int src1, int src2) {
    if (!cpu->apollo_mode || src1 >= 16 || src2 >= 16) return 0.0f;
    
    float* f1 = (float*)&cpu->apollo.v[src1];
    float* f2 = (float*)&cpu->apollo.v[src2];
    float result = 0.0f;
    
    for (int i = 0; i < 16; i++) {  // 16 floats in 512 bits
        result += f1[i] * f2[i];
    }
    
    return result;
}

void m68k_flush_caches(M68K_CPU* cpu) {
    memset(cpu->icache_valid, 0, sizeof(cpu->icache_valid));
    memset(cpu->dcache_valid, 0, sizeof(cpu->dcache_valid));
}
