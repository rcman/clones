#include "m68k_cpu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// NOTE: Core functions (m68k_init, m68k_reset, m68k_get_flag, m68k_set_flag, etc.)
//       are implemented in m68k_cpu_extended.c to avoid duplication.
//       This file contains only memory access and fetch functions.

uint16_t m68k_fetch_word(M68K_CPU* cpu, uint8_t* memory, size_t mem_size) {
    if (cpu->pc >= mem_size - 1) {
        printf("PC out of bounds: 0x%08X\n", cpu->pc);
        cpu->halted = true;
        return 0;
    }
    
    // Simulate 1024-bit bus transfer (128 bytes at a time)
    cpu->bus_address = cpu->pc & ~0x7F;  // Align to 128-byte boundary
    cpu->bus_active = true;
    memcpy(cpu->bus_data, &memory[cpu->bus_address], 128);
    
    uint16_t word = (memory[cpu->pc] << 8) | memory[cpu->pc + 1];
    cpu->pc += 2;
    cpu->cycle_count++;
    
    return word;
}

uint32_t m68k_fetch_long(M68K_CPU* cpu, uint8_t* memory, size_t mem_size) {
    uint32_t high = m68k_fetch_word(cpu, memory, mem_size);
    uint32_t low = m68k_fetch_word(cpu, memory, mem_size);
    return (high << 16) | low;
}

uint32_t m68k_read_memory(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, 
                          uint32_t addr, OperandSize size) {
    if (addr >= mem_size) {
        printf("Memory read out of bounds: 0x%08X\n", addr);
        return 0;
    }
    
    cpu->bus_address = addr & ~0x7F;
    cpu->bus_active = true;
    memcpy(cpu->bus_data, &memory[cpu->bus_address], 128);
    cpu->load_count++;
    
    uint32_t value = 0;
    switch (size) {
        case SIZE_BYTE:
            value = memory[addr];
            break;
        case SIZE_WORD:
            if (addr + 1 < mem_size) {
                value = (memory[addr] << 8) | memory[addr + 1];
            }
            break;
        case SIZE_LONG:
            if (addr + 3 < mem_size) {
                value = (memory[addr] << 24) | (memory[addr + 1] << 16) |
                       (memory[addr + 2] << 8) | memory[addr + 3];
            }
            break;
        default:
            break;
    }
    
    cpu->cycle_count++;
    return value;
}

void m68k_write_memory(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, 
                       uint32_t addr, uint32_t value, OperandSize size) {
    if (addr >= mem_size) {
        printf("Memory write out of bounds: 0x%08X\n", addr);
        return;
    }
    
    cpu->bus_address = addr & ~0x7F;
    cpu->bus_active = true;
    cpu->store_count++;
    
    switch (size) {
        case SIZE_BYTE:
            memory[addr] = value & 0xFF;
            break;
        case SIZE_WORD:
            if (addr + 1 < mem_size) {
                memory[addr] = (value >> 8) & 0xFF;
                memory[addr + 1] = value & 0xFF;
            }
            break;
        case SIZE_LONG:
            if (addr + 3 < mem_size) {
                memory[addr] = (value >> 24) & 0xFF;
                memory[addr + 1] = (value >> 16) & 0xFF;
                memory[addr + 2] = (value >> 8) & 0xFF;
                memory[addr + 3] = value & 0xFF;
            }
            break;
        default:
            break;
    }
    
    cpu->cycle_count++;
}

// NOTE: The following functions are implemented elsewhere to avoid duplication:
// - m68k_test_condition() -> in m68k_cpu_extended.c
// - m68k_decode_instruction() -> in m68k_decode.c
// - m68k_execute_instruction() -> in m68k_execute.c
// - m68k_execute_cycle() -> in m68k_execute2.c
