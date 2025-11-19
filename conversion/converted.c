/*
 * Westinghouse LMT-100 Load Management Terminal
 * CPSK Demodulation Algorithms converted from 6801 Assembly to C
 * Based on US Patent 4311964 - Coherent Phase-Shift Keying
 */

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Data Structures
// ============================================================================

// Sine wave lookup table for phase calculations
// Values are signed 8-bit: 244 = -12, 239 = -17
const int8_t sineTb[10] = {17, 12, 0, -12, -17, -12, 0, 12, 17, 12};

// Demodulator group structure (4 groups total)
typedef struct {
    int8_t valIi;      // Current 0-degree value
    int8_t valQi;      // Current 90-degree value
    int8_t vecSum;     // Current vector sum
    int16_t avgI;      // Rolling I average (16-bit for precision)
    int16_t avgQ;      // Rolling Q average (16-bit for precision)
    int8_t mSum;       // Current M value
} DemodGroup;

// Demodulator state
typedef struct {
    uint8_t byteNow;       // Incoming bits being gathered
    uint8_t byteLast;      // Byte being evaluated for transitions
    uint8_t byteOld;       // Previously evaluated byte
    int8_t valueIi;        // Instantaneous 0-degree value
    int8_t valueQi;        // Instantaneous 90-degree value
    uint8_t ctPhse;        // Current phase (0, 8, 16, 24)
    DemodGroup groups[4];  // Four demodulation groups
    int8_t mSumLst;        // Previous M-Sum value
    int8_t mSumOld;        // Previously analyzed M-Sum
    int8_t MValue;         // Calculated M value
    uint8_t bitIntv;       // Bit interval counter
    bool msgStrt;          // Message start flag
} DemodState;

// Message reception state
typedef struct {
    uint8_t msgTbl[5];     // Incoming message buffer (40 bits)
    uint8_t rcvByt;        // Assembled byte after header
    uint8_t bchSum;        // BCH checksum (6-bit)
    uint8_t segBit;        // Bits to count in segment
    uint8_t msgRej;        // Message rejected flag
    uint8_t unitID;        // Unit type from message
} MsgState;

// ============================================================================
// 1. BIT TRANSITION DETECTION AND I/Q CALCULATION
// ============================================================================

/*
 * Examines an 8-bit pattern to find bit transitions and calculates
 * instantaneous I (0-degree) and Q (90-degree) values
 * 
 * Uses XOR to detect transitions between consecutive bits.
 * When a transition is found, looks up sine wave value and adds/subtracts
 * based on transition direction (0->1 is positive, 1->0 is negative)
 */
void bitTest(DemodState *state) {
    int8_t valueIi = 0;
    int8_t valueQi = 0;
    uint8_t bitA = state->byteOld;
    
    // Process 8 bits, looking for transitions
    for (int i = 0; i < 8; i++) {
        uint8_t bitB = state->byteLast;
        
        // XOR consecutive bits - if result is negative, we have a transition
        if ((int8_t)(bitA ^ bitB) < 0) {
            // Get 0-degree value from sine table
            int8_t val0 = sineTb[i];
            if ((int8_t)bitA >= 0) {
                // Positive bit: negate the value
                val0 = -val0;
            }
            valueIi += val0;
            
            // Get 90-degree value (offset by 2 in table)
            int8_t val90 = sineTb[i + 2];
            if ((int8_t)bitA >= 0) {
                val90 = -val90;
            }
            valueQi += val90;
        }
        
        // Prepare for next bit
        bitA = bitB;
        state->byteLast = (state->byteLast << 1) | (state->byteLast >> 7); // ROL
    }
    
    // Save byteLast for next comparison cycle
    state->byteOld = bitA;
    state->valueIi = valueIi;
    state->valueQi = valueQi;
}

// ============================================================================
// 2. PHASE CALCULATION SUBROUTINE
// ============================================================================

/*
 * Calculates phase difference component
 * Multiplies two signed 8-bit values, shifts left by 2, and handles sign
 */
int8_t phsCalc(int8_t a, int8_t b) {
    int8_t phsTmp = a;
    bool negResult = false;
    
    // Make both values positive, tracking if result should be negative
    if (a < 0) {
        a = -a;
    }
    if (b < 0) {
        b = -b;
        negResult = !negResult;
        phsTmp = -phsTmp;
    }
    
    // Multiply and shift left by 2 (multiply by 4)
    int16_t result = (int16_t)a * (int16_t)b;
    result <<= 2;
    
    // Apply sign from original calculation
    int8_t final = (int8_t)(result >> 8); // Take high byte
    if ((int8_t)phsTmp < 0) {
        final = -final;
    }
    
    return final;
}

// ============================================================================
// 3. DEMODULATION - I/Q AVERAGING AND M-VALUE CALCULATION
// ============================================================================

/*
 * Performs the core demodulation algorithm:
 * - Stores instantaneous I/Q values in current group
 * - Calculates running averages across all 4 groups
 * - Computes vector sum and M-value for bit detection
 */
void demodulate(DemodState *state) {
    // Determine which group (0-3) based on phase
    int groupIdx = state->ctPhse / 8;
    DemodGroup *grp = &state->groups[groupIdx];
    
    // Store instantaneous values in current group
    grp->valIi = state->valueIi;
    grp->valQi = state->valueQi;
    
    // Calculate running average of I values across all 4 groups
    int8_t avgI = ((state->groups[0].valIi + state->groups[1].valIi) >> 1);
    avgI = (avgI + ((state->groups[2].valIi + state->groups[3].valIi) >> 1)) >> 1;
    state->valueIi = avgI;
    
    // Calculate vector sum using phase calculation
    int8_t vecSum = phsCalc(avgI, (int8_t)(grp->avgI & 0xFF));
    grp->vecSum = vecSum;
    
    // Calculate running average of Q values across all 4 groups
    int8_t avgQ = ((state->groups[0].valQi + state->groups[1].valQi) >> 1);
    avgQ = (avgQ + ((state->groups[2].valQi + state->groups[3].valQi) >> 1)) >> 1;
    state->valueQi = avgQ;
    
    // Add Q component to vector sum
    vecSum += phsCalc(avgQ, (int8_t)(grp->avgQ & 0xFF));
    grp->vecSum = vecSum;
    
    // Determine polarity from sign bit
    bool negative = (vecSum & 0x80) != 0;
    if (negative) {
        state->valueIi = -state->valueIi;
        state->valueQi = -state->valueQi;
    }
    
    // Calculate M value for I component
    int16_t temp = ((int16_t)state->valueIi << 8) - grp->avgI;
    temp = (temp >> 2) + grp->avgI;  // Divide by 4, add back
    grp->avgI = temp;
    int8_t mValI = (int8_t)(temp >> 8);
    if (mValI < 0) mValI = -mValI;
    grp->mSum = mValI;
    
    // Calculate M value for Q component
    temp = ((int16_t)state->valueQi << 8) - grp->avgQ;
    temp = (temp >> 2) + grp->avgQ;  // Divide by 4, add back
    grp->avgQ = temp;
    int8_t mValQ = (int8_t)(temp >> 8);
    if (mValQ < 0) mValQ = -mValQ;
    
    // Combine I and Q M-values
    if (grp->mSum >= mValQ) {
        mValQ >>= 1;  // Halve the smaller value
    } else {
        grp->mSum >>= 1;
    }
    grp->mSum += mValQ;
}

// ============================================================================
// 4. MESSAGE RECEPTION - BIT DECISION LOGIC
// ============================================================================

/*
 * Determines if the accumulated signal represents a 1 or 0 bit
 * Uses M-value comparisons and timing to make bit decisions
 */
bool msgReceive(DemodState *state, bool *bitOut) {
    bool bitReady = false;
    
    state->bitIntv--;
    
    if (state->bitIntv == 0) {
        // Check for bit transition
        if (((int8_t)(state->mSumLst ^ state->mSumOld)) < 0) {
            state->MValue = state->mSumLst;
        }
        state->bitIntv--;
    } else if (state->bitIntv == -1) {
        state->msgStrt = true;
        int8_t bitVal = state->mSumLst;
        if (state->MValue < 0) {
            bitVal = ~bitVal;
        }
        *bitOut = (bitVal & 0x80) ? 1 : 0;
        bitReady = true;
    } else if (state->bitIntv == -2) {
        // Check for consistent transition
        if (((int8_t)(state->mSumLst ^ state->mSumOld)) >= 0) {
            bitReady = true;
        }
    }
    
    state->mSumOld = state->mSumLst;
    return bitReady;
}

// ============================================================================
// 5. BCH CHECKSUM CALCULATION
// ============================================================================

/*
 * BCH (Bose-Chaudhuri-Hocquenghem) error detection
 * 6-bit checksum - if result is 0 after processing all bits, data is valid
 */
void bchCheck(MsgState *msg, uint8_t dataByte) {
    for (int i = 0; i < 8; i++) {
        msg->bchSum <<= 1;
        
        bool bit0 = (dataByte & 0x01) != 0;
        bool bit6 = (msg->bchSum & 0x40) != 0;
        
        if (bit0 ^ bit6) {
            msg->bchSum ^= 0x03;
        }
        
        msg->bchSum &= 0x3F;  // Keep only 6 bits
        dataByte >>= 1;
    }
}

// ============================================================================
// 6. ROLL BIT INTO MESSAGE TABLE
// ============================================================================

/*
 * Adds one demodulated bit to the message buffer
 * Shifts the 40-bit (5-byte) message buffer left by one bit
 */
void rollBitIntoMessage(MsgState *msg, bool bit) {
    // Rotate bit through entire 40-bit message buffer
    uint8_t carry = bit ? 1 : 0;
    
    for (int i = 4; i >= 0; i--) {
        uint8_t newCarry = (msg->msgTbl[i] & 0x80) ? 1 : 0;
        msg->msgTbl[i] = (msg->msgTbl[i] << 1) | carry;
        carry = newCarry;
    }
    
    // Update assembled byte
    msg->rcvByt = msg->msgTbl[4];
}

// ============================================================================
// EXAMPLE USAGE
// ============================================================================

/*
void example_usage() {
    DemodState demod = {0};
    MsgState msg = {0};
    
    // Initialize demodulation state
    demod.bitIntv = 5;
    
    // Simulate receiving 8 bits from serial input
    // (In real hardware, this comes from PORT1.1 every clock tick)
    for (int i = 0; i < 8; i++) {
        // Roll in next bit (example: alternating pattern)
        demod.byteNow = (demod.byteNow << 1) | (i & 1);
    }
    
    // Process the 8 bits
    demod.byteLast = demod.byteNow;
    bitTest(&demod);
    
    // Perform demodulation
    demodulate(&demod);
    
    // Try to receive a bit
    bool bitValue;
    if (msgReceive(&demod, &bitValue)) {
        // Got a bit! Add to message
        rollBitIntoMessage(&msg, bitValue);
        
        // Update checksum
        bchCheck(&msg, msg.rcvByt);
        
        // Check if message is complete and valid
        if (msg.bchSum == 0) {
            // Valid message received!
        }
    }
}
*/