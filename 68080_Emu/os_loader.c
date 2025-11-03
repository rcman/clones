// Example: Loading and running an OS on the 68000 simulator
// Add this to main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m68k_cpu.h"
#include "m68k_peripherals.h"
#include "m68k_display.h"

// Load binary file into memory
bool load_binary(const char* filename, uint8_t* memory, size_t mem_size, uint32_t load_address) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("Failed to open %s\n", filename);
        return false;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Check if it fits
    if (load_address + file_size > mem_size) {
        printf("Binary too large: %ld bytes at 0x%08X\n", file_size, load_address);
        fclose(f);
        return false;
    }
    
    // Load into memory
    size_t bytes_read = fread(&memory[load_address], 1, file_size, f);
    fclose(f);
    
    printf("Loaded %zu bytes from %s to 0x%08X\n", bytes_read, filename, load_address);
    return bytes_read == file_size;
}

// Set up exception vector table
void setup_exception_vectors(M68K_CPU* cpu, uint8_t* memory) {
    // 68000 exception vectors start at address 0x000000
    // Vector 0: Initial SSP (Supervisor Stack Pointer)
    // Vector 1: Initial PC (Program Counter)
    
    uint32_t initial_ssp = 0x00010000;  // Stack at 64KB
    uint32_t initial_pc  = 0x00001000;  // Code starts at 4KB
    
    // Write initial SSP (vector 0)
    memory[0] = (initial_ssp >> 24) & 0xFF;
    memory[1] = (initial_ssp >> 16) & 0xFF;
    memory[2] = (initial_ssp >> 8) & 0xFF;
    memory[3] = initial_ssp & 0xFF;
    
    // Write initial PC (vector 1)
    memory[4] = (initial_pc >> 24) & 0xFF;
    memory[5] = (initial_pc >> 16) & 0xFF;
    memory[6] = (initial_pc >> 8) & 0xFF;
    memory[7] = initial_pc & 0xFF;
    
    // Set up other exception vectors
    // Vector 2: Bus Error
    // Vector 3: Address Error
    // Vector 4: Illegal Instruction
    // Vector 5: Divide by Zero
    // Vector 6: CHK Instruction
    // Vector 7: TRAPV Instruction
    // Vector 8: Privilege Violation
    // Vector 9: Trace
    // Vectors 10-15: Reserved
    // Vectors 16-23: Reserved
    // Vectors 24-31: Spurious Interrupt, Level 1-7 Autovector
    // Vectors 32-47: TRAP #0-15
    // Vectors 48-63: Reserved
    // Vectors 64-255: User Interrupt Vectors
    
    printf("Exception vectors configured:\n");
    printf("  Initial SSP: 0x%08X\n", initial_ssp);
    printf("  Initial PC:  0x%08X\n", initial_pc);
}

// Handle system calls (TRAP instructions)
void handle_trap(M68K_CPU* cpu, uint8_t* memory, size_t mem_size, 
                 M68K_Peripherals* peripherals, uint8_t trap_number) {
    
    switch (trap_number) {
        case 0: // TRAP #0 - Exit
            printf("\nTRAP #0: Exit requested\n");
            printf("Exit code: D0 = %d\n", cpu->d[0]);
            cpu->halted = true;
            break;
        
        case 1: // TRAP #1 - Print character
            printf("%c", (char)(cpu->d[0] & 0xFF));
            fflush(stdout);
            break;
        
        case 2: // TRAP #2 - Print string
            {
                uint32_t str_addr = cpu->a[0];
                printf("String: ");
                while (str_addr < mem_size && memory[str_addr] != 0) {
                    printf("%c", memory[str_addr]);
                    str_addr++;
                }
                printf("\n");
            }
            break;
        
        case 3: // TRAP #3 - Read character
            {
                int ch = getchar();
                cpu->d[0] = (ch == EOF) ? -1 : ch;
            }
            break;
        
        case 4: // TRAP #4 - Get time
            cpu->d[0] = (uint32_t)time(NULL);
            break;
        
        case 5: // TRAP #5 - NVMe read
            {
                uint32_t lba = cpu->d[0];
                uint16_t count = cpu->d[1] & 0xFFFF;
                uint32_t buffer = cpu->a[0];
                
                if (buffer < mem_size) {
                    bool success = nvme_read(&peripherals->nvme, lba, count, 
                                           &memory[buffer]);
                    cpu->d[0] = success ? 0 : -1;
                }
            }
            break;
        
        case 6: // TRAP #6 - NVMe write
            {
                uint32_t lba = cpu->d[0];
                uint16_t count = cpu->d[1] & 0xFFFF;
                uint32_t buffer = cpu->a[0];
                
                if (buffer < mem_size) {
                    bool success = nvme_write(&peripherals->nvme, lba, count, 
                                            &memory[buffer]);
                    cpu->d[0] = success ? 0 : -1;
                }
            }
            break;
        
        case 7: // TRAP #7 - Network send
            {
                Ethernet_Packet packet;
                uint32_t packet_addr = cpu->a[0];
                uint16_t length = cpu->d[0] & 0xFFFF;
                
                if (packet_addr + length < mem_size && length <= ETH_MTU) {
                    memcpy(packet.data, &memory[packet_addr], length);
                    packet.length = length;
                    eth_send_packet(&peripherals->ethernet, &packet);
                    cpu->d[0] = 0;
                } else {
                    cpu->d[0] = -1;
                }
            }
            break;
        
        case 8: // TRAP #8 - Network receive
            {
                Ethernet_Packet packet;
                if (eth_receive_packet(&peripherals->ethernet, &packet)) {
                    uint32_t buffer = cpu->a[0];
                    if (buffer + packet.length < mem_size) {
                        memcpy(&memory[buffer], packet.data, packet.length);
                        cpu->d[0] = packet.length;
                    } else {
                        cpu->d[0] = -1;
                    }
                } else {
                    cpu->d[0] = 0; // No packet available
                }
            }
            break;
        
        case 9: // TRAP #9 - Graphics: Set pixel
            {
                uint32_t x = cpu->d[0];
                uint32_t y = cpu->d[1];
                uint32_t color = cpu->d[2];
                hdmi_set_pixel(&peripherals->hdmi, x, y, color);
            }
            break;
        
        case 10: // TRAP #10 - Graphics: Draw rectangle
            {
                uint32_t x = cpu->d[0];
                uint32_t y = cpu->d[1];
                uint32_t w = cpu->d[2];
                uint32_t h = cpu->d[3];
                uint32_t color = cpu->d[4];
                hdmi_draw_rect(&peripherals->hdmi, x, y, w, h, color);
            }
            break;
        
        case 11: // TRAP #11 - GPIO: Set pin
            {
                int pin = cpu->d[0] & 0x3F;
                bool value = (cpu->d[1] & 1) != 0;
                gpio_write(&peripherals->gpio, pin, value);
            }
            break;
        
        case 12: // TRAP #12 - GPIO: Read pin
            {
                int pin = cpu->d[0] & 0x3F;
                cpu->d[0] = gpio_read(&peripherals->gpio, pin) ? 1 : 0;
            }
            break;
        
        case 15: // TRAP #15 - General I/O (traditional)
            {
                uint8_t function = (cpu->d[0] >> 8) & 0xFF;
                switch (function) {
                    case 0: // Display string
                        {
                            uint32_t str_addr = cpu->a[1];
                            while (str_addr < mem_size && memory[str_addr] != 0) {
                                printf("%c", memory[str_addr]);
                                str_addr++;
                            }
                        }
                        break;
                    
                    case 1: // Display character
                        printf("%c", (char)(cpu->d[1] & 0xFF));
                        fflush(stdout);
                        break;
                    
                    case 2: // Read character
                        {
                            int ch = getchar();
                            cpu->d[1] = (ch == EOF) ? -1 : ch;
                        }
                        break;
                }
            }
            break;
        
        default:
            printf("Unhandled TRAP #%d\n", trap_number);
            printf("  D0=%08X D1=%08X A0=%08X A1=%08X\n", 
                   cpu->d[0], cpu->d[1], cpu->a[0], cpu->a[1]);
            break;
    }
}

// Main OS loader
int run_operating_system(const char* os_binary) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║       68000 Operating System Loader                      ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    // Initialize CPU
    M68K_CPU cpu;
    m68k_init(&cpu);
    
    // Allocate memory (16MB)
    size_t mem_size = 16 * 1024 * 1024;
    uint8_t* memory = (uint8_t*)calloc(mem_size, 1);
    if (!memory) {
        printf("Failed to allocate memory\n");
        return 1;
    }
    
    // Initialize peripherals
    M68K_Peripherals peripherals;
    peripherals_init(&peripherals);
    
    // Initialize display
    M68K_Display display;
    if (!m68k_display_init(&display)) {
        printf("Failed to initialize display\n");
        free(memory);
        return 1;
    }
    
    // Set up exception vectors
    setup_exception_vectors(&cpu, memory);
    
    // Load OS binary
    if (!load_binary(os_binary, memory, mem_size, 0x00001000)) {
        printf("Failed to load OS binary\n");
        free(memory);
        m68k_display_cleanup(&display);
        return 1;
    }
    
    // Read initial vectors from memory
    cpu.ssp = (memory[0] << 24) | (memory[1] << 16) | 
              (memory[2] << 8) | memory[3];
    cpu.pc = (memory[4] << 24) | (memory[5] << 16) | 
             (memory[6] << 8) | memory[7];
    cpu.a[7] = cpu.ssp;
    cpu.sr = SR_SUPERVISOR; // Start in supervisor mode
    
    printf("\nStarting OS:\n");
    printf("  SSP: 0x%08X\n", cpu.ssp);
    printf("  PC:  0x%08X\n", cpu.pc);
    printf("  SR:  0x%04X\n\n", cpu.sr);
    
    // Main execution loop
    uint32_t last_time = SDL_GetTicks();
    int cycles_per_frame = 10000; // Adjust for speed
    
    while (display.running) {
        // Handle SDL events
        if (!m68k_display_handle_events(&display, &cpu)) {
            break;
        }
        
        // Execute CPU cycles if not paused
        if (!display.paused && !cpu.halted) {
            for (int i = 0; i < cycles_per_frame; i++) {
                m68k_execute_cycle(&cpu, memory, mem_size);
                
                // Check for TRAP instruction
                if (cpu.breakpoint_hit) {
                    // Could be a TRAP - handle it
                    cpu.breakpoint_hit = false;
                }
                
                if (cpu.halted) break;
            }
        }
        
        // Process peripherals
        eth_process(&peripherals.ethernet);
        
        // Update display
        uint32_t current_time = SDL_GetTicks();
        if (current_time - last_time >= 16) { // ~60 FPS
            m68k_display_render(&display, &cpu, memory, mem_size, &peripherals);
            last_time = current_time;
        }
    }
    
    printf("\nOS execution stopped.\n");
    printf("Final state:\n");
    printf("  PC: 0x%08X\n", cpu.pc);
    printf("  SR: 0x%04X\n", cpu.sr);
    printf("  Cycles: %llu\n", cpu.cycle_count);
    printf("  Instructions: %llu\n", cpu.instruction_count);
    
    // Cleanup
    peripherals_cleanup(&peripherals);
    m68k_display_cleanup(&display);
    free(memory);
    
    return 0;
}
