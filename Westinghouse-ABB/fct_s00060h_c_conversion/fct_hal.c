/*
 * S00060-H Hardware Abstraction Layer
 * 
 * This file provides platform-specific implementations of hardware access.
 * Modify for your target platform (simulation, embedded, etc.)
 */

#include "fct_s00060h.h"
#include <stdio.h>
#include <stdlib.h>

/* Simulated memory map */
static uint8_t memory[0x10000];

/* Hardware register simulation */
static uint8_t ptm_ctl0 = 0;
static uint8_t ptm_ctl1 = 0;
static uint16_t ptm_tm1 = 0;
static uint16_t ptm_tm2 = 0;
static uint16_t ptm_tm3 = 0;
static uint8_t pia_port_a = 0;
static uint8_t pia_port_b = 0;
static uint8_t pia_ctl_a = 0;
static uint8_t pia_ctl_b = 0;

/* 8279 keyboard/display simulation */
static uint8_t kd_cmd = 0;
static uint8_t kd_data = 0;
static uint8_t kb_eirq = 0;
static uint8_t display_ram[16];

/* Configuration switches/jumpers */
static uint8_t testsw_val = 0x02;    /* Default: LMT-2/MCT mode */
static uint8_t cfgjpr_val = 0x02;    /* Match testsw */
static uint8_t rly_status_val = 0x00;


void hw_write_byte(uint16_t addr, uint8_t value)
{
    /* Handle special addresses */
    switch (addr) {
        case PTM_CTL0:
            ptm_ctl0 = value;
            break;
        case PTM_CTL1:
            ptm_ctl1 = value;
            break;
        case PTM_TM1:
            ptm_tm1 = (ptm_tm1 & 0x00FF) | ((uint16_t)value << 8);
            break;
        case PTM_TM1 + 1:
            ptm_tm1 = (ptm_tm1 & 0xFF00) | value;
            break;
        case PTM_TM2:
            ptm_tm2 = (ptm_tm2 & 0x00FF) | ((uint16_t)value << 8);
            break;
        case PTM_TM2 + 1:
            ptm_tm2 = (ptm_tm2 & 0xFF00) | value;
            break;
        case PTM_TM3:
            ptm_tm3 = (ptm_tm3 & 0x00FF) | ((uint16_t)value << 8);
            break;
        case PTM_TM3 + 1:
            ptm_tm3 = (ptm_tm3 & 0xFF00) | value;
            break;
        case PIA_PORT_A:
            pia_port_a = value;
            break;
        case PIA_CTL_A:
            pia_ctl_a = value;
            break;
        case PIA_PORT_B:
            pia_port_b = value;
            break;
        case PIA_CTL_B:
            pia_ctl_b = value;
            break;
        case KD_CMD:
            kd_cmd = value;
            /* Handle 8279 commands */
            if ((value & 0xF0) == 0x90) {
                /* Start write to display RAM */
            } else if ((value & 0xF0) == 0x50) {
                /* Read key data - simulate keypress */
            }
            break;
        case KD_DATA:
            kd_data = value;
            /* Store to display RAM based on current position */
            break;
        default:
            memory[addr] = value;
            break;
    }
}


uint8_t hw_read_byte(uint16_t addr)
{
    switch (addr) {
        case PTM_CTL0:
            return ptm_ctl0;
        case PTM_CTL1:
            /* Bit 0 indicates timer expired - simulate always expired */
            return ptm_ctl1 | 0x01;
        case PIA_PORT_A:
            return pia_port_a;
        case PIA_CTL_A:
            return pia_ctl_a;
        case PIA_PORT_B:
            return pia_port_b;
        case PIA_CTL_B:
            return pia_ctl_b;
        case TESTSW:
            return testsw_val;
        case KB_EIRQ:
            return kb_eirq;
        case RLY_STATUS:
            return rly_status_val;
        case CFG_JPR:
            return cfgjpr_val;
        case KD_DATA:
            return kd_data;
        case KD_CMD:
            return kd_cmd;
        default:
            return memory[addr];
    }
}


void hw_write_word(uint16_t addr, uint16_t value)
{
    hw_write_byte(addr, (value >> 8) & 0xFF);
    hw_write_byte(addr + 1, value & 0xFF);
}


uint16_t hw_read_word(uint16_t addr)
{
    return ((uint16_t)hw_read_byte(addr) << 8) | hw_read_byte(addr + 1);
}


/* Simulation helper functions */

void sim_set_key_pressed(uint8_t key_code)
{
    kd_data = key_code;
    kb_eirq |= 0x01;  /* Set key available flag */
}

void sim_clear_key(void)
{
    kb_eirq &= ~0x01;
}

void sim_set_relay_status(uint8_t status)
{
    rly_status_val = status;
}

void sim_set_mode_switch(uint8_t mode)
{
    testsw_val = mode & 0x03;
    cfgjpr_val = mode & 0x03;  /* Match jumpers to switch */
}

void sim_print_display(FCT_State *state)
{
    printf("Display: ");
    for (int i = 7; i >= 0; i--) {
        if (state->readout[i] == 0x0F) {
            printf(" ");
        } else {
            printf("%X", state->readout[i]);
        }
    }
    printf(" | Test: %X%X", state->testTen, state->testOne);
    printf(" | Stat: %s", (state->ledStat == LED_PASS) ? "PASS" : 
                          (state->ledStat == LED_FAIL) ? "FAIL" : "---");
    printf("\n");
}


/* Simple test main */
#ifdef TEST_MAIN
int main(void)
{
    FCT_State state;
    
    printf("FCT S00060-H Simulator\n");
    printf("======================\n\n");
    
    /* Set mode to LMT-2/MCT */
    sim_set_mode_switch(0x02);
    
    /* Initialize */
    printf("Initializing...\n");
    fct_reset(&state);
    
    sim_print_display(&state);
    
    printf("\nSimulation initialized. State ready for testing.\n");
    
    /* In a real simulation, you would:
     * 1. Call nmi_interrupt() periodically to simulate timer interrupts
     * 2. Set sim_set_key_pressed() to simulate keypresses
     * 3. Call fct_main_loop() or individual test functions
     */
    
    return 0;
}
#endif
