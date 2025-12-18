/*
 * S00060-H Field Configuration Terminal
 * Westinghouse/ABB unit 1993D40G06 (12.5kHz)
 * 
 * Converted from 6800 assembly to C
 * Original firmware for testing LMT/MCT/DCT utility meters
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Hardware addresses (adjust for target platform)
#define MC6840_BASE     0x0000
#define MC6821_BASE     0x000C
#define RAM_LO_BASE     0x0080
#define RAM_HI_BASE     0x0C00

// MC6840 Programmable Timer
#define PTM_CTL0        (*(volatile uint8_t*)(MC6840_BASE + 0))
#define PTM_CTL1        (*(volatile uint8_t*)(MC6840_BASE + 1))
#define PTM_TM1_H       (*(volatile uint8_t*)(MC6840_BASE + 2))
#define PTM_TM1_L       (*(volatile uint8_t*)(MC6840_BASE + 3))
#define PTM_TM2_H       (*(volatile uint8_t*)(MC6840_BASE + 4))
#define PTM_TM2_L       (*(volatile uint8_t*)(MC6840_BASE + 5))
#define PTM_TM3_H       (*(volatile uint8_t*)(MC6840_BASE + 6))
#define PTM_TM3_L       (*(volatile uint8_t*)(MC6840_BASE + 7))

// MC6821 PIA
#define PIA_PORTA       (*(volatile uint8_t*)(MC6821_BASE + 0))
#define PIA_CTLA        (*(volatile uint8_t*)(MC6821_BASE + 1))
#define PIA_PORTB       (*(volatile uint8_t*)(MC6821_BASE + 2))
#define PIA_CTLB        (*(volatile uint8_t*)(MC6821_BASE + 3))

// Function selector (74LS138)
#define TESTSW          (*(volatile uint8_t*)(RAM_HI_BASE + 249))
#define KB_IRQ          (*(volatile uint8_t*)(RAM_HI_BASE + 250))
#define RLY_STATUS      (*(volatile uint8_t*)(RAM_HI_BASE + 252))
#define CFG_JPR         (*(volatile uint8_t*)(RAM_HI_BASE + 253))
#define KD_DATA         (*(volatile uint8_t*)(RAM_HI_BASE + 254))
#define KD_CMD          (*(volatile uint8_t*)(RAM_HI_BASE + 255))

// Low RAM structure
typedef struct {
    uint8_t bchSum;           // BCH checksum
    uint8_t reserved1;
    uint8_t byteNow;          // Current incoming byte
    uint8_t byteLast;         // Most recent completed byte
    uint8_t byteOld;          // Previous completed byte
    int8_t valueIi;           // Raw vector - fundamental, 0°
    int8_t valueQi;           // Raw vector - fundamental, 90°
    int8_t valueI2i;          // Raw vector - 2nd harmonic, 0°
    int8_t valueQ2i;          // Raw vector - 2nd harmonic, 90°
    int8_t avgI2H;            // Running average - high byte vector I
    int8_t avgQ2H;            // Running average - high byte vector Q
    uint8_t avgI2L;           // Running average - low byte vector I
    uint8_t avgQ2L;           // Running average - low byte vector Q
    int8_t avgQ20;            // Previous Q2H value
    int8_t vectorI;           // Calculated vector - 0°
    int8_t vectorQ;           // Calculated vector - 90°
    uint8_t sgnFlag;          // Polarity transition flag
    uint8_t sigLevel;         // Signal level (>9 = valid)
    int8_t vectorSum;         // Phase detector output
    int8_t temp1;             // Scratchpad - interrupt
    int8_t temp2;             // Scratchpad - interrupt
    uint8_t reserved2[3];
    uint8_t mTable1[4];       // Convert vectorSum to bits
    uint8_t mTable2[4];       // Convert vectorSum to bits
    uint8_t msgTbl[22];       // Message buffer
    uint8_t msgBits;          // Number of bits in message
    int8_t xmitStatus;        // Transmitter status
    uint8_t xmitByte;         // Carrier segment counter
    uint8_t keyMask;          // Function key enable mask
    uint8_t reserved3;
    uint8_t uutType;          // 1=LMT-1xx, 2=LMT-2/MCT, 4=DCT
    int8_t dataNum;           // Last number pressed
    uint8_t dataKey;          // Last key pressed
    uint8_t waitTime;         // Delay counter (1/16 sec per count)
    uint8_t temp6;            // Scratchpad - application
    uint8_t temp7;
    uint8_t temp8;
    uint8_t temp9;
    uint8_t rlyTemp;          // Pre-test relay state
    uint8_t rlyLatch;         // Latched relay state
    uint8_t addrL;            // Address (24-bit, big-endian)
    uint8_t addrM;
    uint8_t addrH;
    uint8_t reserved4;
    uint8_t xmitFlag;         // Activates transmitter
    int8_t headerBit;         // Preamble bit counter
    int8_t mSum_peak;         // Highest working value
    uint8_t reserved5;
    uint8_t mValue_raw;       // Value for M tables
    uint8_t msgStart;         // End of preamble flag
    uint8_t installFlag;      // Install LED state
    uint8_t bitCount;         // Bit interval flag
    uint8_t msgEnd;           // End of message flag
    uint8_t countPulse;       // Bit stream counter
    uint8_t countPhase;       // Carrier pulse counter
    int8_t countBit;          // Bit interval counter
    uint8_t countSum;         // M table counter
    uint16_t M_index;         // M table pointer
    uint8_t reserved6[2];
    int8_t mSum_last;         // Previous M table value
    int8_t mSum_Old;          // 2nd previous M table value
    int8_t mSum_Cur;          // Current M table value
    uint8_t reserved7[2];
    uint8_t dctTemp;          // DCT analog data temp
    uint8_t reserved8;
    uint8_t optbyte;          // Test validation byte
    uint8_t testSet;          // Test suite selector
} LowRAM;

// High RAM structure
typedef struct {
    uint16_t msgLen;          // Message length
    uint8_t reserved1;
    uint8_t fctCtlNum;        // FCT control number
    uint8_t reserved2[2];
    uint8_t rlyNum;           // Number of relays to test
    uint8_t uutFWbyte;        // UUT firmware number
    uint8_t uutGroup;         // UUT group flag
    uint8_t uutFWspec;        // UUT firmware revision
    uint8_t reserved3;
    uint8_t bcdData10;        // BCD data buffer (packed)
    uint8_t bcdData32;
    uint8_t bcdData54;
    uint8_t bcdData76;
    uint8_t addrL4;           // Address shifted left 2 bits
    uint8_t addrM4;
    uint8_t addrH4;
    uint8_t lmtTestMd;        // UUT test mode flag
    uint16_t f2binary;        // Binary conversion source
    uint16_t f2BCD;           // BCD conversion destination
    uint8_t f2bitpos;         // Bit position marker
    uint8_t f2bytes;          // Bytes to convert
    uint8_t f2bits;           // Bits to convert
    uint8_t dctData1;         // DCT data copies
    uint8_t dctData2;
    uint8_t dctData3;
    uint8_t dctData4;
    uint8_t dctTouStat;       // DCT TOU status
    uint8_t reserved4;
    uint8_t rlyError;         // Relay failure count
    uint8_t rlyBits;          // Relay closure bits
    uint8_t reserved5[71];
    uint8_t readout[8];       // Main display (8 digits)
    uint8_t testOne;          // Test number ones digit
    uint8_t testTen;          // Test number tens digit
    uint8_t ledRlyA;          // Relay A LED
    uint8_t ledRlyB;          // Relay B LED
    uint8_t ledRlyC;          // Relay C LED
    uint8_t ledRlyD;          // Relay D LED
    uint8_t ledInstall;       // Install mode LED
    uint8_t ledStat;          // Pass/fail LED
} HighRAM;

// Global RAM instances
static LowRAM* lowRam = (LowRAM*)RAM_LO_BASE;
static HighRAM* highRam = (HighRAM*)RAM_HI_BASE;

// Frequency lookup tables
static const int8_t freq1[10] = {32, 23, 0, -23, -32, -23, 0, 23, 32, 23};
static const int8_t freq2[9] = {1, 0, -1, 0, 1, 0, -1, 0, 1};

// Forward declarations
void reset(void);
void interrupt_handler(void);
void timer(void);
void dispRefresh(void);
void dispBlank(void);
void getKey(void);
void addrBin(void);
void bin2BCD(void);
void dispBCD(void);
void addrX4(void);
void msg1way(void);
void msg2way(void);
void msgCmd(void);
void msgDataQry(void);

// ===== Hardware Initialization =====

void init_pia(void) {
    PIA_CTLA = 0x30;
    PIA_CTLB = 0x30;
    PIA_PORTA = 0xFE;  // All outputs except PA0
    PIA_PORTB = 0xFF;  // All outputs (inactive)
    PIA_CTLA = 0x34;
    PIA_CTLB = 0x34;
    PIA_PORTA = 0x10;  // PA4 high (Init Enable inactive)
    PIA_PORTB = 0x00;
}

void init_ptm(void) {
    uint8_t freq_sel = PIA_PORTB & 0x01;
    
    if (freq_sel == 0) {
        // 12.5 kHz
        PTM_TM3_H = 0x00;
        PTM_TM3_L = 0x27;
        PTM_TM2_H = 0x01;
        PTM_TM2_L = 0x99;
    } else {
        // 9.615 kHz
        PTM_TM3_H = 0x00;
        PTM_TM3_L = 0x33;
        PTM_TM2_H = 0x01;
        PTM_TM2_L = 0xAC;
    }
    
    PTM_TM1_H = 0x21;
    PTM_TM1_L = 0x33;
    PTM_CTL1 = 0x12;
    PTM_CTL0 = 0x12;
    PTM_CTL1 = 0x13;
    PTM_CTL0 = 0x92;
    PTM_CTL1 = 0xD3;
}

void init_variables(void) {
    lowRam->countBit = 4;
    lowRam->countPulse = 0;
    lowRam->countPhase = 0;
    lowRam->M_index = (uint16_t)&lowRam->mTable1[3];
    highRam->msgLen = 0;
    lowRam->bchSum = 0;
    lowRam->msgBits = 0;
    lowRam->xmitByte = 1;
    lowRam->msgEnd = 1;
    lowRam->keyMask = 0;
    lowRam->uutType = 0;
    lowRam->dataNum = 0;
    lowRam->xmitStatus = 0;
    lowRam->bitCount = 0;
    highRam->uutGroup = 0;
    lowRam->xmitFlag = 0;
    highRam->addrL4 = 0;
    highRam->addrM4 = 0;
    highRam->addrH4 = 0;
}

void reset(void) {
    init_pia();
    init_ptm();
    init_variables();
    
    dispBlank();
    highRam->ledInstall = 0x0F;
    dispRefresh();
    
    // Continue to main loop
    main_loop();
}

// ===== Display Functions =====

void dispRefresh(void) {
    // Reset 8279
    KD_CMD = 0xDC;
    for (volatile uint8_t i = 100; i > 0; i--);
    
    KD_CMD = 0xAA;  // Set masking
    KD_CMD = 0x18;  // Set mode
    KD_CMD = 0x2A;  // Set clock divisor
    
    // Write display data
    KD_CMD = 0x90;
    for (uint8_t i = 0; i < 16; i++) {
        KD_DATA = ((uint8_t*)&highRam->readout)[i];
    }
    KD_CMD = 0x80;
}

void dispBlank(void) {
    for (uint8_t i = 0; i < 8; i++) {
        highRam->readout[i] = 0x0F;
    }
    highRam->testOne = 0x0F;
    highRam->testTen = 0x0F;
    highRam->ledRlyA = 0x0F;
    highRam->ledRlyB = 0x0F;
    highRam->ledRlyC = 0x0F;
    highRam->ledRlyD = 0x0F;
    highRam->ledStat = 0x0F;
}

// ===== Keypad Functions =====

void getKey(void) {
    // Wait for keypress
    while ((KB_IRQ & 0x01) == 0);
    
    KD_CMD = 0x50;  // Get key data
    uint8_t key = KD_DATA & 0x1F;
    lowRam->dataKey = key;
    
    // Convert to number
    lowRam->dataNum = -1;  // Default: not a number
    
    const uint8_t keymap[10] = {0x18, 0x10, 0x11, 0x12, 0x08, 
                                 0x09, 0x0A, 0x00, 0x01, 0x02};
    
    for (uint8_t i = 0; i < 10; i++) {
        if (key == keymap[i]) {
            lowRam->dataNum = i;
            break;
        }
    }
    
    // Check for reset key
    if (key == 0x19) {
        reset();
    }
}

// ===== Timer Function =====

void timer(void) {
    PTM_CTL0 = 0x22;
    
    while (lowRam->waitTime > 0) {
        PTM_TM1_H = 0xFF;
        PTM_TM1_L = 0xFF;
        
        while ((PTM_CTL1 & 0x01) == 0);
        
        lowRam->waitTime--;
    }
}

// ===== BCD Conversion Functions =====

void addrBin(void) {
    lowRam->addrH = 0;
    lowRam->addrM = 0;
    lowRam->addrL = 0;
    
    for (int8_t i = 7; i >= 0; i--) {
        // Add digit
        lowRam->addrL += highRam->readout[i];
        
        if (i == 0) break;
        
        // Multiply by 10 (shift left and add 2x)
        uint32_t temp = ((uint32_t)lowRam->addrH << 16) | 
                        ((uint32_t)lowRam->addrM << 8) | 
                        lowRam->addrL;
        temp = (temp << 1) + (temp << 3);  // *2 + *8 = *10
        
        lowRam->addrH = (temp >> 16) & 0xFF;
        lowRam->addrM = (temp >> 8) & 0xFF;
        lowRam->addrL = temp & 0xFF;
    }
}

void bin2BCD(void) {
    // Clear BCD buffer
    uint8_t* bcd = &highRam->bcdData10;
    for (uint8_t i = 0; i < highRam->f2bytes; i++) {
        bcd[i] = 0;
    }
    
    // Binary to BCD conversion using double-dabble algorithm
    uint8_t* binary = (uint8_t*)highRam->f2binary;
    
    for (uint8_t bit = 0; bit < highRam->f2bits; bit++) {
        // Add 3 to BCD digits >= 5
        for (uint8_t i = 0; i < highRam->f2bytes; i++) {
            if ((bcd[i] & 0x0F) >= 5) bcd[i] += 0x03;
            if ((bcd[i] & 0xF0) >= 0x50) bcd[i] += 0x30;
        }
        
        // Shift left
        uint8_t carry = 0;
        for (int8_t i = highRam->f2bytes - 1; i >= 0; i--) {
            uint8_t new_carry = (bcd[i] & 0x80) ? 1 : 0;
            bcd[i] = (bcd[i] << 1) | carry;
            carry = new_carry;
        }
        
        // Shift in next binary bit
        uint8_t bit_val = (binary[bit / 8] >> (7 - (bit % 8))) & 1;
        bcd[highRam->f2bytes - 1] |= bit_val;
    }
}

void dispBCD(void) {
    highRam->readout[0] = highRam->bcdData10 & 0x0F;
    highRam->readout[1] = (highRam->bcdData10 >> 4) & 0x0F;
    highRam->readout[2] = highRam->bcdData32 & 0x0F;
    highRam->readout[3] = (highRam->bcdData32 >> 4) & 0x0F;
    highRam->readout[4] = highRam->bcdData54 & 0x0F;
    highRam->readout[5] = (highRam->bcdData54 >> 4) & 0x0F;
    highRam->readout[6] = highRam->bcdData76 & 0x0F;
    highRam->readout[7] = (highRam->bcdData76 >> 4) & 0x0F;
    highRam->ledStat = 0x07;
}

void addrX4(void) {
    uint16_t temp;
    
    temp = ((uint16_t)lowRam->addrH << 8) | lowRam->addrM;
    temp <<= 2;
    highRam->addrH4 = (temp >> 8) & 0xFF;
    
    temp = ((uint16_t)lowRam->addrM << 8) | lowRam->addrL;
    temp <<= 2;
    highRam->addrM4 = (temp >> 8) & 0xFF;
    highRam->addrL4 = temp & 0xFF;
}

// ===== Message Functions =====

void msg1way(void) {
    lowRam->msgTbl[0] = 0xAA;
    lowRam->msgTbl[1] = 0xB8;
    lowRam->msgTbl[2] = 0xFF;
    lowRam->msgTbl[3] = highRam->addrL4;
    lowRam->msgTbl[4] = 0x00;  // Command byte
    lowRam->msgTbl[5] = 0x00;  // Response flag (always 0)
    lowRam->xmitByte = 1;
    lowRam->msgBits = 0x2B;
    highRam->msgLen = 6;
}

void msg2way(void) {
    lowRam->msgTbl[0] = 0xAA;
    lowRam->msgTbl[1] = 0xBA;
    lowRam->msgTbl[2] = 0xFF;
    lowRam->msgTbl[3] = highRam->addrH4;
    lowRam->msgTbl[4] = highRam->addrM4;
    lowRam->msgTbl[5] = highRam->addrL4;
    lowRam->msgTbl[6] = 0x00;  // Command byte
    lowRam->msgTbl[7] = 0x00;  // Response flag
    lowRam->xmitByte = 1;
    lowRam->msgBits = 0x41;
    highRam->msgLen = 8;
}

void msgCmd(void) {
    lowRam->xmitStatus = 1;
    lowRam->xmitFlag = 1;
    lowRam->waitTime = 0x10;
    timer();
    
    if (lowRam->xmitStatus > 0) {
        // Error - no transmit
        error_handler(1);
    }
    
    lowRam->xmitFlag = 0;
}

// ===== Main Loop =====

void main_loop(void) {
    // Test 01: Validate switch and jumper settings
    highRam->testOne = 1;
    highRam->testTen = 0;
    dispRefresh();
    lowRam->waitTime = 4;
    timer();
    
    while (1) {
        uint8_t cfg = CFG_JPR & 0x03;
        uint8_t sw = TESTSW & 0x03;
        
        if (cfg == sw) {
            highRam->ledStat = 0x0F;
            dispRefresh();
            
            // Set UUT type
            if (sw == 0x03) sw = 0x04;
            lowRam->uutType = sw;
            break;
        }
        
        highRam->ledStat = 0x04;
        dispRefresh();
        lowRam->waitTime = 4;
        timer();
    }
    
    // Continue with remaining tests...
    // (Additional test implementation would continue here)
}

// ===== Interrupt Handler =====

void interrupt_handler(void) {
    // Implementation of CPSK demodulation algorithm
    // (Complex signal processing code would go here)
    
    // This is a simplified placeholder
    if (lowRam->xmitFlag == 0) {
        return;
    }
    
    // Read incoming bit
    uint8_t bit = (PIA_PORTA & 0x01);
    lowRam->byteNow = (lowRam->byteNow << 1) | bit;
    
    lowRam->countPulse--;
    if ((lowRam->countPulse & 0x07) == 0) {
        // 8 bits received
        lowRam->byteLast = lowRam->byteNow;
        
        // Process based on mode...
    }
}

// ===== Error Handler =====

void error_handler(uint8_t error_code) {
    highRam->readout[0] = error_code;
    highRam->ledStat = 0x04;
    dispRefresh();
    lowRam->xmitFlag = 0;
    lowRam->keyMask = 0x48;
    main_loop();
}

// ===== Entry Point =====

int main(void) {
    reset();
    return 0;
}