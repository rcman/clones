#include "disk.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Initialize disk controller */
disk_t* disk_init(const char *filename) {
    disk_t *disk = calloc(1, sizeof(disk_t));
    if (!disk) return NULL;
    
    disk->filename = strdup(filename);
    disk->file = fopen(filename, "r+b");
    
    if (!disk->file) {
        // Try to create a new disk image
        disk->file = fopen(filename, "w+b");
        if (disk->file) {
            // Create empty disk image
            uint8_t zeros[RK05_SECTOR_SIZE];
            memset(zeros, 0, RK05_SECTOR_SIZE);
            for (int i = 0; i < RK05_TOTAL_SECTORS; i++) {
                fwrite(zeros, 1, RK05_SECTOR_SIZE, disk->file);
            }
            fflush(disk->file);
            rewind(disk->file);
        } else {
            free(disk->filename);
            free(disk);
            return NULL;
        }
    }
    
    disk->ready = true;
    disk->rkcs = RKCS_RDY;
    disk->rkds = 0x4000;  // Drive ready
    
    return disk;
}

/* Free disk resources */
void disk_free(disk_t *disk) {
    if (disk) {
        if (disk->file) {
            fflush(disk->file);
            fclose(disk->file);
        }
        if (disk->filename) free(disk->filename);
        free(disk);
    }
}

/* Read disk controller register */
uint16_t disk_read_register(disk_t *disk, uint16_t addr) {
    switch (addr) {
        case RKDS:
            return disk->rkds;
        case RKER:
            return disk->rker;
        case RKCS:
            return disk->rkcs;
        case RKWC:
            return disk->rkwc;
        case RKBA:
            return disk->rkba;
        case RKDA:
            return disk->rkda;
        default:
            return 0;
    }
}

/* Write disk controller register */
void disk_write_register(disk_t *disk, uint16_t addr, uint16_t value, uint8_t *memory) {
    switch (addr) {
        case RKCS:
            disk->rkcs = value;
            if (value & RKCS_GO) {
                // Execute command immediately (no timing simulation)
                uint16_t func = (value & RKCS_FUNC) >> 1;
                
                // Calculate disk position
                uint16_t cylinder = (disk->rkda >> 5) & 0xFF;
                uint16_t surface = (disk->rkda >> 4) & 1;
                uint16_t sector = disk->rkda & 0x0F;
                
                uint32_t disk_sector = (cylinder * RK05_TRACKS + surface) * RK05_SECTORS + sector;
                uint32_t disk_offset = disk_sector * RK05_SECTOR_SIZE;
                
                // Word count is negative (two's complement)
                int word_count = -(int16_t)disk->rkwc;
                if (word_count == 0) word_count = 65536;
                
                uint32_t byte_count = word_count * 2;
                uint16_t mem_addr = disk->rkba;
                
                switch (func) {
                    case FUNC_READ:
                        if (disk->file && disk_offset < RK05_SIZE) {
                            fseek(disk->file, disk_offset, SEEK_SET);
                            
                            for (int i = 0; i < (int)byte_count && mem_addr + i < 0xFFFF; i++) {
                                uint8_t byte = 0;
                                if (fread(&byte, 1, 1, disk->file) != 1) {
                                    break;  // Error or EOF
                                }
                                memory[mem_addr + i] = byte;
                            }
                        }
                        break;
                        
                    case FUNC_WRITE:
                        if (disk->file && disk_offset < RK05_SIZE) {
                            fseek(disk->file, disk_offset, SEEK_SET);
                            
                            for (int i = 0; i < (int)byte_count && mem_addr + i < 0xFFFF; i++) {
                                fwrite(&memory[mem_addr + i], 1, 1, disk->file);
                            }
                            fflush(disk->file);
                        }
                        break;
                        
                    case FUNC_CTRL_RESET:
                        disk->rker = 0;
                        disk->rkcs = RKCS_RDY;
                        disk->rkds = 0x4000;
                        break;
                }
                
                // Set done bit
                disk->rkcs |= RKCS_DONE;
                disk->rkcs &= ~RKCS_GO;
                disk->rkwc = 0;
            }
            break;
            
        case RKWC:
            disk->rkwc = value;
            break;
            
        case RKBA:
            disk->rkba = value;
            break;
            
        case RKDA:
            disk->rkda = value;
            break;
    }
}

/* Service disk (called periodically) */
void disk_service(disk_t *disk, uint8_t *memory) {
    (void)disk;
    (void)memory;
    // With no timing simulation, all disk operations complete immediately
    // in disk_write_register when GO bit is set
}
