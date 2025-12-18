/**
 * S00060-H2 - Westinghouse Field Configuration Terminal Firmware
 * 
 * Converted from 68HC11/6800 assembly to C
 * Original: THRsim11 compiler for SC44125P MPU
 * 
 * Unit: 1993D40G06 (12.5kHz variant)
 * EPROMs: 2732 (U11: S00060G-U11, U17: S00060G-U17)
 * 
 * The FCT is a portable unit for servicing LMTs / MCTs / DCTs
 * Contains an LMT-2 board set configured for two 2732 EPROMs
 * 
 * NOTE: This is a structural conversion. Hardware-specific timing and
 * interrupt handling will need adaptation for your target platform.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ========== Hardware Address Definitions ========== */

/* Base addresses (allows relocation for simulation) */
#define MC6840_BASE     0x0000  /* Programmable Timer Module */
#define MC6821_BASE     0x000C  /* Peripheral Interface Adapter */
#define RAM_LO_BASE     0x0080  /* MC6810 SRAM (128 bytes) */
#define RAM_HI_BASE     0x0C00  /* HM6561 x 2 NVRAM */

/* MC6840 Programmable Timer registers */
#define PTM_CTL0        (*(volatile uint8_t*)(MC6840_BASE + 0))  /* Control: Timer 1 & 3 */
#define PTM_CTL1        (*(volatile uint8_t*)(MC6840_BASE + 1))  /* Control: Timer 2 */
#define PTM_TM1_H       (*(volatile uint8_t*)(MC6840_BASE + 2))  /* Timer 1 high */
#define PTM_TM1_L       (*(volatile uint8_t*)(MC6840_BASE + 3))  /* Timer 1 low */
#define PTM_TM2_H       (*(volatile uint8_t*)(MC6840_BASE + 4))  /* Timer 2 high */
#define PTM_TM2_L       (*(volatile uint8_t*)(MC6840_BASE + 5))  /* Timer 2 low */
#define PTM_TM3_H       (*(volatile uint8_t*)(MC6840_BASE + 6))  /* Timer 3 high */
#define PTM_TM3_L       (*(volatile uint8_t*)(MC6840_BASE + 7))  /* Timer 3 low */

/* MC6821 Peripheral Interface Adapter registers */
#define PIA_PORT_A      (*(volatile uint8_t*)(MC6821_BASE + 0))
#define PIA_CTL_A       (*(volatile uint8_t*)(MC6821_BASE + 1))
#define PIA_PORT_B      (*(volatile uint8_t*)(MC6821_BASE + 2))
#define PIA_CTL_B       (*(volatile uint8_t*)(MC6821_BASE + 3))

/* Function selector (74LS138 on FCT Logic Board) */
#define TEST_SW         (*(volatile uint8_t*)(RAM_HI_BASE + 249))  /* Tester mode switch */
#define KB_EIRQ         (*(volatile uint8_t*)(RAM_HI_BASE + 250))  /* 8279 keyboard IRQ */
#define RLY_STATUS      (*(volatile uint8_t*)(RAM_HI_BASE + 252))  /* External contact closures */
#define CFG_JPR         (*(volatile uint8_t*)(RAM_HI_BASE + 253))  /* Mode jumpers */
#define KD_DATA         (*(volatile uint8_t*)(RAM_HI_BASE + 254))  /* 8279 data port */
#define KD_CMD          (*(volatile uint8_t*)(RAM_HI_BASE + 255))  /* 8279 command port */

/* ========== RAM Variables (Low RAM: 0x0080-0x00FF) ========== */

typedef struct {
    uint8_t bchSum;           /* BCH checksum - 0 if data good */
    uint8_t reserved1;
    uint8_t byteNow;          /* Current incoming byte (bit stream) */
    uint8_t byteLast;         /* Most recently completed byte */
    uint8_t byteOld;          /* Previously completed byte */
    uint8_t valueIi;          /* Raw vector data - fundamental, 0 degree */
    uint8_t valueQi;          /* Raw vector data - fundamental, 90 degree */
    uint8_t valueI2i;         /* Raw vector data - 2nd harmonic, 0 degree */
    uint8_t valueQ2i;         /* Raw vector data - 2nd harmonic, 90 degree */
    uint8_t avgI2H;           /* Running average - high byte of vector I */
    uint8_t avgQ2H;           /* Running average - high byte of vector Q */
    uint8_t avgI2L;           /* Running average - low byte of vector I */
    uint8_t avgQ2L;           /* Running average - low byte of vector Q */
    uint8_t avgQ20;           /* Previous value of Q2H */
    int8_t  vectorI;          /* Calculated vector - 0 degrees */
    int8_t  vectorQ;          /* Calculated vector - 90 degrees */
    int8_t  sgnFlag;          /* Flag for polarity transitions on vector Q */
    uint8_t sigLevel;         /* Received signal level (9 = cutoff) */
    int8_t  vectorSum;        /* Output byte of phase detector */
    uint8_t temp1;            /* Scratchpad byte - interrupt */
    uint8_t temp2;            /* Scratchpad byte - interrupt */
    uint8_t reserved2[3];
    uint8_t mTable1[4];       /* Table - convert vectorSum to bits */
    uint8_t mTable2[4];       /* Table - convert vectorSum to bits */
    uint8_t msgTbl[22];       /* Incoming/outgoing message buffer (max 6 bytes receive) */
    uint8_t msgBits;          /* Number of bits in message */
    int8_t  xmitStatus;       /* Transmitter status flag */
    uint8_t xmitByte;         /* Carrier segment counter */
    uint8_t keyMask;          /* Function key enable / display shifting */
    uint8_t reserved3;
    uint8_t uutType;          /* UUT type: 1=LMT-1xx, 2=LMT-2/MCT, 4=DCT */
    int8_t  dataNum;          /* Last number pressed (-1 = none) */
    uint8_t dataKey;          /* Last key pressed (matrix 0x00-0x1B) */
    uint8_t waitTime;         /* Delay counter (1/16 sec per count) */
    uint8_t temp6;            /* Scratchpad - application */
    uint8_t temp7;            /* Scratchpad - application */
    uint8_t temp8;            /* Scratchpad - application */
    uint8_t temp9;            /* Scratchpad - application */
    uint8_t rlyTemp;          /* Pre-test state of relays */
    uint8_t rlyLatch;         /* Latched relay state */
    uint8_t addrL;            /* Address (24 bits - big endian) */
    uint8_t addrM;
    uint8_t addrH;
    uint8_t reserved4;
    uint8_t xmitFlag;         /* Activates transmitter */
    int8_t  headerBit;        /* Count off bits in preamble */
    int8_t  mSum_peak;        /* Highest working value - mTable1/2 */
    uint8_t reserved5;
    int8_t  mValue_raw;       /* Value to carry into M tables */
    uint8_t msgStart;         /* Flag: end of message preamble */
    uint8_t installFlag;      /* Flag for install LED handling */
    uint8_t bitCount;         /* Flag: end of each received bit interval */
    uint8_t msgEnd;           /* Flag: end of received message */
    uint8_t countPulse;       /* Running counter - bit stream (8) */
    uint8_t countPhase;       /* Running counter - carrier pulse interval (4) */
    int8_t  countBit;         /* Bit interval counter (-1, 0-5) */
    uint8_t countSum;         /* Running counter for mTable calculations */
    uint16_t M_index;         /* Pointer for mTable1/mTable2 (0-3) */
    uint8_t reserved6[3];
    int8_t  mSum_last;        /* Previous working value - mTable1/2 */
    int8_t  mSum_Old;         /* 2nd previous working value */
    int8_t  mSum_Cur;         /* Current working value */
    uint8_t reserved7[2];
    uint8_t dctTemp;          /* DCT Analog data placeholder */
    uint8_t reserved8;
    uint8_t optbyte;          /* Test validation byte from LMT-2/DCT */
    uint8_t testSet;          /* Selects test suite for 2-way units */
} LowRAM_t;

/* ========== RAM Variables (High RAM: 0x0C00-0x0C7F) ========== */

typedef struct {
    uint16_t msgLen;          /* Message length (upper byte = 0) */
    uint8_t reserved1;
    uint8_t fctCtlNum;        /* FCT Control Number */
    uint8_t reserved2[2];
    uint8_t rlyNum;           /* Number of relays to test */
    uint8_t uutFWbyte;        /* Firmware number in UUT */
    uint8_t uutGroup;         /* Flag to direct tests */
    uint8_t uutFWspec;        /* Firmware revision in UUT */
    uint8_t reserved3;
    uint8_t bcdData10;        /* BCD data buffer (8 digits, packed) */
    uint8_t bcdData32;
    uint8_t bcdData54;
    uint8_t bcdData76;
    uint8_t addrL4;           /* Address, shifted left 2 bits */
    uint8_t addrM4;
    uint8_t addrH4;
    uint8_t lmtTestMd;        /* UUT test mode flag */
    uint16_t f2binary;        /* Location of source binary number */
    uint16_t f2BCD;           /* Location of packed BCD after conversion */
    uint8_t f2bitpos;         /* Bit marker for conversion */
    uint8_t f2bytes;          /* Number of bytes (+1) to convert */
    uint8_t f2bits;           /* Number of bits to convert */
    uint8_t dctData1;         /* Local copy of DCT upper RAM $0C3A */
    uint8_t dctData2;         /* Local copy of DCT upper RAM $0C3B */
    uint8_t dctData3;         /* Local copy of DCT upper RAM $0C3C */
    uint8_t dctData4;         /* Local copy of DCT upper RAM $0C3D */
    uint8_t dctTouStat;
    uint8_t reserved4;
    uint8_t rlyError;         /* Count relay failure */
    uint8_t rlyBits;          /* Evaluate relay closures */
    uint8_t reserved5[71];
    /* Display RAM (readout buffer) */
    uint8_t readout[8];       /* Main readout (8 digits) */
    uint8_t testOne;          /* Test number ones digit */
    uint8_t testTen;          /* Test number tens digit */
    uint8_t ledRlyA;          /* Relay A status LED */
    uint8_t ledRlyB;          /* Relay B status LED */
    uint8_t ledRlyC;          /* Relay C status LED */
    uint8_t ledRlyD;          /* Relay D status LED */
    uint8_t ledInstall;       /* Install mode LED */
    uint8_t ledStat;          /* Test pass/fail LED */
} HighRAM_t;

/* LED values: 0x0F=off, 0x04=fail, 0x07=pass, 0x08=all on */
#define LED_OFF     0x0F
#define LED_FAIL    0x04
#define LED_PASS    0x07
#define LED_ALL_ON  0x08

/* Global RAM structures */
static LowRAM_t  lowRAM;
static HighRAM_t highRAM;

/* Convenience macros for accessing nested structures */
#define bchSum      lowRAM.bchSum
#define byteNow     lowRAM.byteNow
#define byteLast    lowRAM.byteLast
#define byteOld     lowRAM.byteOld
#define valueIi     lowRAM.valueIi
#define valueQi     lowRAM.valueQi
#define valueI2i    lowRAM.valueI2i
#define valueQ2i    lowRAM.valueQ2i
#define avgI2H      lowRAM.avgI2H
#define avgQ2H      lowRAM.avgQ2H
#define avgI2L      lowRAM.avgI2L
#define avgQ2L      lowRAM.avgQ2L
#define avgQ20      lowRAM.avgQ20
#define vectorI     lowRAM.vectorI
#define vectorQ     lowRAM.vectorQ
#define sgnFlag     lowRAM.sgnFlag
#define sigLevel    lowRAM.sigLevel
#define vectorSum   lowRAM.vectorSum
#define temp1       lowRAM.temp1
#define temp2       lowRAM.temp2
#define mTable1     lowRAM.mTable1
#define mTable2     lowRAM.mTable2
#define msgTbl      lowRAM.msgTbl
#define msgBits     lowRAM.msgBits
#define xmitStatus  lowRAM.xmitStatus
#define xmitByte    lowRAM.xmitByte
#define keyMask     lowRAM.keyMask
#define uutType     lowRAM.uutType
#define dataNum     lowRAM.dataNum
#define dataKey     lowRAM.dataKey
#define waitTime    lowRAM.waitTime
#define temp6       lowRAM.temp6
#define temp7       lowRAM.temp7
#define temp8       lowRAM.temp8
#define temp9       lowRAM.temp9
#define rlyTemp     lowRAM.rlyTemp
#define rlyLatch    lowRAM.rlyLatch
#define addrL       lowRAM.addrL
#define addrM       lowRAM.addrM
#define addrH       lowRAM.addrH
#define xmitFlag    lowRAM.xmitFlag
#define headerBit   lowRAM.headerBit
#define mSum_peak   lowRAM.mSum_peak
#define mValue_raw  lowRAM.mValue_raw
#define msgStart    lowRAM.msgStart
#define installFlag lowRAM.installFlag
#define bitCount    lowRAM.bitCount
#define msgEnd      lowRAM.msgEnd
#define countPulse  lowRAM.countPulse
#define countPhase  lowRAM.countPhase
#define countBit    lowRAM.countBit
#define countSum    lowRAM.countSum
#define M_index     lowRAM.M_index
#define mSum_last   lowRAM.mSum_last
#define mSum_Old    lowRAM.mSum_Old
#define mSum_Cur    lowRAM.mSum_Cur
#define dctTemp     lowRAM.dctTemp
#define optbyte     lowRAM.optbyte
#define testSet     lowRAM.testSet

#define msgLen      highRAM.msgLen
#define fctCtlNum   highRAM.fctCtlNum
#define rlyNum      highRAM.rlyNum
#define uutFWbyte   highRAM.uutFWbyte
#define uutGroup    highRAM.uutGroup
#define uutFWspec   highRAM.uutFWspec
#define bcdData10   highRAM.bcdData10
#define bcdData32   highRAM.bcdData32
#define bcdData54   highRAM.bcdData54
#define bcdData76   highRAM.bcdData76
#define addrL4      highRAM.addrL4
#define addrM4      highRAM.addrM4
#define addrH4      highRAM.addrH4
#define lmtTestMd   highRAM.lmtTestMd
#define f2binary    highRAM.f2binary
#define f2BCD       highRAM.f2BCD
#define f2bitpos    highRAM.f2bitpos
#define f2bytes     highRAM.f2bytes
#define f2bits      highRAM.f2bits
#define dctData1    highRAM.dctData1
#define dctData2    highRAM.dctData2
#define dctData3    highRAM.dctData3
#define dctData4    highRAM.dctData4
#define dctTouStat  highRAM.dctTouStat
#define rlyError    highRAM.rlyError
#define rlyBits     highRAM.rlyBits
#define readout     highRAM.readout
#define testOne     highRAM.testOne
#define testTen     highRAM.testTen
#define ledRlyA     highRAM.ledRlyA
#define ledRlyB     highRAM.ledRlyB
#define ledRlyC     highRAM.ledRlyC
#define ledRlyD     highRAM.ledRlyD
#define ledInstall  highRAM.ledInstall
#define ledStat     highRAM.ledStat

/* ========== Constant Tables (ROM) ========== */

/* Vector sum calculation tables */
static const int8_t freq1[10] = {32, 23, 0, -23, -32, -23, 0, 23, 32, 23};
static const int8_t freq2[9] = {1, 0, -1, 0, 1, 0, -1, 0, 1};

/* Test message tables (loaded via msgLoad) */
/* First two bytes (0xAA, 0xBA) are loaded within subroutine */

/* Test 15: pointer byte $22, Emetcon test address + flags */
static const uint8_t msg01[24] = {
    0xFF, 0x55, 0x55, 0x56, 0x22, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0C, 0x00, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x41, 0x03, 0x00, 0x15
};

/* Test 30a (uutGroup: 2 or 4): pointer byte $3F */
static const uint8_t msg02[24] = {
    0xFF, 0x00, 0x00, 0x02, 0x3F, 0x00, 0xCF, 0xF0, 0x10, 0x00, 0x00, 0x00,
    0x0C, 0x00, 0x00, 0x00, 0x00, 0x02, 0x40, 0x00, 0x41, 0x03, 0x00, 0x15
};

/* Test 30b (uutGroup: 6): pointer byte $35 */
static const uint8_t msg03[24] = {
    0xFF, 0x00, 0x00, 0x01, 0x35, 0x00, 0xCF, 0xF0, 0x00, 0x00, 0x00, 0x44,
    0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x41, 0x02, 0x00, 0x0F
};

/* Test 32: pointer byte $1F, Emetcon test address without flags */
static const uint8_t msg04[24] = {
    0xFF, 0x55, 0x55, 0x55, 0x1F, 0x00, 0xC0, 0x00, 0x00, 0x0F, 0xF0, 0x34,
    0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x41, 0x02, 0x00, 0x0F
};

/* Test 71/73 (DCT related): pointer byte $3A */
static const uint8_t msg05[24] = {
    0xFF, 0x00, 0x00, 0x01, 0x3A, 0x00, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x44,
    0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x41, 0x02, 0x00, 0x0F
};

/* ========== Function Prototypes ========== */

/* Core functions */
void reset(void);
void mainLoop(void);
void interruptHandler(void);

/* Display functions */
void dispRefresh(void);
void dispBlank(void);
void dispBCD(void);

/* Timer and I/O functions */
void timer(void);
void getKey(void);

/* Conversion functions */
void addrBin(void);
void addrX4(void);
void bin2BCD(void);

/* Message handling */
void msg1way(void);
void msg2way(void);
void msgLoad(const uint8_t *src);
void msgCmd(void);
void msgDataQry(void);

/* BCH checksum */
void bchUpdate(uint8_t bit);

/* Test suites */
void test35a(void);  /* LMT-1xx test suite */
void suite2(void);   /* 2-way test suite */
void test25b(void);  /* Condensed read test */
void lampTest(void);

/* Error handlers */
void error1(void);   /* No transmit from FCT */
void error2(void);   /* No response from UUT */
void error3(void);   /* Incomplete response */
void error4(void);   /* BCH checksum error */
void error5(void);   /* Other data error */
void error6(void);   /* FCT number/hardware mismatch */

/* ========== Hardware Abstraction Layer ========== */
/* Implement these for your target platform */

static inline void write_pia_port_a(uint8_t val) {
    PIA_PORT_A = val;
}

static inline uint8_t read_pia_port_a(void) {
    return PIA_PORT_A;
}

static inline void write_pia_port_b(uint8_t val) {
    PIA_PORT_B = val;
}

static inline uint8_t read_pia_port_b(void) {
    return PIA_PORT_B;
}

static inline void write_ptm_timer(uint8_t reg, uint16_t val) {
    volatile uint8_t *ptr = (volatile uint8_t*)(MC6840_BASE + reg);
    ptr[0] = (val >> 8) & 0xFF;
    ptr[1] = val & 0xFF;
}

/* ========== Initialization (Reset) ========== */

void reset(void) {
    uint8_t port_b_bit0;
    
    /* Configure MC6821 PIA */
    PIA_CTL_A = 0x30;
    PIA_CTL_B = 0x30;
    
    /* Set port directions: Port A all outputs except PA0, Port B all outputs */
    PIA_PORT_A = 0xFE;
    PIA_PORT_B = 0xFF;
    
    PIA_CTL_A = 0x34;
    PIA_CTL_B = 0x34;
    
    /* Initialize port values: PA4 high (active low), PB = 0 */
    PIA_PORT_A = 0x10;
    PIA_PORT_B = 0x00;
    
    /* Configure MC6840 timer based on frequency setting */
    port_b_bit0 = read_pia_port_b() & 0x01;
    
    if (port_b_bit0 == 0) {
        /* 12.5kHz variant */
        write_ptm_timer(6, 0x0027);  /* Timer 3 */
        write_ptm_timer(4, 0x0199);  /* Timer 2 */
    } else {
        /* 9.6kHz variant (G02) */
        write_ptm_timer(6, 0x0033);  /* Timer 3 */
        write_ptm_timer(4, 0x01AC);  /* Timer 2 */
    }
    
    /* Timer 1 setup */
    write_ptm_timer(2, 0x2133);
    
    /* Configure timer control registers */
    PTM_CTL1 = 0x12;
    PTM_CTL0 = 0x12;
    PTM_CTL1 = 0x13;
    PTM_CTL0 = 0x92;
    PTM_CTL1 = 0xD3;
    
    /* Initialize variables */
    countBit = 4;
    countPulse = 0;
    countPhase = 0;
    M_index = 3;  /* Points to mTable1[3] */
    msgLen = 0;
    bchSum = 0;
    msgBits = 0;
    xmitByte = 1;
    msgEnd = 1;
    keyMask = 0;
    uutType = 0;
    dataNum = 0;
    installFlag = 0;
    xmitStatus = 0;
    bitCount = 0;
    uutGroup = 0;
    xmitFlag = 0;
    addrL4 = 0;
    addrM4 = 0;
    addrH4 = 0;
    
    /* Blank display and turn off Install LED */
    dispBlank();
    ledInstall = LED_OFF;
    dispRefresh();
    
    /* Enter main loop */
    mainLoop();
}

/* ========== Main Loop ========== */

void mainLoop(void) {
    uint8_t cfg_val, sw_val;
    uint8_t sum;
    int i;
    
    /* Test 01: Validate switch and jumper settings */
    testOne = 0x01;
    testTen = 0x00;
    dispRefresh();
    
    waitTime = 4;
    timer();
    
test01_1:
    cfg_val = CFG_JPR & 0x03;
    sw_val = TEST_SW & 0x03;
    
    if (cfg_val != sw_val) {
        ledStat = LED_FAIL;
        dispRefresh();
        waitTime = 4;
        timer();
        goto test01_1;
    }
    
    ledStat = LED_OFF;
    dispRefresh();
    
    /* Determine UUT type from switch position */
    sw_val = TEST_SW & 0x03;
    if (sw_val == 0x03) {
        sw_val = 0x04;  /* DCT */
    }
    uutType = sw_val;
    
    /* Test 02: Setup / full 7+1 display */
    dispBlank();
    testOne = 0x02;
    testTen = 0x00;
    memset(readout, 0, 8);
    dispRefresh();
    keyMask = 0xAA;
    
    /* Main keypad loop */
loopMain:
    getKey();
    
    /* Check function keys based on keyMask */
    if ((keyMask & 0x80) && (dataKey == 0x0B)) {
        /* Check key - clear display */
        goto main00;
    }
    
    if ((keyMask & 0x40) && (dataKey == 0x13)) {
        /* Repeat key */
        goto main05;
    }
    
    if ((keyMask & 0x20) && (dataKey == 0x1A)) {
        /* Enter key */
        goto main20;
    }
    
    if ((keyMask & 0x10) && (dataKey == 0x1B)) {
        /* Next key */
        goto main05;
    }
    
    if ((keyMask & 0x08) && (dataKey == 0x03)) {
        /* Test key */
        goto main16;
    }
    
    /* Check if number was entered */
    if (dataNum < 0) {
        goto loopMain;
    }
    
    /* Shift display based on keyMask bits 0/1 */
    if (keyMask & 0x02) {
        /* Full 8-digit shift */
        for (i = 7; i > 0; i--) {
            readout[i] = readout[i-1];
        }
    } else if (keyMask & 0x01) {
        /* 2-digit shift */
        readout[2] = readout[1];
        readout[1] = readout[0];
    } else {
        /* Single digit */
        readout[1] = readout[0];
    }
    readout[0] = dataNum;
    dispRefresh();
    goto loopMain;

main00:
    /* Clear display based on keyMask bits 0/1 */
    readout[0] = 0;
    readout[1] = 0;
    if (keyMask & 0x03) {
        readout[2] = 0;
        if ((keyMask & 0x03) != 0x01) {
            memset(&readout[3], 0, 5);
        }
    }
    dispRefresh();
    goto loopMain;

main05:
    /* Next/Repeat key - exit to test suites */
    keyMask = 0;
    if (uutType == 1) {
        test35a();
        return;
    }
    if (testSet == 8) {
        test25b();
        return;
    }
    suite2();
    return;

main16:
    /* Test key - quick read or lamp test */
    if (testOne != 0x02 || testTen != 0x00) {
        lampTest();
        return;
    }
    
    /* Sum all readout digits */
    sum = 0;
    for (i = 7; i >= 0; i--) {
        sum += readout[i];
    }
    
    if (sum == 0) {
        lampTest();
        return;
    }
    
    /* Validate checksum */
    if ((sum & 0x0F) != readout[0]) {
        ledStat = LED_FAIL;
        dispRefresh();
        goto loopMain;
    }
    
    ledStat = LED_PASS;
    dispRefresh();
    waitTime = 4;
    timer();
    testSet = 8;
    addrBin();
    addrX4();
    test25b();
    return;

main20:
    /* Enter key - check display length and checksum */
    {
        uint8_t *ptr;
        uint8_t checksum;
        
        if (readout[7] != 0x0F) {
            ptr = &readout[7];
        } else {
            ptr = &readout[2];
        }
        
        /* Calculate BCD checksum */
        checksum = 0;
        while (ptr != readout) {
            checksum += *ptr;
            /* DAA equivalent for BCD addition */
            if ((checksum & 0x0F) > 9) {
                checksum += 6;
            }
            if (checksum > 0x99) {
                checksum -= 0xA0;
            }
            ptr--;
        }
        
        if ((checksum & 0x0F) != readout[0]) {
            ledStat = LED_FAIL;
            dispRefresh();
            goto loopMain;
        }
    }
    
    /* Checksum good - route based on keyMask */
    if (keyMask & 0x01) {
        /* Process relay control number */
        rlyNum = readout[2] & 0x07;
        fctCtlNum = readout[1] & 0x03;
        if (fctCtlNum != 0) {
            rlyNum--;
        }
        
        installFlag = 0;
        if (uutType == 1) {
            goto main27;
        }
        
        /* Step 4 setup */
        dispBlank();
        testOne = 0x04;
        testTen = 0x00;
        readout[0] = 0;
        readout[1] = 0;
        ledStat = LED_PASS;
        dispRefresh();
        keyMask = 0xA0;
        goto loopMain;
    }
    
    if (keyMask & 0x02) {
        /* Convert BCD to binary address */
        addrBin();
        
        /* Step 3 setup */
        dispBlank();
        testOne = 0x03;
        testTen = 0x00;
        memset(readout, 0, 3);
        ledStat = LED_PASS;
        dispRefresh();
        keyMask = 0xA9;
        goto loopMain;
    }
    
    /* Process test mode selection */
    switch (readout[1]) {
        case 1:  /* Install */
            testSet = 1;
            installFlag = 1;
            break;
        case 3:  /* Test */
            testSet = 2;
            break;
        case 7:  /* Read/test */
            testSet = 0;
            break;
        case 9:  /* DCT */
            testSet = 3;
            break;
        default:
            ledStat = LED_FAIL;
            dispRefresh();
            goto loopMain;
    }

main27:
    /* Set Install LED */
    ledInstall = installFlag ? LED_ALL_ON : LED_OFF;
    
    /* Step 5 setup - test number only */
    dispBlank();
    testOne = 0x05;
    testTen = 0x00;
    ledStat = LED_PASS;
    dispRefresh();
    keyMask = 0x10;
    goto loopMain;
}

/* ========== Display Functions ========== */

void dispRefresh(void) {
    int i;
    volatile uint8_t delay;
    
    /* Initialize 8279 display controller */
    KD_CMD = 0xDC;  /* Command 6: Reset IC */
    
    /* Wait for reset interval */
    for (delay = 100; delay > 0; delay--) {
        /* Busy wait */
    }
    
    KD_CMD = 0xAA;  /* Command 5: Set masking (disable port A) */
    KD_CMD = 0x18;  /* Command 0: Set mode - 16 chars right-hand, 2-key lockout */
    KD_CMD = 0x2A;  /* Command 1: Set clock divisor (1MHz / 10 = 100kHz) */
    
    /* Send display buffer to 8279 */
    KD_CMD = 0x90;  /* Command 4: Start write to RAM */
    
    for (i = 0; i < 16; i++) {
        KD_DATA = ((uint8_t*)&readout)[i];
    }
    
    KD_CMD = 0x80;  /* Command 4: End write to RAM */
}

void dispBlank(void) {
    memset(readout, LED_OFF, 8);
    testOne = LED_OFF;
    testTen = LED_OFF;
    ledRlyA = LED_OFF;
    ledRlyB = LED_OFF;
    ledRlyC = LED_OFF;
    ledRlyD = LED_OFF;
    ledStat = LED_OFF;
    /* Note: ledInstall is handled separately */
}

void dispBCD(void) {
    readout[0] = bcdData10 & 0x0F;
    readout[1] = bcdData10 >> 4;
    readout[2] = bcdData32 & 0x0F;
    readout[3] = bcdData32 >> 4;
    readout[4] = bcdData54 & 0x0F;
    readout[5] = bcdData54 >> 4;
    readout[6] = bcdData76 & 0x0F;
    readout[7] = bcdData76 >> 4;
    ledStat = LED_PASS;
}

/* ========== Timer Function ========== */

void timer(void) {
    /* Delay timer: 1/16 second per count using PTM channel 1 */
    PTM_CTL0 = 0x22;
    
    while (waitTime > 0) {
        /* Load timer with max value */
        write_ptm_timer(2, 0xFFFF);
        
        /* Wait for timer expiration (poll control register) */
        while ((PTM_CTL1 & 0x01) == 0) {
            /* Busy wait */
        }
        
        waitTime--;
    }
}

/* ========== Keyboard Input ========== */

void getKey(void) {
    uint8_t key;
    
    /* Wait for keypress (poll keyboard IRQ) */
    while ((KB_EIRQ & 0x01) == 0) {
        /* Busy wait */
    }
    
    /* Get key data from 8279 */
    KD_CMD = 0x50;  /* Command 2: Get key data */
    key = KD_DATA & 0x1F;
    dataKey = key;
    
    /* Convert matrix position to number (-1 if not a number key) */
    dataNum = -1;
    
    switch (key) {
        case 0x18: dataNum = 0; break;
        case 0x10: dataNum = 1; break;
        case 0x11: dataNum = 2; break;
        case 0x12: dataNum = 3; break;
        case 0x08: dataNum = 4; break;
        case 0x09: dataNum = 5; break;
        case 0x0A: dataNum = 6; break;
        case 0x00: dataNum = 7; break;
        case 0x01: dataNum = 8; break;
        case 0x02: dataNum = 9; break;
    }
    
    /* Check for soft reset key */
    if (key == 0x19) {
        reset();
    }
}

/* ========== Address Conversion Functions ========== */

void addrBin(void) {
    /* Convert 8-digit unpacked BCD to 24-bit binary */
    uint32_t result = 0;
    int i;
    
    for (i = 7; i >= 0; i--) {
        result = result * 10 + readout[i];
    }
    
    addrL = result & 0xFF;
    addrM = (result >> 8) & 0xFF;
    addrH = (result >> 16) & 0xFF;
}

void addrX4(void) {
    /* Shift address left by 2 bits */
    uint32_t addr = ((uint32_t)addrH << 16) | ((uint32_t)addrM << 8) | addrL;
    addr <<= 2;
    
    addrL4 = addr & 0xFF;
    addrM4 = (addr >> 8) & 0xFF;
    addrH4 = (addr >> 16) & 0xFF;
}

void bin2BCD(void) {
    /* Convert binary value to packed BCD */
    /* This is a placeholder - implement based on f2binary, f2BCD, f2bytes, f2bits */
    uint8_t *bcd_ptr = (uint8_t*)f2BCD;
    uint8_t *bin_ptr = (uint8_t*)f2binary;
    uint8_t bytes = f2bytes;
    uint8_t bits = f2bits;
    int i;
    
    /* Clear BCD output */
    for (i = 0; i < bytes; i++) {
        bcd_ptr[i] = 0;
    }
    
    /* Double-dabble algorithm for binary to BCD conversion */
    /* Implementation depends on actual data locations */
}

/* ========== Message Handling ========== */

void msg1way(void) {
    /* Message template (Type A) for 1-way units */
    msgTbl[0] = 0xAA;
    msgTbl[1] = 0xB8;
    msgTbl[2] = 0xFF;
    msgTbl[3] = addrL4;
    msgTbl[4] = 0x00;  /* Command byte / data pointer */
    msgTbl[5] = 0x00;  /* Response flag: ALWAYS 0 on 1-way units */
    
    xmitByte = 1;
    msgBits = 0x2B;    /* 43 bits */
    msgLen = 6;
}

void msg2way(void) {
    /* Message template (Type B) for 2-way units */
    msgTbl[0] = 0xAA;
    msgTbl[1] = 0xBA;
    msgTbl[2] = 0xFF;
    msgTbl[3] = addrH4;
    msgTbl[4] = addrM4;
    msgTbl[5] = addrL4;
    msgTbl[6] = 0x00;  /* Command byte / data pointer */
    msgTbl[7] = 0x00;  /* Response flag: set to 0x40 if reply required */
    
    xmitByte = 1;
    msgBits = 0x41;    /* 65 bits */
    msgLen = 8;
}

void msgLoad(const uint8_t *src) {
    /* Load long-form message from table */
    int i;
    
    msgTbl[0] = 0xAA;
    msgTbl[1] = 0xBA;
    
    /* Copy 20 bytes of message data */
    for (i = 0; i < 20; i++) {
        msgTbl[2 + i] = src[i];
    }
    
    /* Load overhead variables from last 4 bytes */
    msgBits = src[20];
    xmitByte = src[21];
    msgLen = src[23];  /* Note: src[22] would be high byte, always 0 */
}

void msgCmd(void) {
    /* Send command message to UUT */
    xmitStatus = 1;
    xmitFlag = 1;
    
    timer();
    
    /* Check for error */
    if (xmitStatus > 0) {
        error1();  /* No transmit */
        return;
    }
    
    xmitFlag = 0;
}

void msgDataQry(void) {
    /* Request data from UUT */
    
    /* Set address/reply flags */
    msgTbl[5] |= 0x01;
    msgTbl[7] = 0x40;
    
    xmitStatus = 1;
    xmitFlag = 1;
    waitTime = 5;
    timer();
    
    /* Check for error */
    if (msgBits >= 0x41) {
        error1();  /* No transmit */
        return;
    }
    
    /* Wait for transmission complete */
    while (msgBits != 0) {
        /* Busy wait */
    }
    
    /* Set data packet parameters */
    msgLen = 7;
    xmitByte = 1;
    
    /* Begin data retrieval */
    waitTime = 7;
    timer();
    
    /* Check for response */
    if (msgBits == 0) {
        error2();  /* No response */
        return;
    }
    
    /* Wait for complete message */
    while (msgBits != 0x34) {
        /* Busy wait */
    }
    msgBits = 0;
    
    /* Check BCH */
    if (bchSum != 0) {
        error4();  /* BCH error */
        return;
    }
    
    xmitFlag = 0;
}

/* ========== BCH Checksum ========== */

void bchUpdate(uint8_t bit) {
    /* Update BCH checksum with new bit */
    bchSum <<= 1;
    
    if (bit & 0x01) {
        if (!(bchSum & 0x40)) {
            bchSum ^= 0x03;
        }
    } else {
        if (bchSum & 0x40) {
            bchSum ^= 0x03;
        }
    }
    
    bchSum &= 0x3F;
}

/* ========== Error Handlers ========== */

void error1(void) {
    /* No transmit from FCT */
    readout[0] = 1;
    ledStat = LED_FAIL;
    dispRefresh();
    xmitFlag = 0;
    keyMask = 0x48;
}

void error2(void) {
    /* No response from UUT */
    readout[0] = 2;
    ledStat = LED_FAIL;
    dispRefresh();
    xmitFlag = 0;
    keyMask = 0x48;
}

void error3(void) {
    /* Incomplete response from UUT */
    readout[0] = 3;
    ledStat = LED_FAIL;
    dispRefresh();
    xmitFlag = 0;
    keyMask = 0x48;
}

void error4(void) {
    /* BCH checksum error in UUT response */
    readout[0] = 4;
    ledStat = LED_FAIL;
    dispRefresh();
    xmitFlag = 0;
    keyMask = 0x48;
}

void error5(void) {
    /* Other data error */
    readout[0] = 5;
    ledStat = LED_FAIL;
    dispRefresh();
    xmitFlag = 0;
    keyMask = 0x48;
}

void error6(void) {
    /* FCT number / hardware mismatch */
    readout[0] = 6;
    ledStat = LED_FAIL;
    dispRefresh();
    xmitFlag = 0;
    keyMask = 0x48;
}

/* ========== Test Suites ========== */

void lampTest(void) {
    int i;
    
    /* Light all display segments and LEDs */
    memset(readout, LED_ALL_ON, 8);
    testOne = LED_ALL_ON;
    testTen = LED_ALL_ON;
    ledRlyA = LED_ALL_ON;
    ledRlyB = LED_ALL_ON;
    ledRlyC = LED_ALL_ON;
    ledRlyD = LED_ALL_ON;
    ledInstall = LED_ALL_ON;
    ledStat = LED_ALL_ON;
    dispRefresh();
    
    waitTime = 0x2E;  /* About 2.9 seconds */
    timer();
    
    /* Show firmware revision: '060  8' = S00060 Rev H */
    dispBlank();
    ledInstall = 0xFF;
    readout[2] = 8;   /* Rev H */
    readout[5] = 0;
    readout[6] = 6;
    readout[7] = 0;
    dispRefresh();
    
    waitTime = 0x24;  /* About 2.25 seconds */
    timer();
    
    /* Reboot */
    reset();
}

void test35a(void) {
    /* LMT-1xx test suite - Test timed relays (A-D) */
    dispBlank();
    testOne = 0x05;
    testTen = 0x03;
    dispRefresh();
    
    /* Send message to UUT - shed relays */
    msg1way();
    msgTbl[4] = 0xE0;
    waitTime = 0x10;
    msgCmd();
    
    /* Flash test LEDs while waiting */
    temp7 = 8;
    while (temp7 > 0) {
        waitTime = 0x10;
        timer();
        ledStat = LED_ALL_ON;
        dispRefresh();
        waitTime = 0x10;
        timer();
        ledStat = LED_OFF;
        dispRefresh();
        temp7--;
    }
    
    /* Continue with relay tests... */
    /* (Additional test implementation would go here) */
}

void suite2(void) {
    /* 2-way test suite placeholder */
    /* Implement based on testSet value */
}

void test25b(void) {
    /* Condensed read test placeholder */
}

/* ========== Interrupt Handler ========== */

void interruptHandler(void) {
    uint8_t pia_val;
    int8_t a_reg, b_reg;
    int i;
    
    /* Check if actively listening */
    if (xmitFlag == 0) {
        /* Cleanup and exit */
        msgStart = 0;
        countBit = 5;
        mSum_peak = 0;
        mSum_last = 0;
        
        for (i = 0; i < 4; i++) {
            mTable1[i] = 0;
            mTable2[i] = 0;
        }
        return;
    }
    
    /* Gather next bit and advance receive counter */
    pia_val = read_pia_port_a();
    byteNow = (byteNow << 1) | (pia_val & 0x01);
    countPulse--;
    
    if ((countPulse & 0x07) != 0) {
        return;
    }
    
    /* 8 bits received - transfer completed byte */
    byteLast = byteNow;
    
    /* Set PTM channel 3 output based on xmitStatus */
    PTM_CTL1 = 0x52;
    PTM_CTL0 = (xmitStatus > 0) ? 0x92 : 0x12;
    PTM_CTL1 = 0x53;
    
    /* Advance phase interval counter */
    countPhase--;
    if ((countPhase & 0x03) != 0) {
        goto phase_detect;
    }
    
    /* Mode check - route to receiver or transmitter */
    xmitStatus--;
    if (xmitStatus == 0) {
        /* Transmitter mode */
        xmitStatus++;
        msgBits--;
        
        if (msgBits == 0) {
            xmitByte--;
            if (xmitByte == 0) {
                /* Done - clean up */
                pia_val = read_pia_port_a() & 0xF9;
                write_pia_port_a(pia_val);
                xmitStatus--;
                bitCount++;
                msgStart = 0;
                countBit = 5;
                return;
            }
            msgBits = 0x34;
            bchSum = 0;
        }
        
        /* BCH handling and data transmission */
        if (msgBits == 6) {
            msgTbl[0] = (bchSum << 2) + msgTbl[0];
        }
        
        /* Rotate message bits out */
        a_reg = 0;
        for (i = msgLen; i > 0; i--) {
            uint8_t carry = (msgTbl[i-1] & 0x80) ? 1 : 0;
            msgTbl[i-1] <<= 1;
            msgTbl[i-1] |= (a_reg & 0x01);
            a_reg = carry;
        }
        
        /* Update BCH and send bit */
        bchUpdate(a_reg);
        
        pia_val = read_pia_port_a() & 0xF9;
        pia_val |= ((a_reg & 0x01) << 1) | 0x04;
        write_pia_port_a(pia_val);
        return;
    }
    
    if (xmitStatus < 0) {
        /* Receiver mode */
        xmitStatus++;
        countBit--;
        
        /* Receiver processing... */
        /* (Full receiver state machine would go here) */
    }
    return;

phase_detect:
    /* Phase detection and vector calculations */
    /* Based on US Patent 4311964 */
    
    /* Vector calculations using freq1/freq2 tables */
    /* Signal level detection */
    /* Phase detector output */
    
    /* M-Store / M-Sum table operations */
    {
        int8_t val = mValue_raw >> 1;
        mTable1[M_index & 0x03] = val;
        
        /* Sum mTable1 */
        mSum_Cur = 0;
        for (i = 0; i < 4; i++) {
            mSum_Cur += mTable1[i];
        }
        
        /* Absolute value for mTable2 */
        val = (mSum_Cur < 0) ? -mSum_Cur : mSum_Cur;
        val >>= 1;
        mTable2[M_index & 0x03] += val;
        
        /* Update index */
        M_index--;
        if (M_index > 3) {
            M_index = 3;
        }
        
        /* Peak detection */
        countSum++;
        if (mSum_peak >= 0 && val >= mSum_peak) {
            mSum_peak = val;
            countSum = 0;
        }
        
        if ((countSum & 0x03) == 0) {
            mSum_last = mSum_Cur;
        }
    }
}

/* ========== Main Entry Point ========== */

int main(void) {
    reset();
    return 0;
}