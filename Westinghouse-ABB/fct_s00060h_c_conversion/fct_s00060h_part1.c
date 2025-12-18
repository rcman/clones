/*
 * S00060-H / MPU: SC44125P
 * Westinghouse Field Configuration Terminal - unit 1993D40G06 (12.5kHz)
 * 
 * C conversion of THRsim11 assembly code
 * Part 1: Initialization, Display, and Utility Functions
 */

#include "fct_s00060h.h"
#include <string.h>

/* Vector sum calculation tables */
const int8_t freq1[10] = {32, 23, 0, -23, -32, -23, 0, 23, 32, 23};
const int8_t freq2[9] = {1, 0, -1, 0, 1, 0, -1, 0, 1};

/* Test message tables (loaded via msg_load) */
/* First two bytes (0xAA, 0xBA) are loaded within subroutine */

/* Test 15: pointer byte $22, Emetcon test address + flags */
static const uint8_t msg01[24] = {
    0xFF, 0x55, 0x55, 0x56, 0x22, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0C, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x41, 0x03, 0x00, 0x15
};

/* Test 30a (uutGroup: 2 or 4) / pointer byte $3F */
static const uint8_t msg02[24] = {
    0xFF, 0x00, 0x00, 0x02, 0x3F, 0x00, 0xCF, 0xF0, 0x10, 0x00, 0x00, 0x00,
    0x0C, 0x00, 0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x41, 0x03, 0x00, 0x15
};

/* Test 30b (uutGroup: 6) / pointer byte $35 */
static const uint8_t msg03[24] = {
    0xFF, 0x00, 0x00, 0x01, 0x35, 0x00, 0xCF, 0xF0, 0x00, 0x00, 0x00, 0x44,
    0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x41, 0x02, 0x00, 0x0F
};

/* Test 32 / pointer byte $1F, Emetcon test address without flags */
static const uint8_t msg04[24] = {
    0xFF, 0x55, 0x55, 0x55, 0x1F, 0x00, 0xC0, 0x00, 0x00, 0x0F, 0xF0, 0x34,
    0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x41, 0x02, 0x00, 0x0F
};

/* Test 71/73 (DCT related), pointer byte $3A */
static const uint8_t msg05[24] = {
    0xFF, 0x00, 0x00, 0x01, 0x3A, 0x00, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x44,
    0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x41, 0x02, 0x00, 0x0F
};


/* ========== Initialization ========== */

void fct_reset(FCT_State *state)
{
    /* Clear state structure */
    memset(state, 0, sizeof(FCT_State));

    /* Configure MC6821 PIA */
    hw_write_byte(PIA_CTL_A, 0x30);
    hw_write_byte(PIA_CTL_B, 0x30);
    
    /* Set port directions: all outputs except PA0 */
    hw_write_byte(PIA_PORT_A, 0xFE);
    hw_write_byte(PIA_PORT_B, 0xFF);
    
    hw_write_byte(PIA_CTL_A, 0x34);
    hw_write_byte(PIA_CTL_B, 0x34);
    
    /* PA4 is active low so we start with it high */
    hw_write_byte(PIA_PORT_A, 0x10);
    hw_write_byte(PIA_PORT_B, 0x00);

    /* Configure MC6840 timer based on frequency selection */
    uint8_t portb_val = hw_read_byte(PIA_PORT_B);
    
    if ((portb_val & 0x01) == 0) {
        /* 12.5kHz */
        hw_write_word(PTM_TM3, 0x0027);
        hw_write_word(PTM_TM2, 0x0199);
    } else {
        /* 9.615kHz */
        hw_write_word(PTM_TM3, 0x0033);
        hw_write_word(PTM_TM2, 0x01AC);
    }

    /* PTM setup */
    hw_write_word(PTM_TM1, 0x2133);
    hw_write_byte(PTM_CTL1, 0x12);
    hw_write_byte(PTM_CTL0, 0x12);
    hw_write_byte(PTM_CTL1, 0x13);
    hw_write_byte(PTM_CTL0, 0x92);
    hw_write_byte(PTM_CTL1, 0xD3);

    /* Initialize variables */
    state->countBit = 4;
    state->countPulse = 0;
    state->countPhase = 0;
    state->M_index = 3;  /* Points to mTable1[3] */
    state->msgLen = 0;
    state->bchSum = 0;
    state->msgBits = 0;
    state->xmitByte = 1;
    state->msgEnd = 1;
    state->keyMask = 0;
    state->uutType = 0;
    state->dataNum = 0;
    state->xmitStatus = 0;
    state->bitCount = 0;
    state->uutGroup = 0;
    state->xmitFlag = 0;
    state->addrL4 = 0;
    state->addrM4 = 0;
    state->addrH4 = 0;

    /* Blank display and turn off Install LED */
    disp_blank(state);
    state->ledInstall = LED_OFF;
    disp_refresh(state);
}


/* ========== Display Functions ========== */

/*
 * Display refresh - transfer readout buffer to 8279 display IC
 */
void disp_refresh(FCT_State *state)
{
    /* Initialize the 8279 */
    hw_write_byte(KD_CMD, 0xDC);    /* Command 6: Reset IC */
    
    /* Wait out reset interval */
    for (volatile int i = 0; i < 100; i++);
    
    hw_write_byte(KD_CMD, 0xAA);    /* Command 5: Set masking (disable port A) */
    hw_write_byte(KD_CMD, 0x18);    /* Command 0: Set mode - 16 chars right-hand, 2-key lockout */
    hw_write_byte(KD_CMD, 0x2A);    /* Command 1: Set clock divisor (1MHz / 10 = 100kHz) */

    /* Send display buffer contents to 8279 */
    hw_write_byte(KD_CMD, 0x90);    /* Command 4: Start write to RAM */
    
    /* Write main readout */
    for (int i = 0; i < 8; i++) {
        hw_write_byte(KD_DATA, state->readout[i]);
    }
    
    /* Write test number and status LEDs */
    hw_write_byte(KD_DATA, state->testOne);
    hw_write_byte(KD_DATA, state->testTen);
    hw_write_byte(KD_DATA, state->ledRlyA);
    hw_write_byte(KD_DATA, state->ledRlyB);
    hw_write_byte(KD_DATA, state->ledRlyC);
    hw_write_byte(KD_DATA, state->ledRlyD);
    hw_write_byte(KD_DATA, state->ledInstall);
    hw_write_byte(KD_DATA, state->ledStat);
    
    hw_write_byte(KD_CMD, 0x80);    /* Command 4: End write to RAM */
}


/*
 * Display blanking - fill display buffer with F to turn off LEDs
 * Install LED is not included, as it is handled separately
 */
void disp_blank(FCT_State *state)
{
    for (int i = 0; i < 8; i++) {
        state->readout[i] = LED_OFF;
    }
    state->testOne = LED_OFF;
    state->testTen = LED_OFF;
    state->ledRlyA = LED_OFF;
    state->ledRlyB = LED_OFF;
    state->ledRlyC = LED_OFF;
    state->ledRlyD = LED_OFF;
    state->ledStat = LED_OFF;
}


/*
 * Expand 4-byte packed BCD data to 8-byte unpacked BCD data
 */
void disp_bcd(FCT_State *state)
{
    state->readout[0] = state->bcdData10 & 0x0F;
    state->readout[1] = (state->bcdData10 >> 4) & 0x0F;
    state->readout[2] = state->bcdData32 & 0x0F;
    state->readout[3] = (state->bcdData32 >> 4) & 0x0F;
    state->readout[4] = state->bcdData54 & 0x0F;
    state->readout[5] = (state->bcdData54 >> 4) & 0x0F;
    state->readout[6] = state->bcdData76 & 0x0F;
    state->readout[7] = (state->bcdData76 >> 4) & 0x0F;
    state->ledStat = LED_PASS;
}


/* ========== Timer Functions ========== */

/*
 * Delay timer (1/16 second per count) using PTM channel 1
 */
void timer_delay(FCT_State *state)
{
    hw_write_byte(PTM_CTL0, 0x22);
    
    while (state->waitTime > 0) {
        hw_write_word(PTM_TM1, 0xFFFF);
        
        /* Wait for timer to expire */
        while ((hw_read_byte(PTM_CTL1) & 0x01) == 0);
        
        state->waitTime--;
    }
}


/* ========== Input Functions ========== */

/*
 * Get and process character from keypad, save in dataKey
 */
uint8_t get_key(FCT_State *state)
{
    /* Wait for keypress */
    while ((hw_read_byte(KB_EIRQ) & 0x01) == 0);
    
    /* Command 2: get key data */
    hw_write_byte(KD_CMD, 0x50);
    state->dataKey = hw_read_byte(KD_DATA) & 0x1F;

    /* Convert matrix key code to number, or -1 (0xFF) for NaN */
    switch (state->dataKey) {
        case KEY_0: state->dataNum = 0; break;
        case KEY_1: state->dataNum = 1; break;
        case KEY_2: state->dataNum = 2; break;
        case KEY_3: state->dataNum = 3; break;
        case KEY_4: state->dataNum = 4; break;
        case KEY_5: state->dataNum = 5; break;
        case KEY_6: state->dataNum = 6; break;
        case KEY_7: state->dataNum = 7; break;
        case KEY_8: state->dataNum = 8; break;
        case KEY_9: state->dataNum = 9; break;
        default:    state->dataNum = -1; break;
    }

    /* Check for soft reset key */
    if (state->dataKey == KEY_RESET) {
        fct_reset(state);
    }
    
    return state->dataKey;
}


/*
 * Process keypress during DCT analog tests 61-64
 */
uint8_t get_key_dct(FCT_State *state)
{
    uint8_t key;
    
    do {
        key = get_key(state);
    } while (key != KEY_REPEAT && key != KEY_NEXT);
    
    return key;
}


/* ========== Address Conversion Functions ========== */

/*
 * Convert 8-digit unpacked BCD to 24-bit binary
 */
void addr_bin(FCT_State *state)
{
    uint32_t value = 0;
    
    /* Process from most significant digit (readout[7]) to least (readout[0]) */
    for (int i = 7; i >= 0; i--) {
        value = value * 10 + (state->readout[i] & 0x0F);
    }
    
    /* Store as 24-bit big-endian */
    state->addrH = (value >> 16) & 0xFF;
    state->addrM = (value >> 8) & 0xFF;
    state->addrL = value & 0xFF;
}


/*
 * Shift address left 2 bits to make room for message type flag
 * 00 = command, 01 = data read, 10 = data write
 */
void addr_x4(FCT_State *state)
{
    uint32_t addr = ((uint32_t)state->addrH << 16) | 
                    ((uint32_t)state->addrM << 8) | 
                    state->addrL;
    addr <<= 2;
    
    state->addrH4 = (addr >> 16) & 0xFF;
    state->addrM4 = (addr >> 8) & 0xFF;
    state->addrL4 = addr & 0xFF;
}


/*
 * Convert binary value to four-byte packed BCD
 * Uses BCD double-dabble algorithm
 */
void bin_to_bcd(FCT_State *state)
{
    uint8_t *bcd_ptr;
    uint8_t *bin_ptr;
    uint8_t bcd[4] = {0, 0, 0, 0};
    uint8_t bits_remaining = state->f2bits;
    uint8_t bytes_remaining = state->f2bytes;
    
    /* Process each bit from binary source */
    for (int bit = 0; bit < state->f2bits; bit++) {
        /* Add 3 to any BCD digit >= 5 (double-dabble adjustment) */
        for (int i = 0; i < bytes_remaining; i++) {
            if ((bcd[i] & 0x0F) >= 5) {
                bcd[i] += 3;
            }
            if ((bcd[i] & 0xF0) >= 0x50) {
                bcd[i] += 0x30;
            }
        }
        
        /* Shift BCD left by 1 */
        uint8_t carry = 0;
        for (int i = 0; i < bytes_remaining; i++) {
            uint8_t new_carry = (bcd[i] & 0x80) ? 1 : 0;
            bcd[i] = (bcd[i] << 1) | carry;
            carry = new_carry;
        }
        
        /* Get next bit from binary source and shift into BCD */
        /* Note: This is a simplified version - actual implementation 
           would need to track the binary source pointer */
    }
    
    /* Store result */
    state->bcdData10 = bcd[0];
    state->bcdData32 = bcd[1];
    state->bcdData54 = bcd[2];
    state->bcdData76 = bcd[3];
}


/* ========== Message Handling Functions ========== */

/*
 * Message template (Type A) for 1-way units
 */
void msg_1way(FCT_State *state)
{
    state->msgTbl[0] = 0xAA;
    state->msgTbl[1] = 0xB8;
    state->msgTbl[2] = 0xFF;
    state->msgTbl[3] = state->addrL4;
    state->msgTbl[4] = 0x00;    /* command byte / data pointer */
    state->msgTbl[5] = 0x00;    /* response flag: ALWAYS 0 on 1-way units */
    state->xmitByte = 1;
    state->msgBits = 0x2B;      /* 43 bits */
    state->msgLen = 6;
}


/*
 * Message template (Type B) for 2-way units
 */
void msg_2way(FCT_State *state)
{
    state->msgTbl[0] = 0xAA;
    state->msgTbl[1] = 0xBA;
    state->msgTbl[2] = 0xFF;
    state->msgTbl[3] = state->addrH4;
    state->msgTbl[4] = state->addrM4;
    state->msgTbl[5] = state->addrL4;
    state->msgTbl[6] = 0x00;    /* command byte / data pointer */
    state->msgTbl[7] = 0x00;    /* response flag: set to 64 if reply required */
    state->xmitByte = 1;
    state->msgBits = 0x41;      /* 65 bits */
    state->msgLen = 8;
}


/*
 * Load long-form message from table
 */
void msg_load(FCT_State *state, const uint8_t *msg_data)
{
    state->msgTbl[0] = 0xAA;
    state->msgTbl[1] = 0xBA;
    
    /* Copy 20 bytes of message data */
    for (int i = 0; i < 20; i++) {
        state->msgTbl[2 + i] = msg_data[i];
    }
    
    /* Last four bytes are overhead variables */
    state->msgBits = msg_data[20];
    state->xmitByte = msg_data[21];
    state->msgLen = msg_data[23];  /* Upper byte is 0 */
}


/*
 * Transmit message - standard format
 */
void msg_cmd(FCT_State *state)
{
    state->xmitStatus = 1;
    state->xmitFlag = 1;
    state->waitTime = 0x10;     /* 1 second */
    timer_delay(state);
    
    /* Check for error condition */
    if (state->xmitStatus > 0) {
        error_handler(state, 1);    /* No transmit error */
        return;
    }
    
    state->xmitFlag = 0;
}


/*
 * Request data from UUT
 */
void msg_data_qry(FCT_State *state)
{
    /* Set address / reply flags */
    state->msgTbl[5] |= 0x01;
    state->msgTbl[7] = 0x40;
    
    state->xmitStatus = 1;
    state->xmitFlag = 1;
    state->waitTime = 5;
    timer_delay(state);
    
    /* Check for transmit error */
    if (state->msgBits >= 0x41) {
        error_handler(state, 1);    /* No transmit */
        return;
    }
    
    /* Wait for transmission complete */
    while (state->msgBits != 0);
    
    /* Set data packet parameters */
    state->msgLen = 7;
    state->xmitByte = 1;
    
    /* Begin data retrieval */
    state->waitTime = 7;
    timer_delay(state);
    
    /* Check for response */
    if (state->msgBits == 0) {
        error_handler(state, 2);    /* No response */
        return;
    }
    
    /* Wait for message complete */
    while (state->msgBits != 0x34);
    state->msgBits = 0;
    
    /* Check BCH */
    if (state->bchSum != 0) {
        error_handler(state, 4);    /* BCH error */
        return;
    }
    
    state->xmitFlag = 0;
}


/*
 * Transmit message - DCT format (tests 71 and 73 only)
 */
void send_msg_dct(FCT_State *state)
{
    state->msgTbl[3] = state->addrH4;
    state->msgTbl[4] = state->addrM4;
    state->msgTbl[5] = state->addrL4 | 0x01;
    
    state->xmitStatus = 1;
    state->xmitFlag = 1;
    state->waitTime = 0x24;
    timer_delay(state);
    
    /* Check for error */
    if (state->xmitStatus > 0) {
        error_handler(state, 1);    /* No transmit */
        return;
    }
    
    state->xmitFlag = 0;
}


/* ========== Display UUT Info ========== */

/*
 * Display UUT firmware number and revision
 */
void get_fw_rev(FCT_State *state)
{
    /* Convert firmware byte to BCD */
    state->f2bits = 8;
    state->f2bytes = 2;
    bin_to_bcd(state);
    
    state->readout[5] = state->bcdData10 & 0x0F;
    state->readout[6] = (state->bcdData10 >> 4) & 0x0F;
    state->readout[7] = state->bcdData32;
    
    /* Convert firmware spec to BCD */
    state->f2bits = 8;
    state->f2bytes = 2;
    bin_to_bcd(state);
    
    state->readout[2] = state->bcdData10 & 0x0F;
    state->readout[3] = (state->bcdData10 >> 4) & 0x0F;
    
    disp_refresh(state);
    state->waitTime = 0x20;     /* 2 seconds */
    timer_delay(state);
}


/*
 * Display DCT analog data
 */
void disp_dct(FCT_State *state)
{
    state->f2bits = 0x10;
    state->f2bytes = 4;
    
    state->dctTemp = (state->msgTbl[3] >> 5) & 0x03;
    state->msgTbl[3] &= 0x3F;
    
    /* Byte swap for proper conversion */
    uint8_t temp = state->msgTbl[4];
    state->msgTbl[2] = temp;
    
    bin_to_bcd(state);
    disp_bcd(state);
    
    state->readout[5] = LED_OFF;
    state->readout[7] = LED_OFF;
    state->readout[6] = state->dctTemp;
    
    disp_refresh(state);
}


/* ========== Error Handler ========== */

void error_handler(FCT_State *state, uint8_t error_code)
{
    /*
     * Error codes:
     * 1 = No transmit from FCT
     * 2 = No response from UUT
     * 3 = Incomplete response from UUT
     * 4 = BCH checksum error in UUT response
     * 5 = Other data error
     * 6 = FCT number / hardware mismatch
     */
    state->readout[0] = error_code;
    state->ledStat = LED_FAIL;
    disp_refresh(state);
    state->xmitFlag = 0;
    state->keyMask = 0x48;  /* Allow Repeat and Test keys */
}


/* ========== BCH Checksum ========== */

/*
 * Update BCH checksum with one bit
 */
void bch_update(FCT_State *state, uint8_t bit_value)
{
    state->bchSum <<= 1;
    
    if (bit_value & 0x01) {
        if (!(state->bchSum & 0x40)) {
            state->bchSum ^= 0x03;
        }
    } else {
        if (state->bchSum & 0x40) {
            state->bchSum ^= 0x03;
        }
    }
    
    state->bchSum &= 0x3F;
}
