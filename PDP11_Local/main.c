#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "pdp11.h"
#include "disk.h"
#include "serial.h"
#include "telnet.h"

static volatile bool running = true;

void signal_handler(int sig) {
    (void)sig;  // Suppress unused warning
    running = false;
}

/* Handle memory-mapped I/O */
static uint16_t io_read_word(pdp11_t *cpu, serial_t *serial, disk_t *disk, uint16_t addr) {
    (void)cpu;  // May be used later for debugging
    
    // Serial interface
    if (addr >= RCSR && addr <= XBUF) {
        return serial_read_register(serial, addr);
    }
    
    // Disk controller
    if (addr >= RKDS && addr <= RKDA) {
        return disk_read_register(disk, addr);
    }
    
    // Unknown I/O address
    return 0;
}

static void io_write_word(pdp11_t *cpu, serial_t *serial, disk_t *disk, uint16_t addr, uint16_t value) {
    // Serial interface
    if (addr >= RCSR && addr <= XBUF) {
        serial_write_register(serial, addr, value);
        return;
    }
    
    // Disk controller
    if (addr >= RKDS && addr <= RKDA) {
        disk_write_register(disk, addr, value, cpu->memory);
        return;
    }
}

/* Main emulation loop with I/O handling */
static void emulation_loop(pdp11_t *cpu, serial_t *serial, disk_t *disk, telnet_server_t *telnet) {
    struct timespec sleep_time = {0, 1000000};  // 1ms
    int io_check_counter = 0;
    
    // Hook memory-mapped I/O into CPU memory
    // The I/O page is at the top of the 16-bit address space
    // For simplicity, we'll handle I/O in the main loop by checking addresses
    
    while (running && !cpu->halted) {
        // Execute a batch of instructions
        for (int i = 0; i < 10000 && !cpu->halted; i++) {
            // Before executing, check if any I/O memory needs to be updated
            // Update serial receiver status in memory if needed
            if (serial->rcsr & RCSR_DONE) {
                mem_write_word(cpu, RCSR, serial->rcsr);
                mem_write_word(cpu, RBUF, serial->rbuf);
            }
            
            // Update disk controller status
            mem_write_word(cpu, RKCS, disk->rkcs);
            mem_write_word(cpu, RKDS, disk->rkds);
            
            pdp11_step(cpu);
            
            // After execution, check if CPU wrote to I/O registers
            // Check serial transmit
            uint16_t xcsr_mem = mem_read_word(cpu, XCSR);
            uint16_t xbuf_mem = mem_read_word(cpu, XBUF);
            if (xcsr_mem != serial->xcsr || xbuf_mem != 0) {
                if (xbuf_mem != 0) {
                    serial_write_register(serial, XBUF, xbuf_mem);
                    mem_write_word(cpu, XBUF, 0);  // Clear after write
                }
            }
            
            // Check disk controller writes
            uint16_t rkcs_mem = mem_read_word(cpu, RKCS);
            if (rkcs_mem != disk->rkcs) {
                disk_write_register(disk, RKCS, rkcs_mem, cpu->memory);
            }
            
            uint16_t rkwc_mem = mem_read_word(cpu, RKWC);
            if (rkwc_mem != disk->rkwc) {
                disk_write_register(disk, RKWC, rkwc_mem, cpu->memory);
            }
            
            uint16_t rkba_mem = mem_read_word(cpu, RKBA);
            if (rkba_mem != disk->rkba) {
                disk_write_register(disk, RKBA, rkba_mem, cpu->memory);
            }
            
            uint16_t rkda_mem = mem_read_word(cpu, RKDA);
            if (rkda_mem != disk->rkda) {
                disk_write_register(disk, RKDA, rkda_mem, cpu->memory);
            }
        }
        
        // Service I/O devices
        io_check_counter++;
        if (io_check_counter >= 10) {
            io_check_counter = 0;
            
            // Accept new telnet connections
            telnet_accept_connections(telnet);
            
            // Handle serial I/O
            serial_service(serial);
            
            // Transfer data from telnet to serial
            while (telnet_has_input(telnet)) {
                uint8_t byte = telnet_read_byte(telnet);
                serial_input_byte(serial, byte);
            }
            
            // Transfer data from serial to telnet
            while (serial_has_output(serial)) {
                uint8_t byte = serial_read_output(serial);
                telnet_write_byte(telnet, byte);
            }
            
            // Service disk
            disk_service(disk, cpu->memory);
            
            // Small sleep to prevent 100% CPU usage
            nanosleep(&sleep_time, NULL);
        }
    }
    
    if (cpu->halted) {
        printf("\nCPU halted at PC=%06o, total instructions: %u\n", 
               cpu->regs[REG_PC], cpu->instr_count);
    }
}

/* Load a bootstrap or test program */
static void load_bootstrap(pdp11_t *cpu) {
    // Simple bootstrap that echoes characters from serial
    uint16_t bootstrap[] = {
        0012700,  // MOV #177560, R0   ; RCSR address
        0177560,
        0012701,  // MOV #177562, R1   ; RBUF address
        0177562,
        0012702,  // MOV #177564, R2   ; XCSR address
        0177564,
        0012703,  // MOV #177566, R3   ; XBUF address
        0177566,
        // loop:
        0105710,  // TSTB (R0)        ; Check RCSR
        0001775,  // BEQ loop         ; Wait for character
        0116104,  // MOVB (R1), R4    ; Read character
        0105712,  // TSTB (R2)        ; Check XCSR ready
        0001775,  // BEQ .-2          ; Wait for ready
        0110413,  // MOVB R4, (R3)    ; Write character
        0000765,  // BR loop          ; Repeat
    };
    
    pdp11_load_memory(cpu, 01000, (uint8_t*)bootstrap, sizeof(bootstrap));
    cpu->regs[REG_PC] = 01000;  // Start at octal 1000
}

int main(int argc, char *argv[]) {
    int port = 2311;
    const char *disk_file = "disk.img";
    uint32_t memory_size = 256 * 1024;  // 256KB default
    bool use_bootstrap = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--disk") == 0 && i + 1 < argc) {
            disk_file = argv[++i];
        } else if (strcmp(argv[i], "--memory") == 0 && i + 1 < argc) {
            memory_size = atoi(argv[++i]) * 1024;
        } else if (strcmp(argv[i], "--bootstrap") == 0) {
            use_bootstrap = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("PDP-11 Emulator\n");
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --port PORT       Telnet port (default: 2311)\n");
            printf("  --disk FILE       Disk image file (default: disk.img)\n");
            printf("  --memory SIZE     Memory size in KB (default: 256)\n");
            printf("  --bootstrap       Load echo bootstrap program\n");
            printf("  --help            Show this help\n");
            return 0;
        }
    }
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("PDP-11 Emulator starting...\n");
    printf("Memory: %u KB\n", memory_size / 1024);
    printf("Disk: %s\n", disk_file);
    
    // Initialize components
    pdp11_t *cpu = pdp11_init(memory_size);
    if (!cpu) {
        fprintf(stderr, "Failed to initialize CPU\n");
        return 1;
    }
    
    serial_t *serial = serial_init();
    if (!serial) {
        fprintf(stderr, "Failed to initialize serial interface\n");
        pdp11_free(cpu);
        return 1;
    }
    
    disk_t *disk = disk_init(disk_file);
    if (!disk) {
        fprintf(stderr, "Failed to initialize disk\n");
        serial_free(serial);
        pdp11_free(cpu);
        return 1;
    }
    
    telnet_server_t *telnet = telnet_init(port);
    if (!telnet) {
        fprintf(stderr, "Failed to initialize telnet server\n");
        disk_free(disk);
        serial_free(serial);
        pdp11_free(cpu);
        return 1;
    }
    
    // Load bootstrap if requested
    if (use_bootstrap) {
        printf("Loading bootstrap program...\n");
        load_bootstrap(cpu);
    }
    
    printf("System ready. Connect via: telnet localhost %d\n", port);
    
    // Run emulation
    emulation_loop(cpu, serial, disk, telnet);
    
    // Cleanup
    printf("Shutting down...\n");
    telnet_free(telnet);
    disk_free(disk);
    serial_free(serial);
    pdp11_free(cpu);
    
    return 0;
}
