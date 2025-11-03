#include "m68k_cpu.h"
#include <string.h>
#include <stdio.h>

// Complete 68000 instruction decoder
void m68k_decode_instruction(M68K_CPU* cpu, uint16_t opcode, OpCode* op, uint8_t* params) {
    *op = OP_ILLEGAL;
    memset(params, 0, 8);
    
    // Extract common fields
    uint8_t hi4 = (opcode >> 12) & 0xF;
    uint8_t reg_dst = (opcode >> 9) & 0x7;
    uint8_t mode_dst = (opcode >> 6) & 0x7;
    uint8_t mode_src = (opcode >> 3) & 0x7;
    uint8_t reg_src = opcode & 0x7;
    
    // Line 0: Bit manipulation, MOVEP, Immediate
    if (hi4 == 0x0) {
        if ((opcode & 0xF1C0) == 0x0100) {
            // BTST, BCHG, BCLR, BSET (register)
            uint8_t type = (opcode >> 6) & 0x3;
            *op = (type == 0) ? OP_BTST : (type == 1) ? OP_BCHG : 
                  (type == 2) ? OP_BCLR : OP_BSET;
        } else if ((opcode & 0xFFC0) == 0x0800) {
            // BTST immediate
            *op = OP_BTST;
        } else if ((opcode & 0xFF00) == 0x0000) {
            // ORI, ANDI, SUBI, ADDI, EORI, CMPI
            uint8_t operation = (opcode >> 9) & 0x7;
            switch (operation) {
                case 0: *op = OP_ORI; break;
                case 1: *op = OP_ANDI; break;
                case 2: *op = OP_SUBI; break;
                case 3: *op = OP_ADDI; break;
                case 5: *op = OP_EORI; break;
                case 6: *op = OP_CMPI; break;
            }
        } else if ((opcode & 0xF138) == 0x0108) {
            *op = OP_MOVEP;
        }
        return;
    }
    
    // Line 1, 2, 3: MOVE
    if (hi4 == 0x1 || hi4 == 0x2 || hi4 == 0x3) {
        if ((opcode & 0x01C0) == 0x0040) {
            *op = OP_MOVEA;
        } else {
            *op = OP_MOVE;
        }
        params[0] = hi4; // size: 1=byte, 3=word, 2=long
        return;
    }
    
    // Line 4: Miscellaneous
    if (hi4 == 0x4) {
        if ((opcode & 0xFFC0) == 0x4E40) {
            *op = OP_TRAP;
            params[0] = opcode & 0xF;
        } else if (opcode == 0x4E70) {
            *op = OP_RESET;
        } else if (opcode == 0x4E71) {
            *op = OP_NOP;
        } else if (opcode == 0x4E72) {
            *op = OP_STOP;
        } else if (opcode == 0x4E73) {
            *op = OP_RTE;
        } else if (opcode == 0x4E75) {
            *op = OP_RTS;
        } else if (opcode == 0x4E76) {
            *op = OP_TRAPV;
        } else if (opcode == 0x4E77) {
            *op = OP_RTR;
        } else if ((opcode & 0xFFF0) == 0x4E50) {
            *op = OP_LINK;
        } else if ((opcode & 0xFFF8) == 0x4E58) {
            *op = OP_UNLK;
        } else if ((opcode & 0xFFC0) == 0x4E80) {
            *op = OP_JSR;
        } else if ((opcode & 0xFFC0) == 0x4EC0) {
            *op = OP_JMP;
        } else if ((opcode & 0xFF00) == 0x4000) {
            // NEGX, CLR, NEG, NOT
            uint8_t type = (opcode >> 9) & 0x7;
            switch (type) {
                case 0: *op = OP_NEGX; break;
                case 2: *op = OP_CLR; break;
                case 4: *op = OP_NEG; break;
                case 6: *op = OP_NOT; break;
            }
        } else if ((opcode & 0xFB80) == 0x4880) {
            *op = OP_EXT;
        } else if ((opcode & 0xFFC0) == 0x4AC0) {
            *op = OP_TAS;
        } else if ((opcode & 0xFF00) == 0x4A00) {
            *op = OP_TST;
        } else if ((opcode & 0xFFC0) == 0x4800) {
            *op = OP_NBCD;
        } else if ((opcode & 0xFFF8) == 0x4840) {
            *op = OP_SWAP;
        } else if ((opcode & 0xFFF8) == 0x4848) {
            *op = OP_BKPT;
        } else if ((opcode & 0xFFC0) == 0x4840) {
            *op = OP_PEA;
        } else if ((opcode & 0xFB80) == 0x4880) {
            *op = OP_MOVEM;
        } else if ((opcode & 0xFFC0) == 0x44C0) {
            *op = OP_MOVE_CCR;
        } else if ((opcode & 0xFFC0) == 0x46C0) {
            *op = OP_MOVE_SR;
        } else if ((opcode & 0xFF00) == 0x4200) {
            *op = OP_CLR;
        } else if ((opcode & 0xF1C0) == 0x41C0) {
            *op = OP_LEA;
        } else if ((opcode & 0xF1C0) == 0x4180) {
            *op = OP_CHK;
        }
        return;
    }
    
    // Line 5: ADDQ, SUBQ, Scc, DBcc
    if (hi4 == 0x5) {
        if ((opcode & 0xF0C0) == 0x50C0) {
            if ((opcode & 0x38) == 0x08) {
                *op = OP_DBCC + ((opcode >> 8) & 0xF);
                params[0] = (opcode >> 8) & 0xF; // condition
            } else {
                *op = OP_SCC + ((opcode >> 8) & 0xF);
                params[0] = (opcode >> 8) & 0xF;
            }
        } else {
            *op = ((opcode >> 8) & 1) ? OP_SUBQ : OP_ADDQ;
            params[0] = reg_dst ? reg_dst : 8; // Quick data
        }
        return;
    }
    
    // Line 6: Bcc, BSR, BRA
    if (hi4 == 0x6) {
        uint8_t cond = (opcode >> 8) & 0xF;
        if (cond == 0x0) {
            *op = OP_BRA;
        } else if (cond == 0x1) {
            *op = OP_BSR;
        } else {
            *op = OP_BCC + (cond - 2);
        }
        params[0] = opcode & 0xFF; // displacement
        return;
    }
    
    // Line 7: MOVEQ
    if (hi4 == 0x7) {
        if ((opcode & 0x0100) == 0) {
            *op = OP_MOVEQ;
            params[0] = opcode & 0xFF;
            params[1] = reg_dst;
        }
        return;
    }
    
    // Line 8: OR, DIV, SBCD
    if (hi4 == 0x8) {
        if ((opcode & 0x01F0) == 0x0100) {
            *op = OP_SBCD;
        } else if ((opcode & 0x01C0) == 0x01C0) {
            *op = ((opcode >> 8) & 1) ? OP_DIVS : OP_DIVU;
        } else {
            *op = OP_OR;
        }
        return;
    }
    
    // Line 9: SUB, SUBX
    if (hi4 == 0x9) {
        if ((opcode & 0xF130) == 0x9100) {
            *op = OP_SUBX;
        } else if ((opcode & 0x00C0) == 0x00C0) {
            *op = OP_SUBA;
        } else {
            *op = OP_SUB;
        }
        return;
    }
    
    // Line A: Unassigned (Line-A emulator trap)
    if (hi4 == 0xA) {
        *op = OP_ILLEGAL;
        return;
    }
    
    // Line B: CMP, EOR
    if (hi4 == 0xB) {
        if ((opcode & 0x0100) == 0x0100) {
            if ((opcode & 0x0038) == 0x0008) {
                *op = OP_CMPM;
            } else {
                *op = OP_EOR;
            }
        } else {
            if ((opcode & 0x00C0) == 0x00C0) {
                *op = OP_CMPA;
            } else {
                *op = OP_CMP;
            }
        }
        return;
    }
    
    // Line C: AND, MUL, ABCD, EXG
    if (hi4 == 0xC) {
        if ((opcode & 0x01F0) == 0x0100) {
            *op = OP_ABCD;
        } else if ((opcode & 0xF130) == 0xC100) {
            *op = OP_EXG;
        } else if ((opcode & 0x01C0) == 0x01C0) {
            *op = ((opcode >> 8) & 1) ? OP_MULS : OP_MULU;
        } else {
            *op = OP_AND;
        }
        return;
    }
    
    // Line D: ADD, ADDX
    if (hi4 == 0xD) {
        if ((opcode & 0xF130) == 0xD100) {
            *op = OP_ADDX;
        } else if ((opcode & 0x00C0) == 0x00C0) {
            *op = OP_ADDA;
        } else {
            *op = OP_ADD;
        }
        return;
    }
    
    // Line E: Shift/Rotate
    if (hi4 == 0xE) {
        if ((opcode & 0x0018) == 0x0000) {
            // Register shifts
            uint8_t type = (opcode >> 3) & 0x3;
            uint8_t dir = (opcode >> 8) & 0x1;
            switch (type) {
                case 0: *op = dir ? OP_ASR : OP_ASL; break;
                case 1: *op = dir ? OP_LSR : OP_LSL; break;
                case 2: *op = dir ? OP_ROXR : OP_ROXL; break;
                case 3: *op = dir ? OP_ROR : OP_ROL; break;
            }
        }
        return;
    }
    
    // Line F: Unassigned (Line-F emulator trap / FPU)
    if (hi4 == 0xF) {
        // Could be FPU instructions
        *op = OP_ILLEGAL;
        return;
    }
}
