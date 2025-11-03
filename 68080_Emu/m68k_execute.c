#include "m68k_cpu.h"
#include <string.h>
#include <stdio.h>

// Get operand size from opcode
static OperandSize get_size(uint16_t opcode) {
    uint8_t size_bits = (opcode >> 6) & 0x3;
    switch (size_bits) {
        case 0: return SIZE_BYTE;
        case 1: return SIZE_WORD;
        case 2: return SIZE_LONG;
        default: return SIZE_WORD;
    }
}

// Execute decoded instruction
void m68k_execute_instruction(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, 
                              OpCode op, uint16_t opcode) {
    uint8_t reg_dst = (opcode >> 9) & 0x7;
    uint8_t reg_src = opcode & 0x7;
    OperandSize size = get_size(opcode);
    
    switch (op) {
        // === NOP ===
        case OP_NOP:
            break;
        
        // === MOVE ===
        case OP_MOVE: {
            uint32_t value = cpu->d[reg_src];
            cpu->d[reg_dst] = value;
            m68k_set_flags(cpu, value, size);
            break;
        }
        
        case OP_MOVEA: {
            cpu->a[reg_dst] = cpu->d[reg_src];
            break;
        }
        
        case OP_MOVEQ: {
            int8_t immediate = (int8_t)(opcode & 0xFF);
            cpu->d[reg_dst] = (int32_t)immediate;
            m68k_set_flags(cpu, cpu->d[reg_dst], SIZE_LONG);
            break;
        }
        
        // === ADD ===
        case OP_ADD: {
            uint32_t src = cpu->d[reg_src];
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst + src;
            cpu->d[reg_dst] = result;
            m68k_set_flags_add(cpu, src, dst, result, size);
            break;
        }
        
        case OP_ADDA: {
            cpu->a[reg_dst] += cpu->d[reg_src];
            break;
        }
        
        case OP_ADDI: {
            uint32_t immediate = (size == SIZE_LONG) ? 
                m68k_fetch_long(cpu, memory, mem_size) :
                m68k_fetch_word(cpu, memory, mem_size);
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst + immediate;
            cpu->d[reg_dst] = result;
            m68k_set_flags_add(cpu, immediate, dst, result, size);
            break;
        }
        
        case OP_ADDQ: {
            uint8_t quick = reg_dst ? reg_dst : 8;
            uint32_t dst = cpu->d[reg_src];
            uint32_t result = dst + quick;
            cpu->d[reg_src] = result;
            m68k_set_flags_add(cpu, quick, dst, result, size);
            break;
        }
        
        case OP_ADDX: {
            uint32_t src = cpu->d[reg_src];
            uint32_t dst = cpu->d[reg_dst];
            uint32_t extend = m68k_get_flag(cpu, SR_EXTEND) ? 1 : 0;
            uint32_t result = dst + src + extend;
            cpu->d[reg_dst] = result;
            m68k_set_flags_add(cpu, src + extend, dst, result, size);
            break;
        }
        
        // === SUB ===
        case OP_SUB: {
            uint32_t src = cpu->d[reg_src];
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst - src;
            cpu->d[reg_dst] = result;
            m68k_set_flags_sub(cpu, src, dst, result, size);
            break;
        }
        
        case OP_SUBA: {
            cpu->a[reg_dst] -= cpu->d[reg_src];
            break;
        }
        
        case OP_SUBI: {
            uint32_t immediate = (size == SIZE_LONG) ? 
                m68k_fetch_long(cpu, memory, mem_size) :
                m68k_fetch_word(cpu, memory, mem_size);
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst - immediate;
            cpu->d[reg_dst] = result;
            m68k_set_flags_sub(cpu, immediate, dst, result, size);
            break;
        }
        
        case OP_SUBQ: {
            uint8_t quick = reg_dst ? reg_dst : 8;
            uint32_t dst = cpu->d[reg_src];
            uint32_t result = dst - quick;
            cpu->d[reg_src] = result;
            m68k_set_flags_sub(cpu, quick, dst, result, size);
            break;
        }
        
        case OP_SUBX: {
            uint32_t src = cpu->d[reg_src];
            uint32_t dst = cpu->d[reg_dst];
            uint32_t extend = m68k_get_flag(cpu, SR_EXTEND) ? 1 : 0;
            uint32_t result = dst - src - extend;
            cpu->d[reg_dst] = result;
            m68k_set_flags_sub(cpu, src + extend, dst, result, size);
            break;
        }
        
        // === CMP ===
        case OP_CMP: {
            uint32_t src = cpu->d[reg_src];
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst - src;
            m68k_set_flags_sub(cpu, src, dst, result, size);
            break;
        }
        
        case OP_CMPA: {
            uint32_t src = cpu->d[reg_src];
            uint32_t dst = cpu->a[reg_dst];
            uint32_t result = dst - src;
            m68k_set_flags_sub(cpu, src, dst, result, SIZE_LONG);
            break;
        }
        
        case OP_CMPI: {
            uint32_t immediate = (size == SIZE_LONG) ? 
                m68k_fetch_long(cpu, memory, mem_size) :
                m68k_fetch_word(cpu, memory, mem_size);
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst - immediate;
            m68k_set_flags_sub(cpu, immediate, dst, result, size);
            break;
        }
        
        // === Logic Operations ===
        case OP_AND: {
            uint32_t src = cpu->d[reg_src];
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst & src;
            cpu->d[reg_dst] = result;
            m68k_set_flags(cpu, result, size);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            break;
        }
        
        case OP_ANDI: {
            uint32_t immediate = (size == SIZE_LONG) ? 
                m68k_fetch_long(cpu, memory, mem_size) :
                m68k_fetch_word(cpu, memory, mem_size);
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst & immediate;
            cpu->d[reg_dst] = result;
            m68k_set_flags(cpu, result, size);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            break;
        }
        
        case OP_OR: {
            uint32_t src = cpu->d[reg_src];
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst | src;
            cpu->d[reg_dst] = result;
            m68k_set_flags(cpu, result, size);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            break;
        }
        
        case OP_ORI: {
            uint32_t immediate = (size == SIZE_LONG) ? 
                m68k_fetch_long(cpu, memory, mem_size) :
                m68k_fetch_word(cpu, memory, mem_size);
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst | immediate;
            cpu->d[reg_dst] = result;
            m68k_set_flags(cpu, result, size);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            break;
        }
        
        case OP_EOR: {
            uint32_t src = cpu->d[reg_src];
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst ^ src;
            cpu->d[reg_dst] = result;
            m68k_set_flags(cpu, result, size);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            break;
        }
        
        case OP_EORI: {
            uint32_t immediate = (size == SIZE_LONG) ? 
                m68k_fetch_long(cpu, memory, mem_size) :
                m68k_fetch_word(cpu, memory, mem_size);
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = dst ^ immediate;
            cpu->d[reg_dst] = result;
            m68k_set_flags(cpu, result, size);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            break;
        }
        
        case OP_NOT: {
            uint32_t result = ~cpu->d[reg_dst];
            cpu->d[reg_dst] = result;
            m68k_set_flags(cpu, result, size);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            break;
        }
        
        // === NEG ===
        case OP_NEG: {
            uint32_t dst = cpu->d[reg_dst];
            uint32_t result = (uint32_t)(-(int32_t)dst);
            cpu->d[reg_dst] = result;
            m68k_set_flags_sub(cpu, dst, 0, result, size);
            break;
        }
        
        case OP_NEGX: {
            uint32_t dst = cpu->d[reg_dst];
            uint32_t extend = m68k_get_flag(cpu, SR_EXTEND) ? 1 : 0;
            uint32_t result = (uint32_t)(-(int32_t)dst - extend);
            cpu->d[reg_dst] = result;
            m68k_set_flags_sub(cpu, dst + extend, 0, result, size);
            break;
        }
        
        // === CLR ===
        case OP_CLR: {
            cpu->d[reg_dst] = 0;
            m68k_set_flag(cpu, SR_NEGATIVE, false);
            m68k_set_flag(cpu, SR_ZERO, true);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            m68k_set_flag(cpu, SR_CARRY, false);
            break;
        }
        
        // === TST ===
        case OP_TST: {
            m68k_set_flags(cpu, cpu->d[reg_dst], size);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            break;
        }
        
        // === MUL ===
        case OP_MULU: {
            uint16_t src = cpu->d[reg_src] & 0xFFFF;
            uint16_t dst = cpu->d[reg_dst] & 0xFFFF;
            uint32_t result = (uint32_t)src * (uint32_t)dst;
            cpu->d[reg_dst] = result;
            m68k_set_flags(cpu, result, SIZE_LONG);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            cpu->cycle_count += 70; // MUL is slow
            break;
        }
        
        case OP_MULS: {
            int16_t src = (int16_t)(cpu->d[reg_src] & 0xFFFF);
            int16_t dst = (int16_t)(cpu->d[reg_dst] & 0xFFFF);
            int32_t result = (int32_t)src * (int32_t)dst;
            cpu->d[reg_dst] = (uint32_t)result;
            m68k_set_flags(cpu, result, SIZE_LONG);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            cpu->cycle_count += 70;
            break;
        }
        
        // === DIV ===
        case OP_DIVU: {
            uint16_t src = cpu->d[reg_src] & 0xFFFF;
            uint32_t dst = cpu->d[reg_dst];
            if (src == 0) {
                // Division by zero trap
                printf("Division by zero!\n");
                cpu->halted = true;
            } else {
                uint16_t quotient = dst / src;
                uint16_t remainder = dst % src;
                cpu->d[reg_dst] = ((uint32_t)remainder << 16) | quotient;
                m68k_set_flags(cpu, quotient, SIZE_WORD);
                m68k_set_flag(cpu, SR_CARRY, false);
                m68k_set_flag(cpu, SR_OVERFLOW, false);
            }
            cpu->cycle_count += 140; // DIV is very slow
            break;
        }
        
        case OP_DIVS: {
            int16_t src = (int16_t)(cpu->d[reg_src] & 0xFFFF);
            int32_t dst = (int32_t)cpu->d[reg_dst];
            if (src == 0) {
                printf("Division by zero!\n");
                cpu->halted = true;
            } else {
                int16_t quotient = dst / src;
                int16_t remainder = dst % src;
                cpu->d[reg_dst] = ((uint32_t)(uint16_t)remainder << 16) | (uint16_t)quotient;
                m68k_set_flags(cpu, quotient, SIZE_WORD);
                m68k_set_flag(cpu, SR_CARRY, false);
                m68k_set_flag(cpu, SR_OVERFLOW, false);
            }
            cpu->cycle_count += 158;
            break;
        }
        
        // === Shifts and Rotates ===
        case OP_ASL:
        case OP_ASR:
        case OP_LSL:
        case OP_LSR:
        case OP_ROL:
        case OP_ROR:
        case OP_ROXL:
        case OP_ROXR: {
            uint8_t count = ((opcode >> 9) & 0x7);
            if (count == 0) count = 8;
            uint32_t value = cpu->d[reg_dst];
            
            for (int i = 0; i < count; i++) {
                bool carry = false;
                if (op == OP_ASL || op == OP_LSL || op == OP_ROL || op == OP_ROXL) {
                    carry = (value & 0x80000000) != 0;
                    value <<= 1;
                    if (op == OP_ROL) value |= carry ? 1 : 0;
                    if (op == OP_ROXL) value |= m68k_get_flag(cpu, SR_EXTEND) ? 1 : 0;
                } else {
                    carry = (value & 0x1) != 0;
                    if (op == OP_ASR) {
                        value = (int32_t)value >> 1;
                    } else {
                        value >>= 1;
                    }
                    if (op == OP_ROR) value |= carry ? 0x80000000 : 0;
                    if (op == OP_ROXR) value |= m68k_get_flag(cpu, SR_EXTEND) ? 0x80000000 : 0;
                }
                m68k_set_flag(cpu, SR_CARRY, carry);
                m68k_set_flag(cpu, SR_EXTEND, carry);
            }
            
            cpu->d[reg_dst] = value;
            m68k_set_flags(cpu, value, size);
            break;
        }
        
        // === VCROSS - Vector Cross Product (Apollo 68080) ===
        case OP_VCROSS: {
            if (!cpu->apollo_mode) break;
            // Simplified 3D cross product
            float* dst = (float*)&cpu->apollo.v[reg_dst];
            float* src1 = (float*)&cpu->apollo.v[reg_src];
            float* src2 = (float*)&cpu->apollo.v[((opcode >> 6) & 0xF)];
            dst[0] = src1[1] * src2[2] - src1[2] * src2[1];
            dst[1] = src1[2] * src2[0] - src1[0] * src2[2];
            dst[2] = src1[0] * src2[1] - src1[1] * src2[0];
            break;
        }
        
        default:
            // Call complete execution for any remaining instructions
            m68k_execute_complete(cpu, memory, mem_size, op, opcode);
            break;
    }
    
    cpu->instruction_count++;
}

// External declaration for complete executor
extern void m68k_execute_complete(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                                  OpCode op, uint16_t opcode);
