#ifndef PDP11_H
#define PDP11_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* PDP-11 has 8 general purpose registers (R0-R7)
 * R6 = SP (Stack Pointer)
 * R7 = PC (Program Counter)
 */
#define NUM_REGISTERS 8
#define REG_SP 6
#define REG_PC 7

/* Processor Status Word bits */
#define PSW_C  0x0001  // Carry
#define PSW_V  0x0002  // Overflow
#define PSW_Z  0x0004  // Zero
#define PSW_N  0x0008  // Negative
#define PSW_T  0x0010  // Trap
#define PSW_PRI 0x00E0 // Priority (bits 5-7)
#define PSW_UNUSED 0x0F00
#define PSW_MODE 0xC000 // Current/Previous mode

/* Memory sizes */
#define MAX_MEMORY (4 * 1024 * 1024)  // 4MB max for PDP-11/70
#define IOPAGE_BASE 0760000U  // Start of I/O page (octal)
#define IOPAGE_SIZE 0020000   // 8KB I/O page

/* Addressing mode masks */
#define MODE_MASK 0070
#define REG_MASK  0007

/* Addressing modes */
#define MODE_REG       0   // Register
#define MODE_REG_DEF   1   // Register Deferred
#define MODE_AUTOINC   2   // Autoincrement
#define MODE_AUTOINC_DEF 3 // Autoincrement Deferred
#define MODE_AUTODEC   4   // Autodecrement
#define MODE_AUTODEC_DEF 5 // Autodecrement Deferred
#define MODE_INDEX     6   // Index
#define MODE_INDEX_DEF 7   // Index Deferred

/* CPU state */
typedef struct {
    uint16_t regs[NUM_REGISTERS];  // General purpose registers
    uint16_t psw;                   // Processor Status Word
    uint8_t *memory;                // Main memory
    uint32_t memory_size;           // Size of memory
    bool halted;                    // CPU halted flag
    bool wait_state;                // CPU waiting for interrupt
    uint32_t instr_count;           // Instruction counter
} pdp11_t;

/* Function prototypes */
pdp11_t* pdp11_init(uint32_t memory_size);
void pdp11_free(pdp11_t *cpu);
void pdp11_reset(pdp11_t *cpu);
int pdp11_step(pdp11_t *cpu);
void pdp11_load_memory(pdp11_t *cpu, uint16_t addr, const uint8_t *data, size_t len);

/* Memory access */
uint16_t mem_read_word(pdp11_t *cpu, uint16_t addr);
uint8_t mem_read_byte(pdp11_t *cpu, uint16_t addr);
void mem_write_word(pdp11_t *cpu, uint16_t addr, uint16_t value);
void mem_write_byte(pdp11_t *cpu, uint16_t addr, uint8_t value);

/* Condition code helpers */
void set_cc_byte(pdp11_t *cpu, uint8_t result);
void set_cc_word(pdp11_t *cpu, uint16_t result);
void set_cc_nz(pdp11_t *cpu, uint16_t result);

#endif // PDP11_H
