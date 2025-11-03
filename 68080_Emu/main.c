#include "m68k_cpu.h"
#include "m68k_display.h"
#include "m68k_peripherals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MEMORY_SIZE (16 * 1024 * 1024)  // 16 MB

// Simple test program for the 68000
void load_test_program(uint8_t* memory) {
    // This is a simple 68000 test program
    // It will initialize registers, do some operations, and loop
    
    uint16_t program[] = {
        // Initialize some data registers
        0x7001,  // MOVEQ #1, D0     - Load 1 into D0
        0x7202,  // MOVEQ #2, D1     - Load 2 into D1
        0x7403,  // MOVEQ #3, D2     - Load 3 into D2
        
        // Simple arithmetic
        0xD240,  // ADD.W D0, D1     - D1 = D1 + D0 (2 + 1 = 3)
        0x9440,  // SUB.W D0, D2     - D2 = D2 - D0 (3 - 1 = 2)
        
        // Compare and branch
        0xB440,  // CMP.W D0, D2     - Compare D0 with D2
        0x6700,  // BEQ (skip ahead)
        0x0002,  // ... 2 bytes
        
        // More arithmetic
        0xD440,  // ADD.W D0, D2     - D2 = D2 + D0
        
        // Infinite loop
        0x4E71,  // NOP
        0x60FE,  // BRA -2 (loop back to NOP)
    };
    
    // Copy program to memory at address 0x1000
    uint32_t program_start = 0x1000;
    for (size_t i = 0; i < sizeof(program) / sizeof(program[0]); i++) {
        memory[program_start + i * 2] = (program[i] >> 8) & 0xFF;
        memory[program_start + i * 2 + 1] = program[i] & 0xFF;
    }
    
    printf("Test program loaded at 0x%08X\n", program_start);
    printf("Program size: %zu bytes\n", sizeof(program));
}

int main(int argc, char* argv[]) {
    (void)argc;  // Unused
    (void)argv;  // Unused
    
    printf("===========================================\n");
    printf("  68000 Simulator - 1024-bit Bus Edition  \n");
    printf("===========================================\n\n");
    
    // Allocate memory
    uint8_t* memory = (uint8_t*)calloc(MEMORY_SIZE, 1);
    if (!memory) {
        printf("Failed to allocate memory!\n");
        return 1;
    }
    
    printf("Allocated %d MB of memory\n", MEMORY_SIZE / (1024 * 1024));
    
    // Initialize CPU
    M68K_CPU cpu;
    m68k_init(&cpu);
    cpu.pc = 0x1000;  // Start at test program
    cpu.a[7] = 0x10000;  // Stack pointer
    
    printf("CPU initialized\n");
    printf("  PC: 0x%08X\n", cpu.pc);
    printf("  SP: 0x%08X\n", cpu.a[7]);
    
    // Initialize peripherals
    M68K_Peripherals peripherals;
    peripherals_init(&peripherals);
    printf("Peripherals initialized\n");
    
    // Load test program
    load_test_program(memory);
    
    // Initialize display
    M68K_Display display;
    if (!m68k_display_init(&display)) {
        printf("Failed to initialize display!\n");
        free(memory);
        return 1;
    }
    
    printf("\nDisplay initialized\n");
    printf("Window size: %dx%d\n", WINDOW_WIDTH, WINDOW_HEIGHT);
    printf("\nControls:\n");
    printf("  SPACE     - Run/Pause simulation\n");
    printf("  S         - Step one instruction\n");
    printf("  R         - Reset CPU\n");
    printf("  UP/DOWN   - Scroll memory view\n");
    printf("  PGUP/PGDN - Fast scroll memory view\n");
    printf("  ESC       - Quit\n");
    printf("\nSimulation starting (paused)...\n\n");
    
    // Main loop
    uint32_t last_time = SDL_GetTicks();
    uint32_t frame_count = 0;
    
    while (display.running) {
        uint32_t current_time = SDL_GetTicks();
        uint32_t delta_time = current_time - last_time;
        
        // Handle events
        if (!m68k_display_handle_events(&display, &cpu)) {
            break;
        }
        
        // Execute CPU cycles
        if (!display.paused || display.step_mode) {
            // Run multiple cycles per frame for better performance
            int cycles_per_frame = display.paused ? 1 : 1000;
            
            for (int i = 0; i < cycles_per_frame && !cpu.halted; i++) {
                m68k_execute_cycle(&cpu, memory, MEMORY_SIZE);
            }
            
            display.step_mode = false;
        }
        
        // Render at 60 FPS
        if (delta_time >= 16) {  // ~60 FPS
            m68k_display_render(&display, &cpu, memory, MEMORY_SIZE, &peripherals);
            
            frame_count++;
            if (frame_count % 60 == 0) {
                // Calculate actual FPS
                display.fps = 1000 / (delta_time > 0 ? delta_time : 1);
            }
            
            last_time = current_time;
        }
        
        // Small delay to prevent CPU spinning
        SDL_Delay(1);
    }
    
    // Cleanup
    printf("\nShutting down...\n");
    printf("Total cycles executed: %llu\n", (unsigned long long)cpu.cycle_count);
    
    peripherals_cleanup(&peripherals);
    m68k_display_cleanup(&display);
    free(memory);
    
    printf("Done.\n");
    
    return 0;
}
