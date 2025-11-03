#include "m68k_cpu.h"
#include <string.h>
#include <stdio.h>

// Execute branch and control flow instructions
void m68k_execute_branches(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, 
                           OpCode op, uint16_t opcode) {
    uint8_t reg = opcode & 0x7;
    int8_t displacement = (int8_t)(opcode & 0xFF);
    
    switch (op) {
        // === BRA - Branch Always ===
        case OP_BRA: {
            if (displacement == 0) {
                displacement = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
            }
            cpu->pc += displacement;
            cpu->branch_count++;
            cpu->branch_taken++;
            break;
        }
        
        // === BSR - Branch to Subroutine ===
        case OP_BSR: {
            if (displacement == 0) {
                displacement = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
            }
            // Push return address
            cpu->a[7] -= 4;
            m68k_write_memory(cpu, memory, mem_size, cpu->a[7], cpu->pc, SIZE_LONG);
            cpu->pc += displacement;
            cpu->branch_count++;
            cpu->branch_taken++;
            break;
        }
        
        // === Bcc - Conditional Branches ===
        case OP_BEQ:
        case OP_BNE:
        case OP_BCC:
        case OP_BCS:
        case OP_BGE:
        case OP_BLT:
        case OP_BGT:
        case OP_BLE:
        case OP_BHI:
        case OP_BLS:
        case OP_BPL:
        case OP_BMI:
        case OP_BVC:
        case OP_BVS: {
            uint8_t condition = (opcode >> 8) & 0xF;
            cpu->branch_count++;
            
            if (m68k_test_condition(cpu, condition)) {
                if (displacement == 0) {
                    displacement = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
                }
                cpu->pc += displacement;
                cpu->branch_taken++;
            }
            break;
        }
        
        // === DBcc - Decrement and Branch ===
        case OP_DBCC:
        case OP_DBCS:
        case OP_DBEQ:
        case OP_DBNE:
        case OP_DBGE:
        case OP_DBLT:
        case OP_DBGT:
        case OP_DBLE:
        case OP_DBHI:
        case OP_DBLS:
        case OP_DBPL:
        case OP_DBMI:
        case OP_DBVC:
        case OP_DBVS:
        case OP_DBT:
        case OP_DBF: {
            uint8_t condition = (opcode >> 8) & 0xF;
            int16_t displacement = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
            
            if (!m68k_test_condition(cpu, condition)) {
                uint16_t counter = cpu->d[reg] & 0xFFFF;
                counter--;
                cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | counter;
                
                if (counter != 0xFFFF) {
                    cpu->pc += displacement;
                }
            }
            break;
        }
        
        // === JMP - Jump ===
        case OP_JMP: {
            uint8_t mode = (opcode >> 3) & 0x7;
            uint8_t reg = opcode & 0x7;
            
            if (mode == 2) { // (An)
                cpu->pc = cpu->a[reg];
            } else if (mode == 7 && reg == 0) { // Absolute word
                cpu->pc = m68k_fetch_word(cpu, memory, mem_size);
            } else if (mode == 7 && reg == 1) { // Absolute long
                cpu->pc = m68k_fetch_long(cpu, memory, mem_size);
            } else {
                printf("Unsupported JMP addressing mode\n");
            }
            break;
        }
        
        // === JSR - Jump to Subroutine ===
        case OP_JSR: {
            uint8_t mode = (opcode >> 3) & 0x7;
            uint8_t reg = opcode & 0x7;
            uint32_t target = 0;
            
            if (mode == 2) { // (An)
                target = cpu->a[reg];
            } else if (mode == 7 && reg == 0) { // Absolute word
                target = m68k_fetch_word(cpu, memory, mem_size);
            } else if (mode == 7 && reg == 1) { // Absolute long
                target = m68k_fetch_long(cpu, memory, mem_size);
            }
            
            // Push return address
            cpu->a[7] -= 4;
            m68k_write_memory(cpu, memory, mem_size, cpu->a[7], cpu->pc, SIZE_LONG);
            cpu->pc = target;
            break;
        }
        
        // === RTS - Return from Subroutine ===
        case OP_RTS: {
            uint32_t return_addr = m68k_read_memory(cpu, memory, mem_size, 
                                                    cpu->a[7], SIZE_LONG);
            cpu->a[7] += 4;
            cpu->pc = return_addr;
            break;
        }
        
        // === RTR - Return and Restore ===
        case OP_RTR: {
            uint16_t ccr = m68k_read_memory(cpu, memory, mem_size, 
                                           cpu->a[7], SIZE_WORD);
            cpu->a[7] += 2;
            cpu->sr = (cpu->sr & 0xFF00) | (ccr & 0x00FF);
            
            uint32_t return_addr = m68k_read_memory(cpu, memory, mem_size, 
                                                    cpu->a[7], SIZE_LONG);
            cpu->a[7] += 4;
            cpu->pc = return_addr;
            break;
        }
        
        // === RTE - Return from Exception ===
        case OP_RTE: {
            if (!(cpu->sr & SR_SUPERVISOR)) {
                printf("RTE in user mode!\n");
                cpu->halted = true;
                break;
            }
            
            cpu->sr = m68k_read_memory(cpu, memory, mem_size, cpu->a[7], SIZE_WORD);
            cpu->a[7] += 2;
            
            uint32_t return_addr = m68k_read_memory(cpu, memory, mem_size, 
                                                    cpu->a[7], SIZE_LONG);
            cpu->a[7] += 4;
            cpu->pc = return_addr;
            break;
        }
        
        // === Scc - Set According to Condition ===
        case OP_SCC:
        case OP_SCS:
        case OP_SEQ:
        case OP_SNE:
        case OP_SGE:
        case OP_SLT:
        case OP_SGT:
        case OP_SLE:
        case OP_SHI:
        case OP_SLS:
        case OP_SPL:
        case OP_SMI:
        case OP_SVC:
        case OP_SVS:
        case OP_ST:
        case OP_SF: {
            uint8_t condition = (opcode >> 8) & 0xF;
            uint8_t reg = opcode & 0x7;
            
            if (m68k_test_condition(cpu, condition)) {
                cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00) | 0xFF;
            } else {
                cpu->d[reg] = (cpu->d[reg] & 0xFFFFFF00);
            }
            break;
        }
        
        default:
            break;
    }
}

// Execute special and system instructions
void m68k_execute_special(M68K_CPU* cpu, uint8_t* memory, size_t mem_size,
                         OpCode op, uint16_t opcode) {
    uint8_t reg = opcode & 0x7;
    
    switch (op) {
        // === LEA - Load Effective Address ===
        case OP_LEA: {
            uint8_t mode = (opcode >> 3) & 0x7;
            uint8_t reg_src = opcode & 0x7;
            uint8_t reg_dst = (opcode >> 9) & 0x7;
            
            if (mode == 2) { // (An)
                cpu->a[reg_dst] = cpu->a[reg_src];
            } else if (mode == 5) { // (d16,An)
                int16_t disp = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
                cpu->a[reg_dst] = cpu->a[reg_src] + disp;
            } else if (mode == 7 && reg_src == 0) { // Absolute word
                cpu->a[reg_dst] = m68k_fetch_word(cpu, memory, mem_size);
            } else if (mode == 7 && reg_src == 1) { // Absolute long
                cpu->a[reg_dst] = m68k_fetch_long(cpu, memory, mem_size);
            }
            break;
        }
        
        // === PEA - Push Effective Address ===
        case OP_PEA: {
            uint8_t mode = (opcode >> 3) & 0x7;
            uint8_t reg = opcode & 0x7;
            uint32_t ea = 0;
            
            if (mode == 2) { // (An)
                ea = cpu->a[reg];
            }
            
            cpu->a[7] -= 4;
            m68k_write_memory(cpu, memory, mem_size, cpu->a[7], ea, SIZE_LONG);
            break;
        }
        
        // === LINK ===
        case OP_LINK: {
            int16_t displacement = (int16_t)m68k_fetch_word(cpu, memory, mem_size);
            cpu->a[7] -= 4;
            m68k_write_memory(cpu, memory, mem_size, cpu->a[7], cpu->a[reg], SIZE_LONG);
            cpu->a[reg] = cpu->a[7];
            cpu->a[7] += displacement;
            break;
        }
        
        // === UNLK ===
        case OP_UNLK: {
            cpu->a[7] = cpu->a[reg];
            cpu->a[reg] = m68k_read_memory(cpu, memory, mem_size, cpu->a[7], SIZE_LONG);
            cpu->a[7] += 4;
            break;
        }
        
        // === SWAP ===
        case OP_SWAP: {
            uint32_t value = cpu->d[reg];
            cpu->d[reg] = ((value & 0xFFFF) << 16) | ((value >> 16) & 0xFFFF);
            m68k_set_flags(cpu, cpu->d[reg], SIZE_LONG);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            break;
        }
        
        // === EXT - Sign Extend ===
        case OP_EXT: {
            if ((opcode & 0x0040) == 0) { // Byte to word
                int8_t byte_val = (int8_t)(cpu->d[reg] & 0xFF);
                cpu->d[reg] = (cpu->d[reg] & 0xFFFF0000) | (uint16_t)(int16_t)byte_val;
            } else { // Word to long
                int16_t word_val = (int16_t)(cpu->d[reg] & 0xFFFF);
                cpu->d[reg] = (uint32_t)(int32_t)word_val;
            }
            m68k_set_flags(cpu, cpu->d[reg], SIZE_LONG);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            break;
        }
        
        // === EXG - Exchange Registers ===
        case OP_EXG: {
            uint8_t reg1 = (opcode >> 9) & 0x7;
            uint8_t reg2 = opcode & 0x7;
            uint8_t mode = (opcode >> 3) & 0x1F;
            
            uint32_t temp;
            if (mode == 0x08) { // Data registers
                temp = cpu->d[reg1];
                cpu->d[reg1] = cpu->d[reg2];
                cpu->d[reg2] = temp;
            } else if (mode == 0x09) { // Address registers
                temp = cpu->a[reg1];
                cpu->a[reg1] = cpu->a[reg2];
                cpu->a[reg2] = temp;
            } else if (mode == 0x11) { // Data and address
                temp = cpu->d[reg1];
                cpu->d[reg1] = cpu->a[reg2];
                cpu->a[reg2] = temp;
            }
            break;
        }
        
        // === TRAP ===
        case OP_TRAP: {
            uint8_t vector = opcode & 0xF;
            printf("TRAP #%d\n", vector);
            // In real 68000, this would vector through exception table
            // For simulation, we can handle common traps
            if (vector == 15) { // TRAP #15 - often used for I/O
                // Could implement TRAP handlers here
            }
            break;
        }
        
        // === TRAPV - Trap on Overflow ===
        case OP_TRAPV: {
            if (m68k_get_flag(cpu, SR_OVERFLOW)) {
                printf("TRAPV - overflow trap\n");
                cpu->halted = true;
            }
            break;
        }
        
        // === CHK - Check Register Against Bounds ===
        case OP_CHK: {
            uint8_t reg_data = (opcode >> 9) & 0x7;
            int16_t data = (int16_t)(cpu->d[reg_data] & 0xFFFF);
            int16_t upper = (int16_t)(cpu->d[reg] & 0xFFFF);
            
            if (data < 0 || data > upper) {
                printf("CHK - bounds exception\n");
                cpu->halted = true;
            }
            break;
        }
        
        // === TAS - Test and Set ===
        case OP_TAS: {
            uint8_t value = cpu->d[reg] & 0xFF;
            m68k_set_flags(cpu, value, SIZE_BYTE);
            m68k_set_flag(cpu, SR_CARRY, false);
            m68k_set_flag(cpu, SR_OVERFLOW, false);
            cpu->d[reg] |= 0x80; // Set bit 7
            break;
        }
        
        // === RESET ===
        case OP_RESET: {
            if (cpu->sr & SR_SUPERVISOR) {
                printf("RESET instruction executed\n");
                // In hardware, this would reset external devices
            } else {
                printf("RESET in user mode - privilege violation\n");
                cpu->halted = true;
            }
            break;
        }
        
        // === STOP ===
        case OP_STOP: {
            if (cpu->sr & SR_SUPERVISOR) {
                uint16_t immediate = m68k_fetch_word(cpu, memory, mem_size);
                cpu->sr = immediate;
                cpu->halted = true; // Stop until interrupt
                printf("STOP #$%04X\n", immediate);
            } else {
                printf("STOP in user mode - privilege violation\n");
                cpu->halted = true;
            }
            break;
        }
        
        default:
            break;
    }
}

// Main execution cycle dispatcher
void m68k_execute_cycle(M68K_CPU* cpu, uint8_t* memory, size_t mem_size) {
    if (cpu->halted) return;
    
    cpu->bus_active = false;
    
    // Check for breakpoint
    if (m68k_check_breakpoint(cpu, cpu->pc)) {
        cpu->halted = true;
        printf("Breakpoint hit at 0x%08X\n", cpu->pc);
        return;
    }
    
    // Fetch instruction
    uint16_t opcode = m68k_fetch_word(cpu, memory, mem_size);
    
    // Decode instruction
    OpCode op;
    uint8_t params[8] = {0};
    m68k_decode_instruction(cpu, opcode, &op, params);
    
    // Execute instruction
    if (op >= OP_BRA && op <= OP_DBF) {
        m68k_execute_branches(cpu, memory, mem_size, op, opcode);
    } else if (op >= OP_LEA && op <= OP_STOP) {
        m68k_execute_special(cpu, memory, mem_size, op, opcode);
    } else {
        m68k_execute_instruction(cpu, memory, mem_size, op, opcode);
    }
}
