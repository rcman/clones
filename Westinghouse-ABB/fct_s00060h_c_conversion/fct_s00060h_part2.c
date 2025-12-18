/*
 * S00060-H / MPU: SC44125P
 * Westinghouse Field Configuration Terminal - unit 1993D40G06 (12.5kHz)
 * 
 * C conversion of THRsim11 assembly code
 * Part 2: Main Loop and Test Suites
 */

#include "fct_s00060h.h"


/* Forward declarations for internal test functions */
static void test01_validate_switch(FCT_State *state);
static void test15_download_address(FCT_State *state);
static void test20_read_verify_address(FCT_State *state);
static void test25_get_hardware_id(FCT_State *state);
static void test25b_condensed(FCT_State *state);
static void test30_setup(FCT_State *state);
static void test35_timed_relays_1way(FCT_State *state);
static void test35_timed_relays_2way(FCT_State *state);
static void test40_latched_relay_1way(FCT_State *state);
static void test40_latched_relay_2way(FCT_State *state);
static void test50_toggle_test_mode(FCT_State *state);
static void test61_dct_analog(FCT_State *state);


/* ========== Main Loop ========== */

void fct_main_loop(FCT_State *state)
{
    uint8_t key;
    uint8_t checksum;
    int i;
    
    /* Step 1 - Validate switch and jumper settings */
    test01_validate_switch(state);
    
    /* Step 2 - setup / full 7+1 display */
    disp_blank(state);
    state->testOne = 0x02;
    state->testTen = 0x00;
    
    for (i = 0; i < 8; i++) {
        state->readout[i] = 0;
    }
    
    disp_refresh(state);
    state->keyMask = 0xAA;  /* Enable Check, Repeat, Enter, number keys */
    
    /* Main keypad loop */
    while (1) {
        key = get_key(state);
        
        /* Check key handling based on keyMask */
        
        /* Check key (0x80 mask) */
        if ((state->keyMask & 0x80) && (state->dataKey == KEY_CHECK)) {
            /* Clear display based on keyMask bits 0/1 */
            state->readout[0] = 0;
            state->readout[1] = 0;
            
            uint8_t mask_bits = state->keyMask & 0x03;
            if (mask_bits != 0) {
                state->readout[2] = 0;
                if (mask_bits != 1) {
                    for (i = 3; i < 8; i++) {
                        state->readout[i] = 0;
                    }
                }
            }
            disp_refresh(state);
            continue;
        }
        
        /* Repeat key (0x40 mask) */
        if ((state->keyMask & 0x40) && (state->dataKey == KEY_REPEAT)) {
            state->keyMask = 0;
            if (state->uutType == UUT_LMT1XX) {
                test35_timed_relays_1way(state);
            } else if (state->testSet == 8) {
                test25b_condensed(state);
            } else {
                test_2way_suite(state);
            }
            continue;
        }
        
        /* Enter key (0x20 mask) */
        if ((state->keyMask & 0x20) && (state->dataKey == KEY_ENTER)) {
            /* Check display length then do checksum */
            int start_idx = (state->readout[7] == LED_OFF) ? 2 : 7;
            
            checksum = 0;
            for (i = start_idx; i >= 0; i--) {
                checksum += state->readout[i];
                /* DAA - decimal adjust (simplified) */
                if ((checksum & 0x0F) > 9) {
                    checksum += 6;
                }
            }
            checksum &= 0x0F;
            
            if (checksum != state->readout[0]) {
                state->ledStat = LED_FAIL;
                disp_refresh(state);
                continue;
            }
            
            /* Checksum was good - route based on keyMask bits */
            if (state->keyMask & 0x01) {
                /* Process FCT control number */
                state->rlyNum = state->readout[2] & 0x07;
                state->fctCtlNum = state->readout[1] & 0x03;
                if (state->fctCtlNum != 0) {
                    state->rlyNum--;
                }
                
                state->installFlag = 0;
                
                if (state->uutType == UUT_LMT1XX) {
                    /* Skip to Step 5 */
                    goto step5_setup;
                }
                
                /* Step 4 - setup / single 1+1 display */
                disp_blank(state);
                state->testOne = 0x04;
                state->testTen = 0x00;
                state->ledStat = LED_PASS;
                disp_refresh(state);
                state->keyMask = 0xA0;
                continue;
            }
            
            if (state->keyMask & 0x02) {
                /* Step 2 - process data / convert BCD to binary */
                addr_bin(state);
                
                /* Step 3 - setup / abbreviated 2+1 display */
                disp_blank(state);
                state->testOne = 0x03;
                state->testTen = 0x00;
                state->readout[0] = 0;
                state->readout[1] = 0;
                state->readout[2] = 0;
                state->ledStat = LED_PASS;
                disp_refresh(state);
                state->keyMask = 0xA9;
                continue;
            }
            
            /* Process install/test mode selection */
            if (state->readout[1] == 1) {        /* Install */
                state->testSet = 1;
                state->installFlag = 1;
            } else if (state->readout[1] == 3) { /* Test */
                state->testSet = 2;
            } else if (state->readout[1] == 7) { /* Read/test */
                state->testSet = 0;
            } else if (state->readout[1] == 9) { /* DCT */
                state->testSet = 3;
            } else {
                state->ledStat = LED_FAIL;
                disp_refresh(state);
                continue;
            }
            
        step5_setup:
            /* Set Install LED */
            state->ledInstall = (state->installFlag) ? LED_ALL_ON : LED_OFF;
            
            /* Step 5 - setup / display test number only */
            disp_blank(state);
            state->testOne = 0x05;
            state->testTen = 0x00;
            state->ledStat = LED_PASS;
            disp_refresh(state);
            state->keyMask = 0x10;  /* Only Next key enabled */
            continue;
        }
        
        /* Next key (0x10 mask) */
        if ((state->keyMask & 0x10) && (state->dataKey == KEY_NEXT)) {
            state->keyMask = 0;
            if (state->uutType == UUT_LMT1XX) {
                test35_timed_relays_1way(state);
            } else if (state->testSet == 8) {
                test25b_condensed(state);
            } else {
                test_2way_suite(state);
            }
            continue;
        }
        
        /* Test key (0x08 mask) */
        if ((state->keyMask & 0x08) && (state->dataKey == KEY_TEST)) {
            if (state->testOne == 0x02 && state->testTen == 0x00) {
                /* Check if any digits entered */
                checksum = 0;
                for (i = 7; i >= 0; i--) {
                    checksum += state->readout[i];
                }
                
                if (checksum != 0) {
                    /* Validate checksum */
                    if ((checksum & 0x0F) == state->readout[0]) {
                        state->ledStat = LED_PASS;
                        disp_refresh(state);
                        state->waitTime = 4;
                        timer_delay(state);
                        state->testSet = 8;
                        addr_bin(state);
                        addr_x4(state);
                        test25b_condensed(state);
                        continue;
                    } else {
                        state->ledStat = LED_FAIL;
                        disp_refresh(state);
                        continue;
                    }
                }
            }
            lamp_test(state);
            continue;
        }
        
        /* Was a number entered? */
        if (state->dataNum < 0) {
            continue;
        }
        
        /* Shift display and add number based on keyMask bits */
        if (state->keyMask & 0x02) {
            /* Full 8-digit shift */
            for (i = 7; i > 0; i--) {
                state->readout[i] = state->readout[i-1];
            }
        } else if (state->keyMask & 0x01) {
            /* 3-digit shift */
            state->readout[2] = state->readout[1];
            state->readout[1] = state->readout[0];
        }
        
        state->readout[0] = state->dataNum;
        disp_refresh(state);
    }
}


/* ========== Test 01: Validate Switch and Jumper Settings ========== */

static void test01_validate_switch(FCT_State *state)
{
    state->testOne = 0x01;
    state->testTen = 0x00;
    disp_refresh(state);
    
    state->waitTime = 4;
    timer_delay(state);
    
    while (1) {
        uint8_t cfg = hw_read_byte(CFG_JPR) & 0x03;
        uint8_t sw = hw_read_byte(TESTSW) & 0x03;
        
        if (cfg == sw) {
            break;
        }
        
        state->ledStat = LED_FAIL;
        disp_refresh(state);
        state->waitTime = 4;
        timer_delay(state);
    }
    
    state->ledStat = LED_OFF;
    disp_refresh(state);
    
    /* Set uutType based on switch position */
    uint8_t sw = hw_read_byte(TESTSW) & 0x03;
    if (sw == 3) {
        sw = 4;  /* DCT */
    }
    state->uutType = sw;
}


/* ========== Lamp Test ========== */

void lamp_test(FCT_State *state)
{
    /* Light all LED segments */
    for (int i = 0; i < 8; i++) {
        state->readout[i] = LED_ALL_ON;
    }
    state->testOne = LED_ALL_ON;
    state->testTen = LED_ALL_ON;
    state->ledRlyA = LED_ALL_ON;
    state->ledRlyB = LED_ALL_ON;
    state->ledRlyC = LED_ALL_ON;
    state->ledRlyD = LED_ALL_ON;
    state->ledInstall = LED_ALL_ON;
    state->ledStat = LED_ALL_ON;
    disp_refresh(state);
    
    state->waitTime = 0x2E;     /* ~2.9 seconds */
    timer_delay(state);
    
    /* Now display firmware revision */
    disp_blank(state);
    state->ledInstall = 0xFF;
    state->readout[2] = LED_ALL_ON;
    state->readout[5] = 0;
    state->readout[6] = 6;      /* Revision H = 0x08, but showing 060 */
    state->readout[7] = 0;
    disp_refresh(state);
    
    state->waitTime = 0x24;     /* ~2.25 seconds */
    timer_delay(state);
    
    /* Reboot */
    fct_reset(state);
}


/* ========== 1-Way Test Suite ========== */

void test_1way_suite(FCT_State *state)
{
    test35_timed_relays_1way(state);
}


/* Test 35 (1-way): Test timed relays (A - D) */
static void test35_timed_relays_1way(FCT_State *state)
{
    disp_blank(state);
    state->testOne = 0x05;
    state->testTen = 0x03;
    disp_refresh(state);
    
    /* Send message to UUT - shed relays */
    msg_1way(state);
    state->msgTbl[4] = 0xE0;
    msg_cmd(state);
    
    /* Wait and flash test LEDs */
    for (int i = 0; i < 8; i++) {
        state->waitTime = 0x10;
        timer_delay(state);
        state->ledStat = LED_ALL_ON;
        disp_refresh(state);
        state->waitTime = 0x10;
        timer_delay(state);
        state->ledStat = LED_OFF;
        disp_refresh(state);
    }
    
    state->rlyTemp = hw_read_byte(RLY_STATUS);
    
    /* Send message to UUT - restore relays */
    msg_1way(state);
    state->waitTime = 0x10;
    msg_cmd(state);
    
    /* Flash test LEDs again */
    for (int i = 0; i < 16; i++) {
        state->waitTime = 0x10;
        timer_delay(state);
        state->ledStat = LED_ALL_ON;
        disp_refresh(state);
        state->waitTime = 0x10;
        timer_delay(state);
        state->ledStat = LED_OFF;
        disp_refresh(state);
    }
    
    /* Evaluate relays */
    state->rlyError = 0;
    uint8_t rly_status = hw_read_byte(RLY_STATUS);
    state->rlyBits = rly_status ^ state->rlyTemp;
    
    /* Check each relay based on rlyNum */
    if (state->rlyNum >= 4) {
        state->ledRlyD = (state->rlyBits & 0x08) ? LED_PASS : LED_FAIL;
        if (!(state->rlyBits & 0x08)) state->rlyError++;
    }
    if (state->rlyNum >= 3) {
        state->ledRlyC = (state->rlyBits & 0x04) ? LED_PASS : LED_FAIL;
        if (!(state->rlyBits & 0x04)) state->rlyError++;
    }
    if (state->rlyNum >= 2) {
        state->ledRlyB = (state->rlyBits & 0x02) ? LED_PASS : LED_FAIL;
        if (!(state->rlyBits & 0x02)) state->rlyError++;
    }
    if (state->rlyNum >= 1) {
        state->ledRlyA = (state->rlyBits & 0x01) ? LED_PASS : LED_FAIL;
        if (!(state->rlyBits & 0x01)) state->rlyError++;
    }
    
    state->ledStat = (state->rlyError == 0) ? LED_PASS : LED_FAIL;
    disp_refresh(state);
    
    state->waitTime = 0x10;
    timer_delay(state);
    
    /* Wait for user input */
    uint8_t key;
    do {
        key = get_key(state);
        if (key == KEY_REPEAT) {
            test35_timed_relays_1way(state);
            return;
        }
    } while (key != KEY_NEXT);
    
    /* Check if latched relay test needed */
    if (state->fctCtlNum != 0) {
        test40_latched_relay_1way(state);
    } else {
        /* Return to main loop */
        state->keyMask = 0x48;
    }
}


/* Test 40 (1-way): Latched relay D */
static void test40_latched_relay_1way(FCT_State *state)
{
    disp_blank(state);
    state->testOne = 0x00;
    state->testTen = 0x04;
    disp_refresh(state);
    
    state->rlyTemp = hw_read_byte(RLY_STATUS);
    
    /* Close relay */
    msg_1way(state);
    state->msgTbl[4] = 0xC0;
    msg_cmd(state);
    
    state->waitTime = 0x10;
    timer_delay(state);
    
    state->rlyLatch = hw_read_byte(RLY_STATUS) & 0x04;
    
    /* Open relay */
    msg_1way(state);
    state->msgTbl[4] = 0xA0;
    msg_cmd(state);
    
    state->waitTime = 0x10;
    timer_delay(state);
    
    uint8_t rly_status = hw_read_byte(RLY_STATUS);
    if (((~rly_status) & 0x04) == state->rlyLatch) {
        state->ledRlyD = LED_PASS;
        state->ledStat = LED_PASS;
        disp_refresh(state);
        
        /* Handle post-test relay state based on fctCtlNum */
        if (state->fctCtlNum == 3) {
            /* Leave closed */
            msg_1way(state);
            state->msgTbl[4] = 0xC0;
            msg_cmd(state);
            state->waitTime = 0x10;
            timer_delay(state);
        } else if (state->fctCtlNum == 1) {
            /* Restore original state */
            if (state->rlyTemp & 0x04) {
                msg_1way(state);
                state->msgTbl[4] = 0xC0;
                msg_cmd(state);
                state->waitTime = 0x10;
                timer_delay(state);
            }
        }
        /* fctCtlNum == 2: leave open (already open) */
    } else {
        state->ledRlyD = LED_FAIL;
        state->ledStat = LED_FAIL;
        disp_refresh(state);
    }
    
    /* Return to main loop */
    state->keyMask = 0x48;
}


/* ========== 2-Way Test Suite ========== */

void test_2way_suite(FCT_State *state)
{
    if (state->testSet == 1) {
        test15_download_address(state);
    } else {
        test20_read_verify_address(state);
    }
}


/* Test 15: Download address to UUT */
static void test15_download_address(FCT_State *state)
{
    uint8_t temp_a, temp_b;
    
    disp_blank(state);
    state->testOne = 0x05;
    state->testTen = 0x01;
    disp_refresh(state);
    
    msg_load(state, msg01);
    
    /* Calculate address bytes for message */
    /* 8th byte = 1100 + upper nybble of addrH */
    temp_a = state->addrH;
    temp_b = state->addrM;
    
    /* Shift right 4 bits (signed arithmetic shift) */
    for (int i = 0; i < 4; i++) {
        uint8_t carry = temp_a & 0x01;
        temp_a = (int8_t)temp_a >> 1;
        temp_b = (temp_b >> 1) | (carry << 7);
    }
    
    state->msgTbl[8] = (temp_a & 0x0F) | 0xC0;
    state->msgTbl[9] = temp_b;
    
    /* Calculate remaining address bytes */
    temp_a = state->addrM;
    temp_b = state->addrL;
    
    /* Shift left 4 bits */
    for (int i = 0; i < 4; i++) {
        uint8_t carry = (temp_a & 0x80) ? 1 : 0;
        temp_a = (temp_a << 1) | ((temp_b & 0x80) ? 1 : 0);
        temp_b <<= 1;
    }
    
    state->msgTbl[10] = temp_a;
    state->msgTbl[11] = temp_b;
    
    /* Add inverted address bytes */
    uint8_t inv_h = ~state->addrH;
    state->msgTbl[11] |= (inv_h >> 4) & 0x0F;
    
    temp_a = state->addrH;
    temp_b = state->addrM;
    
    for (int i = 0; i < 4; i++) {
        uint8_t carry = temp_a & 0x01;
        temp_a = (int8_t)temp_a >> 1;
        temp_b = (temp_b >> 1) | (carry << 7);
    }
    
    state->msgTbl[12] = ~temp_b;
    
    uint8_t inv_m = ~state->addrM;
    state->msgTbl[13] = (inv_m << 4) & 0xF0;
    state->msgTbl[15] = ~state->addrL;
    
    /* Enable Init Enable line (active low) */
    uint8_t porta = hw_read_byte(PIA_PORT_A);
    hw_write_byte(PIA_PORT_A, porta & 0xEF);
    
    /* Send message */
    state->xmitStatus = 1;
    state->xmitFlag = 1;
    state->waitTime = 0x24;
    timer_delay(state);
    
    if (state->xmitStatus > 0) {
        error_handler(state, 1);
        return;
    }
    
    state->waitTime = 3;
    timer_delay(state);
    
    /* Deactivate Init Enable line */
    porta = hw_read_byte(PIA_PORT_A);
    hw_write_byte(PIA_PORT_A, porta | 0x10);
    
    state->ledStat = LED_PASS;
    disp_refresh(state);
    state->waitTime = 0x10;
    timer_delay(state);
    
    /* Continue to test 20 */
    test20_read_verify_address(state);
}


/* Test 20: Read / verify UUT address */
static void test20_read_verify_address(FCT_State *state)
{
    disp_blank(state);
    state->testOne = 0x02;
    state->testTen = 0x00;
    disp_refresh(state);
    
    msg_2way(state);
    state->msgTbl[6] = 0x22;    /* data pointer */
    state->msgTbl[7] = 0x40;    /* response flag */
    
    if (state->testSet == 0) {
        /* Use Emetcon test address */
        state->addrH = 0x15;
        state->addrM = 0x55;
        state->addrL = 0x55;
        
        /* Enable Init Enable line */
        uint8_t porta = hw_read_byte(PIA_PORT_A);
        hw_write_byte(PIA_PORT_A, porta & 0xEF);
    }
    
    addr_x4(state);
    state->msgTbl[3] = state->addrH4;
    state->msgTbl[4] = state->addrM4;
    state->msgTbl[5] = state->addrL4 | 0x02;
    
    /* Send command and response flag */
    state->xmitStatus = 1;
    state->xmitFlag = 1;
    state->waitTime = 5;
    timer_delay(state);
    
    if (state->msgBits >= 0x41) {
        error_handler(state, 1);
        return;
    }
    
    /* Wait for transmission complete */
    while (state->msgBits != 0);
    
    /* Set receive parameters */
    state->msgLen = 0x0D;
    state->xmitByte = 2;
    
    /* Deactivate Init Enable line */
    uint8_t porta = hw_read_byte(PIA_PORT_A);
    hw_write_byte(PIA_PORT_A, porta | 0x10);
    
    /* Begin data retrieval */
    state->waitTime = 7;
    timer_delay(state);
    
    if (state->msgBits == 0) {
        error_handler(state, 2);
        return;
    }
    
    /* Wait for first response packet */
    while (state->msgBits != 0x34);
    state->msgBits = 0;
    
    if (state->bchSum != 0) {
        error_handler(state, 4);
        return;
    }
    
    /* Wait for second packet */
    state->waitTime = 5;
    timer_delay(state);
    
    if (state->msgBits == 0) {
        error_handler(state, 3);
        return;
    }
    
    while (state->msgBits != 0x34);
    state->msgBits = 0;
    
    if (state->bchSum != 0) {
        error_handler(state, 4);
        return;
    }
    
    state->xmitFlag = 0;
    
    /* Validate received data */
    /* Extract address from received bytes */
    uint8_t recv_h = (state->msgTbl[2] << 4) | (state->msgTbl[3] >> 4);
    uint8_t recv_m = (state->msgTbl[3] << 4) | (state->msgTbl[4] >> 4);
    uint8_t recv_l = (state->msgTbl[4] << 4) | (state->msgTbl[5] >> 4);
    
    if (state->testSet != 0) {
        if (recv_h != state->addrH || recv_m != state->addrM || recv_l != state->addrL) {
            error_handler(state, 5);
            return;
        }
    } else {
        state->addrH = recv_h;
        state->addrM = recv_m;
        state->addrL = recv_l;
    }
    
    /* Verify inverted copy */
    if (~state->msgTbl[7] != state->addrH ||
        ~state->msgTbl[8] != state->addrM ||
        ~state->msgTbl[9] != state->addrL) {
        error_handler(state, 5);
        return;
    }
    
    state->ledStat = LED_PASS;
    disp_refresh(state);
    state->waitTime = 0x10;
    timer_delay(state);
    
    if (state->testSet == 0) {
        /* Display address and wait for input */
        addr_x4(state);
        state->f2bits = 0x18;
        state->f2bytes = 4;
        bin_to_bcd(state);
        disp_bcd(state);
        disp_refresh(state);
        
        uint8_t key;
        do {
            key = get_key(state);
            if (key == KEY_REPEAT) {
                test20_read_verify_address(state);
                return;
            }
        } while (key != KEY_NEXT);
    }
    
    /* Route to next test based on testSet */
    if (state->testSet == 3) {
        /* End of DCT test 30 */
        state->keyMask = 0x48;
        return;
    }
    
    test25_get_hardware_id(state);
}


/* Test 25: Get hardware ID */
static void test25_get_hardware_id(FCT_State *state)
{
    disp_blank(state);
    state->testOne = 0x05;
    state->testTen = 0x02;
    disp_refresh(state);
    
    msg_2way(state);
    msg_data_qry(state);
    
    /* Validate received message */
    if ((state->msgTbl[1] & 0x1F) != (state->addrM & 0x1F)) {
        error_handler(state, 5);
        return;
    }
    
    if (state->msgTbl[2] != state->addrL) {
        error_handler(state, 5);
        return;
    }
    
    /* Save firmware info */
    state->uutFWbyte = state->msgTbl[3];
    state->uutFWspec = state->msgTbl[4];
    state->optbyte = state->msgTbl[5];
    
    get_fw_rev(state);
    
    /* Determine UUT group based on firmware */
    switch (state->uutFWbyte) {
        case 0x01:  /* S00001 (suspected LMT-1) */
        case 0x24:  /* S00036 (LMT-2) */
            state->uutGroup = 2;
            break;
        case 0x4A:  /* S00074 (MCT-212, MCT-213, MCT-22x) */
        case 0x5D:  /* S00093 (MCT-240, MCT-242) */
            state->uutGroup = 4;
            break;
        case 0x58:  /* S00088 (MCT-210) */
        case 0x5F:  /* S00095 (MCT-210, MCT-213) */
            state->uutGroup = 6;
            break;
        case 0x08:  /* S00008 (suspected DCT) */
        case 0x21:  /* S00033 (DCT-501) */
            state->uutGroup = 8;
            break;
        default:
            /* Error 7: Firmware not in lookup table */
            state->readout[0] = 7;
            state->ledStat = LED_FAIL;
            get_fw_rev(state);
            
            uint8_t key;
            do {
                key = get_key(state);
                if (key == KEY_REPEAT) {
                    test25_get_hardware_id(state);
                    return;
                }
            } while (key != KEY_NEXT);
            
            state->keyMask = 0x48;
            return;
    }
    
    /* Continue to appropriate test based on uutGroup and testSet */
    /* Simplified - actual routing logic is complex */
    state->keyMask = 0x48;
}


/* Test 25b: Condensed version for quick read */
static void test25b_condensed(FCT_State *state)
{
    test25_get_hardware_id(state);
}


/* Test 50: Toggle test mode */
static void test50_toggle_test_mode(FCT_State *state)
{
    disp_blank(state);
    state->testOne = 0x00;
    state->testTen = 0x05;
    disp_refresh(state);
    
    if (state->lmtTestMd == 0) {
        msg_2way(state);
        state->msgTbl[6] = 0x54;    /* command byte */
        msg_cmd(state);
    }
    
    state->waitTime = 4;
    timer_delay(state);
    
    state->ledStat = LED_PASS;
    disp_refresh(state);
    state->waitTime = 0x10;
    timer_delay(state);
}


/* Test 35 (2-way): Timed relays */
static void test35_timed_relays_2way(FCT_State *state)
{
    /* Similar to 1-way but uses msg_2way */
    /* Simplified implementation */
    state->keyMask = 0x48;
}


/* Test 40 (2-way): Latched relay */
static void test40_latched_relay_2way(FCT_State *state)
{
    /* Similar to 1-way but uses msg_2way */
    /* Simplified implementation */
    state->keyMask = 0x48;
}


/* Test 61: DCT Analog tests */
static void test61_dct_analog(FCT_State *state)
{
    disp_blank(state);
    state->testOne = 0x01;
    state->testTen = 0x06;
    disp_refresh(state);
    
    msg_2way(state);
    state->msgTbl[6] = 0x3A;    /* data pointer */
    msg_data_qry(state);
    
    disp_dct(state);
    
    state->waitTime = 0x10;
    timer_delay(state);
    
    uint8_t key;
    do {
        key = get_key(state);
        if (key == KEY_REPEAT) {
            test61_dct_analog(state);
            return;
        }
    } while (key != KEY_NEXT);
    
    state->keyMask = 0x48;
}
