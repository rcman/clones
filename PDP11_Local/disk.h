#ifndef DISK_H
#define DISK_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* RK05 Disk Controller registers (typical addresses) */
#define RKDS  0177400  // Drive Status
#define RKER  0177402  // Error Register
#define RKCS  0177404  // Control Status
#define RKWC  0177406  // Word Count
#define RKBA  0177410  // Bus Address
#define RKDA  0177412  // Disk Address

/* Control Status bits */
#define RKCS_GO    0x0001  // Go
#define RKCS_FUNC  0x000E  // Function code
#define RKCS_MEX   0x0030  // Memory extension
#define RKCS_IE    0x0040  // Interrupt enable
#define RKCS_DONE  0x0080  // Done
#define RKCS_SSE   0x0100  // Stop on soft error
#define RKCS_RDY   0x0080  // Ready

/* Function codes */
#define FUNC_CTRL_RESET 0
#define FUNC_WRITE      1
#define FUNC_READ       2
#define FUNC_WRITE_CHK  3
#define FUNC_SEEK       4
#define FUNC_READ_CHK   5
#define FUNC_DRIVE_RST  6
#define FUNC_WRITE_LOCK 7

/* RK05 geometry */
#define RK05_CYLINDERS   203
#define RK05_TRACKS      2
#define RK05_SECTORS     12
#define RK05_SECTOR_SIZE 512
#define RK05_WORDS_PER_SECTOR (RK05_SECTOR_SIZE / 2)
#define RK05_TOTAL_SECTORS (RK05_CYLINDERS * RK05_TRACKS * RK05_SECTORS)
#define RK05_SIZE (RK05_TOTAL_SECTORS * RK05_SECTOR_SIZE)

typedef struct {
    char *filename;
    FILE *file;
    
    /* Controller registers */
    uint16_t rkds;
    uint16_t rker;
    uint16_t rkcs;
    uint16_t rkwc;
    uint16_t rkba;
    uint16_t rkda;
    
    bool ready;
} disk_t;

/* Function prototypes */
disk_t* disk_init(const char *filename);
void disk_free(disk_t *disk);
uint16_t disk_read_register(disk_t *disk, uint16_t addr);
void disk_write_register(disk_t *disk, uint16_t addr, uint16_t value, uint8_t *memory);
void disk_service(disk_t *disk, uint8_t *memory);

#endif // DISK_H
