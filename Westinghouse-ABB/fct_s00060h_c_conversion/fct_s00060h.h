/*
 * S00060-H / MPU: SC44125P
 * Westinghouse Field Configuration Terminal - unit 1993D40G06 (12.5kHz)
 * 
 * C conversion of THRsim11 assembly code
 * 
 * EPROMS: 2732
 * U11 label: S00060G - U11 / 5553C01G17 - G01
 * U17 label: S00060G - U17 / 5553C01G18 - G01
 *
 * Product notes:
 * The FCT is a portable unit for servicing LMTs / MCTs / DCTs
 * Unit contains an LMT-2 board set, assembled as two panels (A/C, B/D)
 * Unit also has an interface board, display board, and power distribution board
 */

#ifndef FCT_S00060H_H
#define FCT_S00060H_H

#include <stdint.h>
#include <stdbool.h>

/* ========== Hardware Addresses ========== */
/* Address allocation is due to partial address decoding */

#define MC6840_BASE     0x0000  /* Programmable Timer Module */
#define MC6821_BASE     0x000C  /* Peripheral Interface Adapter */
#define RAM_LO_BASE     0x0080  /* MC6810 SRAM */
#define RAM_HI_BASE     0x0C00  /* HM6561 x 2 */

/* MC6840 Programmable Timer */
#define PTM_CTL0        (MC6840_BASE + 0)   /* Control register: Timer 1 and Timer 3 */
#define PTM_CTL1        (MC6840_BASE + 1)   /* Control register: Timer 2 */
#define PTM_TM1         (MC6840_BASE + 2)   /* Timer 1 - Programmable delay */
#define PTM_TM2         (MC6840_BASE + 4)   /* Timer 2 - drives MPU's NMI line */
#define PTM_TM3         (MC6840_BASE + 6)   /* Timer 3 - XOR'd with PIA PA1 for output */

/* MC6821 Peripheral Interface Adapter */
#define PIA_PORT_A      (MC6821_BASE + 0)
#define PIA_CTL_A       (MC6821_BASE + 1)
#define PIA_PORT_B      (MC6821_BASE + 2)
#define PIA_CTL_B       (MC6821_BASE + 3)

/*
 * PIA Pin Definitions:
 * Port A:
 *   PA0 - Input; data from receiver (Panel A)
 *   PA1 - Output; XOR'd with PTM timer 3 before going to Panel B
 *   PA2 - Output; to panel B via panel D (transmitter enable / active high)
 *   PA3 - No longer used. Trace on Panel D is bridged to PA2 instead
 *   PA4 - Output; Initialization Enable signal to Panel D
 * Port B:
 *   PB0 - Used for frequency setting code
 */

/* Function selector (74LS138 on FCT Logic Board) */
#define TESTSW          (RAM_HI_BASE + 249)  /* Tester mode switch */
#define KB_EIRQ         (RAM_HI_BASE + 250)  /* 8279 keyboard IRQ */
#define RLY_STATUS      (RAM_HI_BASE + 252)  /* External contact closures */
#define CFG_JPR         (RAM_HI_BASE + 253)  /* Mode jumpers */
#define KD_DATA         (RAM_HI_BASE + 254)  /* 8279 data port */
#define KD_CMD          (RAM_HI_BASE + 255)  /* 8279 command port */

/* LED values: F = off, 4 = fail, 7 = pass, 8 = all on */
#define LED_OFF         0x0F
#define LED_FAIL        0x04
#define LED_PASS        0x07
#define LED_ALL_ON      0x08

/* Key codes */
#define KEY_CHECK       0x0B
#define KEY_REPEAT      0x13
#define KEY_ENTER       0x1A
#define KEY_NEXT        0x1B
#define KEY_TEST        0x03
#define KEY_RESET       0x19
#define KEY_0           0x18
#define KEY_1           0x10
#define KEY_2           0x11
#define KEY_3           0x12
#define KEY_4           0x08
#define KEY_5           0x09
#define KEY_6           0x0A
#define KEY_7           0x00
#define KEY_8           0x01
#define KEY_9           0x02

/* UUT Types */
#define UUT_LMT1XX      1   /* LMT-1xx */
#define UUT_LMT2_MCT    2   /* LMT-2 / MCT-2xx */
#define UUT_DCT         4   /* DCT */

/* ========== Hardware Access Functions (Platform Specific) ========== */
/* These must be implemented for your target platform */

extern void     hw_write_byte(uint16_t addr, uint8_t value);
extern uint8_t  hw_read_byte(uint16_t addr);
extern void     hw_write_word(uint16_t addr, uint16_t value);
extern uint16_t hw_read_word(uint16_t addr);

/* ========== Variable Structure ========== */

typedef struct {
    /* Low RAM (MC6810: 128 x 8 SRAM / $0080 - $00FF) */
    uint8_t  bchSum;            /* BCH checksum - data good if 0 after check */
    uint8_t  byteNow;           /* Current incoming byte (bit stream) */
    uint8_t  byteLast;          /* Most recently completed byte */
    uint8_t  byteOld;           /* Previously completed byte */
    int8_t   valueIi;           /* Raw vector data - fundamental, 0 degree */
    int8_t   valueQi;           /* Raw vector data - fundamental, 90 degree */
    int8_t   valueI2i;          /* Raw vector data - 2nd harmonic, 0 degree */
    int8_t   valueQ2i;          /* Raw vector data - 2nd harmonic, 90 degree */
    int8_t   avgI2H;            /* Running average - high byte of vector I */
    int8_t   avgQ2H;            /* Running average - high byte of vector Q */
    uint8_t  avgI2L;            /* Running average - low byte of vector I */
    uint8_t  avgQ2L;            /* Running average - low byte of vector Q */
    int8_t   avgQ20;            /* Previous value of Q2H */
    int8_t   vectorI;           /* Calculated vector - 0 degrees */
    int8_t   vectorQ;           /* Calculated vector - 90 degrees */
    int8_t   sgnFlag;           /* Flag for polarity transitions on vector Q */
    uint8_t  sigLevel;          /* Received signal level (9 is cutoff) */
    int8_t   vectorSum;         /* Output byte of phase detector */
    uint8_t  temp1;             /* Scratchpad byte - interrupt */
    uint8_t  temp2;             /* Scratchpad byte - interrupt */
    int8_t   mTable1[4];        /* Table - convert vectorSum values into bits */
    int8_t   mTable2[4];        /* Table - convert vectorSum values into bits */
    uint8_t  msgTbl[22];        /* In/out message buffer (receive is 6 bytes max) */
    uint8_t  msgBits;           /* Number of bits in message */
    int8_t   xmitStatus;        /* Transmitter status flag */
    uint8_t  xmitByte;          /* Carrier segment counter */
    uint8_t  keyMask;           /* Function key enable and display shifting */
    uint8_t  uutType;           /* UUT type: 1=LMT-1xx, 2=LMT-2/MCT, 4=DCT */
    int8_t   dataNum;           /* Last number pressed (-1 for NaN) */
    uint8_t  dataKey;           /* Last key pressed (matrix 0x00 to 0x1B) */
    uint8_t  waitTime;          /* Delay counter (1/16 sec per count) */
    uint8_t  temp6;             /* Scratchpad byte - application */
    uint8_t  temp7;             /* Scratchpad byte - application */
    uint8_t  temp8;             /* Scratchpad byte - application */
    uint8_t  temp9;             /* Scratchpad byte - application */
    uint8_t  rlyTemp;           /* Temporary storage of pre-test relay state */
    uint8_t  rlyLatch;          /* Temporary storage of latched relay state */
    uint8_t  addrL;             /* Address (24 bits - big endian) */
    uint8_t  addrM;
    uint8_t  addrH;
    uint8_t  xmitFlag;          /* Activates transmitter */
    int8_t   headerBit;         /* Used to count off bits in preamble */
    int8_t   mSum_peak;         /* Highest working value - mTable1/2 */
    int8_t   mValue_raw;        /* Value to carry into M tables */
    int8_t   msgStart;          /* Flag to mark end of message preamble */
    uint8_t  installFlag;       /* Used to illuminate/clear Install LED */
    uint8_t  bitCount;          /* Flag to mark end of each received bit */
    uint8_t  msgEnd;            /* Flag to mark end of received message */
    uint8_t  countPulse;        /* Running counter - bit stream (8) */
    uint8_t  countPhase;        /* Running counter - carrier pulse interval (4) */
    int8_t   countBit;          /* Bit interval counter (-1, 0-5) */
    uint8_t  countSum;          /* Running counter for mTable calculations */
    uint8_t  M_index;           /* Pointer for mTable1/mTable2 (0-3) */
    int8_t   mSum_last;         /* Previous working value - mTable1/2 */
    int8_t   mSum_Old;          /* 2nd previous working value */
    int8_t   mSum_cur;          /* Current working value */
    uint8_t  dctTemp;           /* Placeholder for DCT analog data display */
    uint8_t  optbyte;           /* Test validation byte from LMT-2/DCT */
    uint8_t  testSet;           /* Selects test suite for 2-way units */

    /* High RAM (HM-6561 NVRAM x 2 / $0C00 - $0C7F) */
    uint16_t msgLen;            /* Message length (upper byte is 0) */
    uint8_t  fctCtlNum;         /* FCT Control Number */
    uint8_t  rlyNum;            /* Number of relays to test */
    uint8_t  uutFWbyte;         /* Firmware number in UUT */
    uint8_t  uutGroup;          /* Flag to direct tests (based on UUT FW) */
    uint8_t  uutFWspec;         /* Firmware revision in UUT */
    uint8_t  bcdData10;         /* BCD data buffer (8 digits, packed) */
    uint8_t  bcdData32;
    uint8_t  bcdData54;
    uint8_t  bcdData76;
    uint8_t  addrL4;            /* Address, shifted left 2 bits */
    uint8_t  addrM4;
    uint8_t  addrH4;
    uint8_t  lmtTestMd;         /* Flag to remember/set UUT test mode */
    uint16_t f2binary;          /* Location of source binary number */
    uint16_t f2BCD;             /* Location of packed BCD after conversion */
    uint8_t  f2bitpos;          /* Bit marker for binary/BCD conversion */
    uint8_t  f2bytes;           /* Number of bytes (+1) to convert */
    uint8_t  f2bits;            /* Number of bits to convert */
    uint8_t  dctData1;          /* Local copy of DCT upper RAM $0C3A */
    uint8_t  dctData2;          /* Local copy of DCT upper RAM $0C3B */
    uint8_t  dctData3;          /* Local copy of DCT upper RAM $0C3C */
    uint8_t  dctData4;          /* Local copy of DCT upper RAM $0C3D */
    uint8_t  dctTouStat;
    uint8_t  rlyError;          /* Count relay failure */
    uint8_t  rlyBits;           /* Evaluate relay closures */

    /* Display RAM (readout is 8 bytes, stored big-endian) */
    uint8_t  readout[8];        /* Main readout (8 digits) */
    uint8_t  testOne;           /* Test number ones digit */
    uint8_t  testTen;           /* Test number tens digit */
    uint8_t  ledRlyA;           /* Relay A status LED */
    uint8_t  ledRlyB;           /* Relay B status LED */
    uint8_t  ledRlyC;           /* Relay C status LED */
    uint8_t  ledRlyD;           /* Relay D status LED */
    uint8_t  ledInstall;        /* Install mode LED */
    uint8_t  ledStat;           /* Test pass/fail LED */
} FCT_State;

/* ========== Vector Sum Calculation Tables ========== */
/* freq1: 32, 23, 0, -23, -32, -23, 0, 23, 32, 23 */
/* freq2: 1, 0, -1, 0, 1, 0, -1, 0, 1 */

extern const int8_t freq1[10];
extern const int8_t freq2[9];

/* ========== Function Prototypes ========== */

/* Initialization */
void fct_reset(FCT_State *state);

/* Main loop */
void fct_main_loop(FCT_State *state);

/* Display functions */
void disp_refresh(FCT_State *state);
void disp_blank(FCT_State *state);
void disp_bcd(FCT_State *state);

/* Input functions */
uint8_t get_key(FCT_State *state);
uint8_t get_key_dct(FCT_State *state);

/* Timer function */
void timer_delay(FCT_State *state);

/* Address conversion functions */
void addr_bin(FCT_State *state);
void addr_x4(FCT_State *state);
void bin_to_bcd(FCT_State *state);

/* Message handling */
void msg_1way(FCT_State *state);
void msg_2way(FCT_State *state);
void msg_load(FCT_State *state, const uint8_t *msg_data);
void msg_cmd(FCT_State *state);
void msg_data_qry(FCT_State *state);
void send_msg_dct(FCT_State *state);

/* Display UUT firmware info */
void get_fw_rev(FCT_State *state);
void disp_dct(FCT_State *state);

/* Test suites */
void test_1way_suite(FCT_State *state);
void test_2way_suite(FCT_State *state);
void lamp_test(FCT_State *state);

/* Error handlers */
void error_handler(FCT_State *state, uint8_t error_code);

/* Interrupt handler (NMI) */
void nmi_interrupt(FCT_State *state);

/* BCH checksum */
void bch_update(FCT_State *state, uint8_t bit_value);

#endif /* FCT_S00060H_H */
