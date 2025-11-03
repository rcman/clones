#include "m68k_cpu.h"
#include <string.h>
#include <stdio.h>

// Execute bit manipulation instructions
void m68k_execute_bit_ops(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                          OpCode op, uint16_t opcode) {
    uint8_t bit_num;
    uint8_t reg_dst = opcode & 0x7;
    uint8_t reg_src = (opcode >> 9) & 0x7;
    
    // Get bit number
    if ((opcode & 0x0100) == 0) {
        // Immediate bit number
        bit_num = m68k_fetch_word(cpu, memory, mem_size) & 0xFF;
    } else {
        // Register bit number
        bit_num = cpu->d[reg_src] & 0x1F;
    }
    
    uint32_t value = cpu->d[reg_dst];
    uint32_t bit_mask = 1 << bit_num;
    
    switch (op) {
        // === BTST - Bit Test ===
        case OP_BTST: {
            bool bit_set = (value & bit_mask) != 0;
            m68k_set_flag(cpu, SR_ZERO, !bit_set);
            break;
        }
        
        // === BCHG - Bit Test and Change ===
        case OP_BCHG: {
            bool bit_set = (value & bit_mask) != 0;
            m68k_set_flag(cpu, SR_ZERO, !bit_set);
            cpu->d[reg_dst] = value ^ bit_mask;
            break;
        }
        
        // === BCLR - Bit Test and Clear ===
        case OP_BCLR: {
            bool bit_set = (value & bit_mask) != 0;
            m68k_set_flag(cpu, SR_ZERO, !bit_set);
            cpu->d[reg_dst] = value & ~bit_mask;
            break;
        }
        
        // === BSET - Bit Test and Set ===
        case OP_BSET: {
            bool bit_set = (value & bit_mask) != 0;
            m68k_set_flag(cpu, SR_ZERO, !bit_set);
            cpu->d[reg_dst] = value | bit_mask;
            break;
        }
        
        default:
            break;
    }
}

// Execute BCD (Binary Coded Decimal) instructions
void m68k_execute_bcd(M68K_CPU* cpu, OpCode op, uint16_t opcode) {
    uint8_t reg_src = opcode & 0x7;
    uint8_t reg_dst = (opcode >> 9) & 0x7;
    
    switch (op) {
        // === ABCD - Add Decimal with Extend ===
        case OP_ABCD: {
            uint8_t src = cpu->d[reg_src] & 0xFF;
            uint8_t dst = cpu->d[reg_dst] & 0xFF;
            uint8_t extend = m68k_get_flag(cpu, SR_EXTEND) ? 1 : 0;
            
            // BCD addition
            uint8_t low = (dst & 0xF) + (src & 0xF) + extend;
            uint8_t carry_low = 0;
            if (low > 9) {
                low += 6;
                carry_low = 1;
            }
            
            uint8_t high = ((dst >> 4) & 0xF) + ((src >> 4) & 0xF) + carry_low;
            uint8_t carry_high = 0;
            if (high > 9) {
                high += 6;
                carry_high = 1;
            }
            
            uint8_t result = ((high & 0xF) << 4) | (low & 0xF);
            cpu->d[reg_dst] = (cpu->d[reg_dst] & 0xFFFFFF00) | result;
            
            m68k_set_flag(cpu, SR_CARRY, carry_high != 0);
            m68k_set_flag(cpu, SR_EXTEND, carry_high != 0);
            if (result != 0) {
                m68k_set_flag(cpu, SR_ZERO, false);
            }
            break;
        }
        
        // === SBCD - Subtract Decimal with Extend ===
        case OP_SBCD: {
            uint8_t src = cpu->d[reg_src] & 0xFF;
            uint8_t dst = cpu->d[reg_dst] & 0xFF;
            uint8_t extend = m68k_get_flag(cpu, SR_EXTEND) ? 1 : 0;
            
            // BCD subtraction
            int16_t low = (dst & 0xF) - (src & 0xF) - extend;
            uint8_t borrow_low = 0;
            if (low < 0) {
                low -= 6;
                borrow_low = 1;
            }
            
            int16_t high = ((dst >> 4) & 0xF) - ((src >> 4) & 0xF) - borrow_low;
            uint8_t borrow_high = 0;
            if (high < 0) {
                high -= 6;
                borrow_high = 1;
            }
            
            uint8_t result = ((high & 0xF) << 4) | (low & 0xF);
            cpu->d[reg_dst] = (cpu->d[reg_dst] & 0xFFFFFF00) | result;
            
            m68k_set_flag(cpu, SR_CARRY, borrow_high != 0);
            m68k_set_flag(cpu, SR_EXTEND, borrow_high != 0);
            if (result != 0) {
                m68k_set_flag(cpu, SR_ZERO, false);
            }
            break;
        }
        
        // === NBCD - Negate Decimal with Extend ===
        case OP_NBCD: {
            uint8_t dst = cpu->d[reg_dst] & 0xFF;
            uint8_t extend = m68k_get_flag(cpu, SR_EXTEND) ? 1 : 0;
            
            // BCD negation (0 - dst - extend)
            int16_t low = 0 - (dst & 0xF) - extend;
            uint8_t borrow_low = 0;
            if (low < 0) {
                low -= 6;
                borrow_low = 1;
            }
            
            int16_t high = 0 - ((dst >> 4) & 0xF) - borrow_low;
            uint8_t borrow_high = 0;
            if (high < 0) {
                high -= 6;
                borrow_high = 1;
            }
            
            uint8_t result = ((high & 0xF) << 4) | (low & 0xF);
            cpu->d[reg_dst] = (cpu->d[reg_dst] & 0xFFFFFF00) | result;
            
            m68k_set_flag(cpu, SR_CARRY, borrow_high != 0);
            m68k_set_flag(cpu, SR_EXTEND, borrow_high != 0);
            if (result != 0) {
                m68k_set_flag(cpu, SR_ZERO, false);
            }
            break;
        }
        
        default:
            break;
    }
}

// Execute MOVEM - Move Multiple Registers
void m68k_execute_movem(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                        uint16_t opcode) {
    uint16_t register_list = m68k_fetch_word(cpu, memory, mem_size);
    uint8_t direction = (opcode >> 10) & 0x1; // 0=reg->mem, 1=mem->reg
    uint8_t mode = (opcode >> 3) & 0x7;
    uint8_t reg = opcode & 0x7;
    OperandSize size = ((opcode >> 6) & 0x1) ? SIZE_LONG : SIZE_WORD;
    
    uint32_t address = 0;
    
    // Get effective address
    if (mode == 2) { // (An)
        address = cpu->a[reg];
    } else if (mode == 4) { // -(An)
        address = cpu->a[reg];
    } else if (mode == 5) { // (d16,An)
        int16_t disp = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
        address = cpu->a[reg] + disp;
    }
    
    if (direction == 0) {
        // Register to memory
        for (int i = 0; i < 8; i++) {
            if (register_list & (1 << i)) {
                m68k_write_memory(cpu, memory, mem_size, address, 
                                cpu->d[i], size);
                address += (size == SIZE_LONG) ? 4 : 2;
            }
        }
        for (int i = 0; i < 8; i++) {
            if (register_list & (1 << (i + 8))) {
                m68k_write_memory(cpu, memory, mem_size, address, 
                                cpu->a[i], size);
                address += (size == SIZE_LONG) ? 4 : 2;
            }
        }
    } else {
        // Memory to register
        for (int i = 0; i < 8; i++) {
            if (register_list & (1 << i)) {
                cpu->d[i] = m68k_read_memory(cpu, memory, mem_size, 
                                            address, size);
                address += (size == SIZE_LONG) ? 4 : 2;
            }
        }
        for (int i = 0; i < 8; i++) {
            if (register_list & (1 << (i + 8))) {
                cpu->a[i] = m68k_read_memory(cpu, memory, mem_size, 
                                            address, size);
                address += (size == SIZE_LONG) ? 4 : 2;
            }
        }
    }
    
    // Update address register if needed
    if (mode == 2 || mode == 4) {
        cpu->a[reg] = address;
    }
}

// Execute MOVEP - Move Peripheral
void m68k_execute_movep(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                        uint16_t opcode) {
    uint8_t data_reg = (opcode >> 9) & 0x7;
    uint8_t addr_reg = opcode & 0x7;
    uint8_t direction = (opcode >> 7) & 0x1; // 0=mem->reg, 1=reg->mem
    uint8_t size_bit = (opcode >> 6) & 0x1; // 0=word, 1=long
    
    int16_t displacement = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
    uint32_t address = cpu->a[addr_reg] + displacement;
    
    if (direction == 0) {
        // Memory to register (every other byte)
        if (size_bit == 0) {
            // Word
            uint16_t value = 0;
            value |= (memory[address] << 8);
            value |= memory[address + 2];
            cpu->d[data_reg] = (cpu->d[data_reg] & 0xFFFF0000) | value;
        } else {
            // Long
            uint32_t value = 0;
            value |= (memory[address] << 24);
            value |= (memory[address + 2] << 16);
            value |= (memory[address + 4] << 8);
            value |= memory[address + 6];
            cpu->d[data_reg] = value;
        }
    } else {
        // Register to memory (every other byte)
        if (size_bit == 0) {
            // Word
            uint16_t value = cpu->d[data_reg] & 0xFFFF;
            memory[address] = (value >> 8) & 0xFF;
            memory[address + 2] = value & 0xFF;
        } else {
            // Long
            uint32_t value = cpu->d[data_reg];
            memory[address] = (value >> 24) & 0xFF;
            memory[address + 2] = (value >> 16) & 0xFF;
            memory[address + 4] = (value >> 8) & 0xFF;
            memory[address + 6] = value & 0xFF;
        }
    }
}

// Execute CMPM - Compare Memory to Memory
void m68k_execute_cmpm(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                       uint16_t opcode) {
    uint8_t reg_src = opcode & 0x7;
    uint8_t reg_dst = (opcode >> 9) & 0x7;
    OperandSize size = (OperandSize)(1 << ((opcode >> 6) & 0x3));
    
    uint32_t src = m68k_read_memory(cpu, memory, mem_size, cpu->a[reg_src], size);
    uint32_t dst = m68k_read_memory(cpu, memory, mem_size, cpu->a[reg_dst], size);
    
    cpu->a[reg_src] += size;
    cpu->a[reg_dst] += size;
    
    uint32_t result = dst - src;
    m68k_set_flags_sub(cpu, src, dst, result, size);
}

// Execute addressing mode operations
uint32_t m68k_get_ea_value(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                           uint8_t mode, uint8_t reg, OperandSize size) {
    uint32_t ea = 0;
    
    switch (mode) {
        case 0: // Dn
            return cpu->d[reg];
        
        case 1: // An
            return cpu->a[reg];
        
        case 2: // (An)
            ea = cpu->a[reg];
            return m68k_read_memory(cpu, memory, mem_size, ea, size);
        
        case 3: // (An)+
            ea = cpu->a[reg];
            cpu->a[reg] += size;
            return m68k_read_memory(cpu, memory, mem_size, ea, size);
        
        case 4: // -(An)
            cpu->a[reg] -= size;
            ea = cpu->a[reg];
            return m68k_read_memory(cpu, memory, mem_size, ea, size);
        
        case 5: { // (d16,An)
            int16_t disp = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
            ea = cpu->a[reg] + disp;
            return m68k_read_memory(cpu, memory, mem_size, ea, size);
        }
        
        case 6: { // (d8,An,Xn)
            uint16_t ext = m68k_fetch_word(cpu, memory, mem_size);
            int8_t disp = (int8_t)(ext & 0xFF);
            uint8_t idx_reg = (ext >> 12) & 0x7;
            bool is_addr = (ext >> 15) & 0x1;
            
            int32_t index = is_addr ? cpu->a[idx_reg] : cpu->d[idx_reg];
            if ((ext & 0x0800) == 0) { // Word index
                index = (int16_t)index;
            }
            
            ea = cpu->a[reg] + disp + index;
            return m68k_read_memory(cpu, memory, mem_size, ea, size);
        }
        
        case 7: {
            switch (reg) {
                case 0: // Absolute short
                    ea = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
                    return m68k_read_memory(cpu, memory, mem_size, ea, size);
                
                case 1: // Absolute long
                    ea = m68k_fetch_long(cpu, memory, mem_size);
                    return m68k_read_memory(cpu, memory, mem_size, ea, size);
                
                case 2: { // (d16,PC)
                    uint32_t pc = cpu->pc;
                    int16_t disp = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
                    ea = pc + disp;
                    return m68k_read_memory(cpu, memory, mem_size, ea, size);
                }
                
                case 4: // Immediate
                    if (size == SIZE_LONG) {
                        return m68k_fetch_long(cpu, memory, mem_size);
                    } else {
                        return m68k_fetch_word(cpu, memory, mem_size);
                    }
                
                default:
                    return 0;
            }
        }
        
        default:
            return 0;
    }
}

void m68k_set_ea_value(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                       uint8_t mode, uint8_t reg, uint32_t value, OperandSize size) {
    uint32_t ea = 0;
    
    switch (mode) {
        case 0: // Dn
            if (size == SIZE_BYTE) {
                cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00) | (value & 0xFF);
            } else if (size == SIZE_WORD) {
                cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | (value & 0xFFFF);
            } else {
                cpu->d[reg] = value;
            }
            break;
        
        case 1: // An
            cpu->a[reg] = value;
            break;
        
        case 2: // (An)
            ea = cpu->a[reg];
            m68k_write_memory(cpu, memory, mem_size, ea, value, size);
            break;
        
        case 3: // (An)+
            ea = cpu->a[reg];
            m68k_write_memory(cpu, memory, mem_size, ea, value, size);
            cpu->a[reg] += size;
            break;
        
        case 4: // -(An)
            cpu->a[reg] -= size;
            ea = cpu->a[reg];
            m68k_write_memory(cpu, memory, mem_size, ea, value, size);
            break;
        
        case 5: { // (d16,An)
            int16_t disp = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
            ea = cpu->a[reg] + disp;
            m68k_write_memory(cpu, memory, mem_size, ea, value, size);
            break;
        }
        
        case 7: {
            switch (reg) {
                case 0: // Absolute short
                    ea = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
                    m68k_write_memory(cpu, memory, mem_size, ea, value, size);
                    break;
                
                case 1: // Absolute long
                    ea = m68k_fetch_long(cpu, memory, mem_size);
                    m68k_write_memory(cpu, memory, mem_size, ea, value, size);
                    break;
                
                default:
                    break;
            }
            break;
        }
        
        default:
            break;
    }
}
