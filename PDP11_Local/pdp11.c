#include "pdp11.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Initialize CPU */
pdp11_t* pdp11_init(uint32_t memory_size) {
    pdp11_t *cpu = calloc(1, sizeof(pdp11_t));
    if (!cpu) return NULL;
    
    if (memory_size > MAX_MEMORY) {
        memory_size = MAX_MEMORY;
    }
    
    cpu->memory = calloc(1, memory_size);
    if (!cpu->memory) {
        free(cpu);
        return NULL;
    }
    
    cpu->memory_size = memory_size;
    pdp11_reset(cpu);
    
    return cpu;
}

/* Free CPU resources */
void pdp11_free(pdp11_t *cpu) {
    if (cpu) {
        if (cpu->memory) free(cpu->memory);
        free(cpu);
    }
}

/* Reset CPU to initial state */
void pdp11_reset(pdp11_t *cpu) {
    memset(cpu->regs, 0, sizeof(cpu->regs));
    cpu->psw = 0;
    cpu->halted = false;
    cpu->wait_state = false;
    cpu->instr_count = 0;
    cpu->regs[REG_PC] = 0;  // Start at address 0
}

/* Memory read/write functions */
uint16_t mem_read_word(pdp11_t *cpu, uint16_t addr) {
    if (addr >= cpu->memory_size - 1) {
        return 0;  // Out of bounds
    }
    // PDP-11 is little-endian
    return cpu->memory[addr] | (cpu->memory[addr + 1] << 8);
}

uint8_t mem_read_byte(pdp11_t *cpu, uint16_t addr) {
    if (addr >= cpu->memory_size) {
        return 0;
    }
    return cpu->memory[addr];
}

void mem_write_word(pdp11_t *cpu, uint16_t addr, uint16_t value) {
    if (addr >= cpu->memory_size - 1) {
        return;  // Out of bounds
    }
    cpu->memory[addr] = value & 0xFF;
    cpu->memory[addr + 1] = (value >> 8) & 0xFF;
}

void mem_write_byte(pdp11_t *cpu, uint16_t addr, uint8_t value) {
    if (addr >= cpu->memory_size) {
        return;
    }
    cpu->memory[addr] = value;
}

/* Load data into memory */
void pdp11_load_memory(pdp11_t *cpu, uint16_t addr, const uint8_t *data, size_t len) {
    if (addr + len > cpu->memory_size) {
        len = cpu->memory_size - addr;
    }
    memcpy(&cpu->memory[addr], data, len);
}

/* Condition code helpers */
void set_cc_byte(pdp11_t *cpu, uint8_t result) {
    cpu->psw &= ~(PSW_N | PSW_Z | PSW_V);
    if (result == 0) cpu->psw |= PSW_Z;
    if (result & 0x80) cpu->psw |= PSW_N;
}

void set_cc_word(pdp11_t *cpu, uint16_t result) {
    cpu->psw &= ~(PSW_N | PSW_Z | PSW_V);
    if (result == 0) cpu->psw |= PSW_Z;
    if (result & 0x8000) cpu->psw |= PSW_N;
}

void set_cc_nz(pdp11_t *cpu, uint16_t result) {
    cpu->psw &= ~(PSW_N | PSW_Z);
    if (result == 0) cpu->psw |= PSW_Z;
    if (result & 0x8000) cpu->psw |= PSW_N;
}

/* Decode addressing mode and get effective address or value */
static uint16_t decode_operand(pdp11_t *cpu, uint16_t mode, uint16_t reg, bool is_byte, bool write_mode, uint16_t *ea) {
    uint16_t addr, index;
    (void)write_mode;  // Reserved for future use
    
    switch (mode) {
        case MODE_REG:  // Register
            *ea = 0;  // No effective address for register mode
            return cpu->regs[reg];
            
        case MODE_REG_DEF:  // Register Deferred: (Rn)
            addr = cpu->regs[reg];
            *ea = addr;
            return is_byte ? mem_read_byte(cpu, addr) : mem_read_word(cpu, addr);
            
        case MODE_AUTOINC:  // Autoincrement: (Rn)+
            addr = cpu->regs[reg];
            *ea = addr;
            if (reg == REG_PC || !is_byte) {
                cpu->regs[reg] += 2;
            } else {
                cpu->regs[reg] += 1;
            }
            return is_byte ? mem_read_byte(cpu, addr) : mem_read_word(cpu, addr);
            
        case MODE_AUTOINC_DEF:  // Autoincrement Deferred: @(Rn)+
            addr = mem_read_word(cpu, cpu->regs[reg]);
            cpu->regs[reg] += 2;
            *ea = addr;
            return is_byte ? mem_read_byte(cpu, addr) : mem_read_word(cpu, addr);
            
        case MODE_AUTODEC:  // Autodecrement: -(Rn)
            if (reg == REG_PC || !is_byte) {
                cpu->regs[reg] -= 2;
            } else {
                cpu->regs[reg] -= 1;
            }
            addr = cpu->regs[reg];
            *ea = addr;
            return is_byte ? mem_read_byte(cpu, addr) : mem_read_word(cpu, addr);
            
        case MODE_AUTODEC_DEF:  // Autodecrement Deferred: @-(Rn)
            cpu->regs[reg] -= 2;
            addr = mem_read_word(cpu, cpu->regs[reg]);
            *ea = addr;
            return is_byte ? mem_read_byte(cpu, addr) : mem_read_word(cpu, addr);
            
        case MODE_INDEX:  // Index: X(Rn)
            index = mem_read_word(cpu, cpu->regs[REG_PC]);
            cpu->regs[REG_PC] += 2;
            addr = (cpu->regs[reg] + index) & 0xFFFF;
            *ea = addr;
            return is_byte ? mem_read_byte(cpu, addr) : mem_read_word(cpu, addr);
            
        case MODE_INDEX_DEF:  // Index Deferred: @X(Rn)
            index = mem_read_word(cpu, cpu->regs[REG_PC]);
            cpu->regs[REG_PC] += 2;
            addr = mem_read_word(cpu, (cpu->regs[reg] + index) & 0xFFFF);
            *ea = addr;
            return is_byte ? mem_read_byte(cpu, addr) : mem_read_word(cpu, addr);
    }
    
    return 0;
}

/* Write operand back */
static void write_operand(pdp11_t *cpu, uint16_t mode, uint16_t reg, bool is_byte, uint16_t ea, uint16_t value) {
    if (mode == MODE_REG) {
        if (is_byte) {
            cpu->regs[reg] = (cpu->regs[reg] & 0xFF00) | (value & 0xFF);
        } else {
            cpu->regs[reg] = value;
        }
    } else {
        if (is_byte) {
            mem_write_byte(cpu, ea, value & 0xFF);
        } else {
            mem_write_word(cpu, ea, value);
        }
    }
}

/* Execute one instruction */
int pdp11_step(pdp11_t *cpu) {
    if (cpu->halted || cpu->wait_state) {
        return 0;
    }
    
    uint16_t pc = cpu->regs[REG_PC];
    uint16_t instr = mem_read_word(cpu, pc);
    cpu->regs[REG_PC] += 2;
    cpu->instr_count++;
    
    // Decode instruction (opcode used for debugging)
    // uint16_t opcode = instr >> 12;
    
    // Double operand instructions (MOV, CMP, BIT, BIC, BIS, ADD, SUB)
    if ((instr & 0170000) == 0010000 || (instr & 0170000) == 0020000 ||
        (instr & 0170000) == 0030000 || (instr & 0170000) == 0040000 ||
        (instr & 0170000) == 0050000 || (instr & 0170000) == 0060000) {
        
        bool is_byte = (instr & 0100000) != 0;
        uint16_t src_mode = (instr >> 9) & 7;
        uint16_t src_reg = (instr >> 6) & 7;
        uint16_t dst_mode = (instr >> 3) & 7;
        uint16_t dst_reg = instr & 7;
        
        uint16_t src_ea, dst_ea;
        uint16_t src = decode_operand(cpu, src_mode, src_reg, is_byte, false, &src_ea);
        uint16_t dst = decode_operand(cpu, dst_mode, dst_reg, is_byte, true, &dst_ea);
        
        if (is_byte) {
            src &= 0xFF;
            dst &= 0xFF;
        }
        
        uint16_t result;
        uint16_t base_op = (instr >> 12) & 7;
        
        switch (base_op) {
            case 1:  // MOV
                result = src;
                set_cc_nz(cpu, result);
                cpu->psw &= ~PSW_V;
                write_operand(cpu, dst_mode, dst_reg, is_byte, dst_ea, result);
                break;
                
            case 2:  // CMP
                result = dst - src;
                if (is_byte) {
                    set_cc_byte(cpu, result & 0xFF);
                } else {
                    set_cc_word(cpu, result);
                }
                // Set carry if borrow occurred
                if (is_byte) {
                    if ((dst & 0xFF) < (src & 0xFF)) cpu->psw |= PSW_C;
                    else cpu->psw &= ~PSW_C;
                } else {
                    if (dst < src) cpu->psw |= PSW_C;
                    else cpu->psw &= ~PSW_C;
                }
                break;
                
            case 3:  // BIT
                result = dst & src;
                set_cc_nz(cpu, result);
                cpu->psw &= ~PSW_V;
                break;
                
            case 4:  // BIC
                result = dst & ~src;
                set_cc_nz(cpu, result);
                cpu->psw &= ~PSW_V;
                write_operand(cpu, dst_mode, dst_reg, is_byte, dst_ea, result);
                break;
                
            case 5:  // BIS
                result = dst | src;
                set_cc_nz(cpu, result);
                cpu->psw &= ~PSW_V;
                write_operand(cpu, dst_mode, dst_reg, is_byte, dst_ea, result);
                break;
                
            case 6:  // ADD
                result = dst + src;
                set_cc_word(cpu, result);
                // Set overflow if signs were same but result differs
                if (((dst ^ result) & (src ^ result) & 0x8000)) cpu->psw |= PSW_V;
                else cpu->psw &= ~PSW_V;
                // Set carry
                if (result < dst) cpu->psw |= PSW_C;
                else cpu->psw &= ~PSW_C;
                write_operand(cpu, dst_mode, dst_reg, is_byte, dst_ea, result);
                break;
        }
        
        return 1;
    }
    
    // Branch instructions
    if ((instr & 0177400) == 0000400) {  // BR
        int8_t offset = (int8_t)(instr & 0xFF);
        cpu->regs[REG_PC] = (cpu->regs[REG_PC] + offset * 2) & 0xFFFF;
        return 1;
    }
    
    if ((instr & 0177400) == 0001000) {  // BNE
        if (!(cpu->psw & PSW_Z)) {
            int8_t offset = (int8_t)(instr & 0xFF);
            cpu->regs[REG_PC] = (cpu->regs[REG_PC] + offset * 2) & 0xFFFF;
        }
        return 1;
    }
    
    if ((instr & 0177400) == 0001400) {  // BEQ
        if (cpu->psw & PSW_Z) {
            int8_t offset = (int8_t)(instr & 0xFF);
            cpu->regs[REG_PC] = (cpu->regs[REG_PC] + offset * 2) & 0xFFFF;
        }
        return 1;
    }
    
    // Single operand instructions
    if ((instr & 0177000) == 0005000) {  // CLR
        bool is_byte = (instr & 0100000) != 0;
        uint16_t mode = (instr >> 3) & 7;
        uint16_t reg = instr & 7;
        uint16_t ea;
        
        decode_operand(cpu, mode, reg, is_byte, true, &ea);
        write_operand(cpu, mode, reg, is_byte, ea, 0);
        
        cpu->psw &= ~(PSW_N | PSW_V | PSW_C);
        cpu->psw |= PSW_Z;
        return 1;
    }
    
    // HALT
    if (instr == 0000000) {
        cpu->halted = true;
        return 0;
    }
    
    // NOP
    if (instr == 0000240) {
        return 1;
    }
    
    // Unknown instruction
    fprintf(stderr, "Unknown instruction: %06o at PC=%06o\n", instr, pc);
    return 1;
}
