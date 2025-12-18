/*
 * S00060-H / MPU: SC44125P
 * Westinghouse Field Configuration Terminal
 * 
 * Part 3: Interrupt Handler (NMI) - Coherent Phase-Shift Keying Algorithm
 * Based on US Patent 4311964
 */

#include "fct_s00060h.h"

extern const int8_t freq1[10];
extern const int8_t freq2[9];

void nmi_interrupt(FCT_State *state)
{
    int8_t temp_a, temp_b;
    int i;
    
    /* Not actively listening? Cleanup and exit */
    if (state->xmitFlag == 0) {
        state->msgStart = 0;
        state->countBit = 5;
        state->mSum_peak = 0;
        state->mSum_last = 0;
        for (i = 0; i < 8; i++) {
            state->mTable1[i % 4] = 0;
            state->mTable2[i % 4] = 0;
        }
        return;
    }
    
    /* Gather next bit and advance receive counter */
    uint8_t porta = hw_read_byte(PIA_PORT_A);
    state->byteNow = (state->byteNow << 1) | (porta & 0x01);
    
    state->countPulse--;
    if ((state->countPulse & 0x07) != 0) return;
    
    /* 8 bits received - transfer completed byte */
    state->byteLast = state->byteNow;
    
    /* Set PTM channel 3 output based on xmitStatus */
    hw_write_byte(PTM_CTL1, 0x52);
    hw_write_byte(PTM_CTL0, (state->xmitStatus > 0) ? 0x92 : 0x12);
    hw_write_byte(PTM_CTL1, 0x53);
    
    /* Advance phase interval counter */
    state->countPhase--;
    if ((state->countPhase & 0x03) != 0) goto process_signal;
    
    state->xmitStatus--;
    
    if (state->xmitStatus == 0) {
        /* === TRANSMITTER === */
        state->xmitStatus++;
        state->msgBits--;
        
        if (state->msgBits == 0) {
            state->xmitByte--;
            if (state->xmitByte == 0) {
                porta = hw_read_byte(PIA_PORT_A);
                hw_write_byte(PIA_PORT_A, porta & 0xF9);
                state->xmitStatus--;
                state->bitCount++;
                state->msgStart = 0;
                state->countBit = 5;
                return;
            }
            state->msgBits = 0x34;
            state->bchSum = 0;
        }
        
        if (state->uutType == UUT_LMT1XX && state->msgBits == 0x1E) {
            state->bchSum = 0;
        }
        
        if (state->msgBits == 6) {
            state->msgTbl[0] = (state->msgTbl[0] & 0x3F) | (state->bchSum << 2);
        }
        
        /* Rotate message bits out */
        uint8_t bit_out = 0;
        for (i = state->msgLen; i > 0; i--) {
            uint8_t carry = (state->msgTbl[i-1] & 0x80) ? 1 : 0;
            state->msgTbl[i-1] = (state->msgTbl[i-1] << 1) | bit_out;
            bit_out = carry;
        }
        
        bch_update(state, bit_out);
        
        porta = hw_read_byte(PIA_PORT_A);
        hw_write_byte(PIA_PORT_A, (porta & 0xF9) | ((bit_out << 1) | 0x04));
        return;
    }
    
    if (state->xmitStatus < 0) {
        /* === RECEIVER === */
        state->xmitStatus++;
        state->countBit--;
        
        if (state->countBit == 0) {
            state->countBit++;
            temp_b = state->mSum_last ^ state->mSum_Old;
            if (temp_b >= 0) {
                state->headerBit = state->mSum_last;
                state->countBit--;
            }
        } else if (state->countBit < 0) {
            state->countBit++;
            temp_b = state->mSum_last;
            if (state->headerBit >= 0) temp_b = ~temp_b;
            state->msgStart = -1;
        } else {
            temp_b = 0;
            if (state->countBit == 1) state->mSum_peak = -1;
        }
        
        porta = hw_read_byte(PIA_PORT_A);
        hw_write_byte(PIA_PORT_A, (porta & 0xF7) | ((temp_b >> 4) & 0x08));
        
        state->mSum_Old = state->mSum_last;
        temp_a = state->mSum_last;
        if (state->headerBit >= 0) temp_a = ~temp_a;
        
        if (state->msgStart >= 0) {
            state->bitCount = 0;
            state->msgEnd = state->xmitByte;
            state->bchSum = 0;
            state->msgBits = 0;
            goto process_signal;
        }
        
        if (state->bitCount != 0) goto process_signal;
        
        /* Rotate received bit into message buffer */
        uint8_t bit_in = (temp_a < 0) ? 1 : 0;
        for (i = state->msgLen; i > 0; i--) {
            uint8_t carry = (state->msgTbl[i-1] & 0x80) ? 1 : 0;
            state->msgTbl[i-1] = (state->msgTbl[i-1] << 1) | bit_in;
            bit_in = carry;
        }
        
        state->msgBits++;
        bch_update(state, state->msgTbl[state->msgLen - 1]);
        
        if (state->msgBits >= 0x34) {
            state->msgEnd--;
            if (state->msgEnd == 0) state->bitCount = 1;
        }
    } else {
        return;
    }
    
process_signal:
    /* === CPSK SIGNAL PROCESSING === */
    state->valueIi = 0;
    state->valueQi = 0;
    state->valueI2i = 0;
    state->valueQ2i = 0;
    
    temp_a = state->byteOld;
    state->temp1 = state->byteLast;
    
    /* Process 8 bits through lookup tables */
    for (i = 7; i >= 0; i--) {
        uint8_t current_bit = (state->temp1 >> i) & 0x01;
        uint8_t prev_bit = (i == 7) ? (temp_a & 0x01) : ((state->temp1 >> (i+1)) & 0x01);
        
        if (current_bit != prev_bit) {
            int8_t freq_val = freq1[7 - i];
            if (prev_bit) freq_val = -freq_val;
            state->valueIi += freq_val;
            
            freq_val = freq1[7 - i + 2];
            if (prev_bit) freq_val = -freq_val;
            state->valueQi += freq_val;
            
            state->valueI2i += freq2[7 - i];
            state->valueQ2i += freq2[7 - i + 1];
        }
        temp_a = current_bit;
    }
    
    state->byteOld = state->byteLast;
    
    /* Calculate rolling averages (15/16 of previous + new) */
    int16_t avg = ((int16_t)state->avgI2H << 8) | state->avgI2L;
    avg = avg - (avg >> 4) + state->valueI2i;
    state->avgI2H = (avg >> 8);
    state->avgI2L = avg & 0xFF;
    
    avg = ((int16_t)state->avgQ2H << 8) | state->avgQ2L;
    avg = avg - (avg >> 4) + state->valueQ2i;
    state->avgQ2H = (avg >> 8);
    state->avgQ2L = avg & 0xFF;
    
    /* Calculate sgnFlag polarity */
    if (state->avgI2H < 0) {
        if ((state->avgQ2H ^ state->avgQ20) < 0) {
            state->sgnFlag = ~state->sgnFlag;
        }
    }
    state->avgQ20 = state->avgQ2H;
    
    /* Calculate signal level */
    temp_a = (state->avgI2H < 0) ? -state->avgI2H : state->avgI2H;
    temp_b = (state->avgQ2H < 0) ? -state->avgQ2H : state->avgQ2H;
    
    if (temp_a > temp_b) {
        state->sigLevel = temp_a + (temp_b >> 1);
    } else {
        state->sigLevel = temp_b + (temp_a >> 1);
    }
    
    /* Threshold check */
    if (state->sigLevel < 9) {
        state->vectorSum = 0;
        state->msgStart = 0;
        state->countBit = 5;
        state->mSum_peak = 0;
        state->mSum_last = 0;
        for (i = 0; i < 8; i++) {
            state->mTable1[i % 4] = 0;
            state->mTable2[i % 4] = 0;
        }
        return;
    }
    
    /* Vector angle calculation */
    temp_a = state->avgI2H;
    temp_b = state->avgQ2H;
    
    if (temp_b >= temp_a) {
        temp_b = -temp_b;
        if (temp_b < temp_a) {
            state->vectorI = temp_a;
            state->vectorQ = (-temp_b) >> 1;
        } else {
            temp_a = temp_a >> 1;
            state->vectorI = temp_a + temp_b;
            state->vectorQ = temp_a - temp_b;
        }
    } else {
        temp_a = -temp_a;
        if (temp_b < temp_a) {
            if (temp_b >= 0) {
                state->vectorQ = temp_a;
                state->vectorI = temp_b >> 1;
            } else {
                temp_a = -temp_a;
                temp_b = -temp_b;
                state->vectorQ = temp_a;
                state->vectorI = temp_b >> 1;
            }
        } else {
            temp_a = temp_a >> 1;
            state->vectorI = temp_a + temp_b;
            state->vectorQ = temp_a - temp_b;
        }
    }
    
    /* Phase detector */
    int8_t sign1 = (state->valueIi ^ state->vectorI) < 0 ? -1 : 1;
    int8_t abs_vi = (state->valueIi < 0) ? -state->valueIi : state->valueIi;
    int8_t abs_veci = (state->vectorI < 0) ? -state->vectorI : state->vectorI;
    int16_t product = ((int16_t)abs_vi * abs_veci) << 2;
    state->vectorSum = (sign1 < 0) ? -(product >> 8) : (product >> 8);
    
    int8_t sign2 = (state->valueQi ^ state->vectorQ) < 0 ? -1 : 1;
    int8_t abs_vq = (state->valueQi < 0) ? -state->valueQi : state->valueQi;
    int8_t abs_vecq = (state->vectorQ < 0) ? -state->vectorQ : state->vectorQ;
    product = ((int16_t)abs_vq * abs_vecq) << 2;
    int8_t q_component = (sign2 < 0) ? -(product >> 8) : (product >> 8);
    
    state->vectorSum += q_component;
    if (state->sgnFlag < 0) state->vectorSum = ~state->vectorSum;
    
    state->mValue_raw = state->vectorSum >> 1;
    
    if (state->countBit != 0) {
        if ((state->mValue_raw ^ state->vectorSum) < 0) {
            state->sgnFlag = ~state->sgnFlag;
        }
    }
    
    /* M-Store / M-Sum table operations */
    state->mTable1[state->M_index] = state->mValue_raw >> 1;
    
    int8_t msum = 0;
    for (i = 0; i < 4; i++) msum += state->mTable1[i];
    state->mSum_cur = msum;
    
    int8_t abs_msum = (msum < 0) ? -msum : msum;
    state->mTable2[state->M_index] += abs_msum >> 1;
    
    state->M_index = (state->M_index == 0) ? 3 : state->M_index - 1;
    state->countSum++;
    
    if (state->mSum_peak >= 0 && abs_msum >= state->mSum_peak) {
        state->mSum_peak = abs_msum;
        state->countSum = 0;
    }
    
    if ((state->countSum & 0x03) == 0) {
        state->mSum_last = state->mSum_cur;
    }
}
