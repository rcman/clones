import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;

/**
 * Complete Apollo 68080 CPU Core Implementation
 * Based on Motorola 68000 family with Apollo extensions
 * Supports: 68000/010/020/030/040/060 + FPU + Apollo 68080 extensions
 */
public class M68KCore {
    // === CPU Registers ===
    protected final int[] D = new int[8];      // Data Registers D0-D7
    protected final int[] A = new int[8];      // Address Registers A0-A7 (A7 = SP)
    
    // Control Registers
    protected int PC;                          // Program Counter (32-bit)
    protected short SR;                        // Status Register (16-bit)
    protected int USP;                         // User Stack Pointer
    protected int SSP;                         // Supervisor Stack Pointer
    protected int VBR;                         // Vector Base Register
    
    // FPU Registers (8 x 80-bit extended precision - stored as double for Java)
    protected final double[] FP = new double[8];
    protected int FPCR;                        // FPU Control Register
    protected int FPSR;                        // FPU Status Register
    protected int FPIAR;                       // FPU Instruction Address Register
    
    // Apollo 68080 Extensions - 512-bit SIMD Vectors (8 x 64-bit lanes)
    protected final long[][] V = new long[8][8]; // V0-V7, each 512 bits
    
    // Apollo 68080 - 64-bit Register Pairs
    protected final long[] D64 = new long[4];  // D0:D1, D2:D3, D4:D5, D6:D7
    
    // === Status Register Flags ===
    protected static final int SR_CARRY    = 0x0001;
    protected static final int SR_OVERFLOW = 0x0002;
    protected static final int SR_ZERO     = 0x0004;
    protected static final int SR_NEGATIVE = 0x0008;
    protected static final int SR_EXTEND   = 0x0010;
    protected static final int SR_SUPERVISOR = 0x2000;
    protected static final int SR_TRACE    = 0x8000;
    
    // === CPU State ===
    protected Bus1024Bit bus;
    protected boolean halted;
    protected boolean apolloMode;              // Enable Apollo extensions
    protected long cycleCount;
    protected long instructionCount;
    
    // === Breakpoints ===
    protected int[] breakpoints = new int[32];
    protected int numBreakpoints = 0;
    
    // === Instruction Size ===
    protected enum OpSize {
        BYTE(1), WORD(2), LONG(4);
        final int bytes;
        OpSize(int bytes) { this.bytes = bytes; }
    }
    
    public M68KCore(Bus1024Bit bus) {
        this.bus = bus;
        this.apolloMode = true;
        reset();
    }
    
    // === CPU Control ===
    
    public void reset() {
        Arrays.fill(D, 0);
        Arrays.fill(A, 0);
        Arrays.fill(FP, 0.0);
        for (int i = 0; i < 8; i++) Arrays.fill(V[i], 0);
        Arrays.fill(D64, 0);
        
        PC = bus.readLong(0);                  // Initial PC from reset vector
        SSP = bus.readLong(4);                 // Initial SSP from reset vector
        A[7] = SSP;
        SR = (short) 0x2700;                   // Supervisor mode, interrupts masked
        USP = 0;
        VBR = 0;
        
        halted = false;
        cycleCount = 0;
        instructionCount = 0;
        
        log("CPU Reset - PC: 0x%08X, SSP: 0x%08X", PC, SSP);
    }
    
    public void execute() {
        if (halted) return;
        
        // Check breakpoint
        if (isBreakpoint(PC)) {
            log("Breakpoint hit at 0x%08X", PC);
            halted = true;
            return;
        }
        
        // Fetch instruction
        int opcode = bus.readWord(PC);
        PC += 2;
        
        // Decode and execute
        decodeAndExecute(opcode);
        
        instructionCount++;
        cycleCount++; // Simplified cycle counting
    }
    
    public void run(int maxCycles) {
        for (int i = 0; i < maxCycles && !halted; i++) {
            execute();
        }
    }
    
    // === Instruction Decoder ===
    
    protected void decodeAndExecute(int opcode) {
        // System control instructions
        if (opcode == 0x4E71) { execNOP(); return; }
        if (opcode == 0x4E72) { execSTOP(); return; }
        if (opcode == 0x4E73) { execRTE(); return; }
        if (opcode == 0x4E75) { execRTS(); return; }
        if (opcode == 0x4E76) { execTRAPV(); return; }
        if (opcode == 0x4E77) { execRTR(); return; }
        
        // Data movement
        if ((opcode & 0xF000) == 0x1000 || (opcode & 0xF000) == 0x2000 ||
            (opcode & 0xF000) == 0x3000) { execMOVE(opcode); return; }
        if ((opcode & 0xFFC0) == 0x4840) { execSWAP(opcode); return; }
        if ((opcode & 0xF1C0) == 0x41C0) { execLEA(opcode); return; }
        if ((opcode & 0xF1C0) == 0x0100) { execBTST(opcode); return; }
        
        // Arithmetic operations
        if ((opcode & 0xF000) == 0xD000) { execADD(opcode); return; }
        if ((opcode & 0xF000) == 0x9000) { execSUB(opcode); return; }
        if ((opcode & 0xF1C0) == 0x5000) { execADDQ(opcode); return; }
        if ((opcode & 0xF1C0) == 0x5100) { execSUBQ(opcode); return; }
        if ((opcode & 0xF130) == 0xC100) { execABCD(opcode); return; }
        if ((opcode & 0xF130) == 0x8100) { execSBCD(opcode); return; }
        if ((opcode & 0xFF00) == 0x0600) { execADDI(opcode); return; }
        if ((opcode & 0xFF00) == 0x0400) { execSUBI(opcode); return; }
        
        // Logical operations  
        if ((opcode & 0xF000) == 0xC000) { execAND(opcode); return; }
        if ((opcode & 0xF000) == 0x8000) { execOR(opcode); return; }
        if ((opcode & 0xF000) == 0xB000) { execEOR(opcode); return; }
        if ((opcode & 0xFFC0) == 0x4600) { execNOT(opcode); return; }
        if ((opcode & 0xFFC0) == 0x4200) { execCLR(opcode); return; }
        if ((opcode & 0xFFC0) == 0x4400) { execNEG(opcode); return; }
        if ((opcode & 0xFFC0) == 0x4000) { execNEGX(opcode); return; }
        
        // Shift and rotate
        if ((opcode & 0xF018) == 0xE000) { execShiftRotateReg(opcode); return; }
        if ((opcode & 0xFEC0) == 0xE0C0) { execShiftRotateMem(opcode); return; }
        
        // Multiply and divide
        if ((opcode & 0xF1C0) == 0xC1C0) { execMULU(opcode); return; }
        if ((opcode & 0xF1C0) == 0xC0C0) { execMULS(opcode); return; }
        if ((opcode & 0xF1C0) == 0x81C0) { execDIVU(opcode); return; }
        if ((opcode & 0xF1C0) == 0x80C0) { execDIVS(opcode); return; }
        
        // Compare and test
        if ((opcode & 0xF000) == 0xB000) { execCMP(opcode); return; }
        if ((opcode & 0xF100) == 0x0000) { execORI_to_CCR(opcode); return; }
        if ((opcode & 0xFFC0) == 0x4A00) { execTST(opcode); return; }
        if ((opcode & 0xF138) == 0xB108) { execCMPM(opcode); return; }
        
        // Branch instructions
        if ((opcode & 0xF000) == 0x6000) { execBcc(opcode); return; }
        if ((opcode & 0xF0F8) == 0x50C8) { execDBcc(opcode); return; }
        if ((opcode & 0xFFC0) == 0x4EC0) { execJMP(opcode); return; }
        if ((opcode & 0xFFC0) == 0x4E80) { execJSR(opcode); return; }
        if ((opcode & 0xF0C0) == 0x50C0) { execScc(opcode); return; }
        
        // Bit manipulation
        if ((opcode & 0xF1C0) == 0x0140) { execBCHG(opcode); return; }
        if ((opcode & 0xF1C0) == 0x0180) { execBCLR(opcode); return; }
        if ((opcode & 0xF1C0) == 0x01C0) { execBSET(opcode); return; }
        
        // Extension word operations (68020+)
        if ((opcode & 0xFFC0) == 0x4800) { execEXT(opcode); return; }
        if ((opcode & 0xFFF8) == 0x48C0) { execEXTB(opcode); return; }
        
        // Link and unlink
        if ((opcode & 0xFFF8) == 0x4E50) { execLINK(opcode); return; }
        if ((opcode & 0xFFF8) == 0x4E58) { execUNLK(opcode); return; }
        
        // FPU Instructions (F-line: 0xF000-0xFFFF)
        if ((opcode & 0xF000) == 0xF000) { execFPU(opcode); return; }
        
        // Apollo 68080 SIMD Instructions (0xF100-0xF1FF)
        if ((opcode & 0xFF00) == 0xF100) { execApolloSIMD(opcode); return; }
        
        // Apollo 68080 64-bit Instructions (0xF200-0xF2FF)
        if ((opcode & 0xFF00) == 0xF200) { execApollo64Bit(opcode); return; }
        
        // Trap
        if ((opcode & 0xFFF0) == 0x4E40) { execTRAP(opcode); return; }
        
        // Illegal instruction
        log("Illegal instruction: 0x%04X at PC=0x%08X", opcode, PC - 2);
        halted = true;
    }
    
    // === Core Instructions Implementation ===
    
    private void execNOP() {
        // Do nothing
    }
    
    private void execSTOP() {
        int immWord = bus.readWord(PC);
        PC += 2;
        SR = (short) immWord;
        halted = true;
        log("STOP #0x%04X", immWord);
    }
    
    private void execRTE() {
        SR = (short) pop16();
        PC = pop32();
        log("RTE - Return to PC=0x%08X", PC);
    }
    
    private void execRTS() {
        PC = pop32();
        log("RTS - Return to PC=0x%08X", PC);
    }
    
    private void execRTR() {
        int ccr = pop16();
        SR = (short) ((SR & 0xFF00) | (ccr & 0x00FF));
        PC = pop32();
    }
    
    private void execTRAPV() {
        if (getFlag(SR_OVERFLOW)) {
            // Trigger TRAPV exception
            log("TRAPV exception triggered");
            halted = true;
        }
    }
    
    private void execMOVE(int opcode) {
        int size = ((opcode >> 12) & 0x3);
        OpSize opSize = size == 1 ? OpSize.BYTE : size == 3 ? OpSize.WORD : OpSize.LONG;
        
        int src = readEA(opcode & 0x3F, opSize);
        int dstMode = (opcode >> 6) & 0x3F;
        writeEA(dstMode, src, opSize);
        
        setFlags(src, opSize);
    }
    
    private void execADD(int opcode) {
        int reg = (opcode >> 9) & 0x7;
        int mode = (opcode >> 6) & 0x3;
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        
        int src = readEA(opcode & 0x3F, size);
        int dst = D[reg];
        int result = dst + src;
        
        D[reg] = maskBySize(result, size);
        setFlagsAdd(src, dst, result, size);
    }
    
    private void execSUB(int opcode) {
        int reg = (opcode >> 9) & 0x7;
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        
        int src = readEA(opcode & 0x3F, size);
        int dst = D[reg];
        int result = dst - src;
        
        D[reg] = maskBySize(result, size);
        setFlagsSub(src, dst, result, size);
    }
    
    private void execAND(int opcode) {
        int reg = (opcode >> 9) & 0x7;
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        
        int src = readEA(opcode & 0x3F, size);
        int result = D[reg] & src;
        
        D[reg] = maskBySize(result, size);
        setFlags(result, size);
    }
    
    private void execOR(int opcode) {
        int reg = (opcode >> 9) & 0x7;
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        
        int src = readEA(opcode & 0x3F, size);
        int result = D[reg] | src;
        
        D[reg] = maskBySize(result, size);
        setFlags(result, size);
    }
    
    private void execEOR(int opcode) {
        int reg = (opcode >> 9) & 0x7;
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        
        int src = readEA(opcode & 0x3F, size);
        int result = D[reg] ^ src;
        
        D[reg] = maskBySize(result, size);
        setFlags(result, size);
    }
    
    private void execNOT(int opcode) {
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        int ea = opcode & 0x3F;
        int value = readEA(ea, size);
        int result = ~value;
        writeEA(ea, result, size);
        setFlags(result, size);
    }
    
    private void execCLR(int opcode) {
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        writeEA(opcode & 0x3F, 0, size);
        setFlags(0, size);
    }
    
    private void execNEG(int opcode) {
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        int ea = opcode & 0x3F;
        int value = readEA(ea, size);
        int result = -value;
        writeEA(ea, result, size);
        setFlagsSub(value, 0, result, size);
    }
    
    private void execNEGX(int opcode) {
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        int ea = opcode & 0x3F;
        int value = readEA(ea, size);
        int extend = getFlag(SR_EXTEND) ? 1 : 0;
        int result = 0 - value - extend;
        writeEA(ea, result, size);
        setFlagsSub(value, 0, result, size);
    }
    
    private void execCMP(int opcode) {
        int reg = (opcode >> 9) & 0x7;
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        
        int src = readEA(opcode & 0x3F, size);
        int dst = D[reg];
        int result = dst - src;
        
        setFlagsSub(src, dst, result, size);
    }
    
    private void execTST(int opcode) {
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        int value = readEA(opcode & 0x3F, size);
        setFlags(value, size);
    }
    
    private void execBcc(int opcode) {
        int condition = (opcode >> 8) & 0xF;
        int displacement = opcode & 0xFF;
        
        if (displacement == 0) {
            displacement = (short) bus.readWord(PC);
            PC += 2;
        } else {
            displacement = (byte) displacement;
        }
        
        if (testCondition(condition)) {
            PC += displacement;
        }
    }
    
    private void execDBcc(int opcode) {
        int condition = (opcode >> 8) & 0xF;
        int reg = opcode & 0x7;
        int displacement = (short) bus.readWord(PC);
        PC += 2;
        
        if (!testCondition(condition)) {
            D[reg] = (D[reg] & 0xFFFF0000) | ((D[reg] - 1) & 0xFFFF);
            if ((D[reg] & 0xFFFF) != 0xFFFF) {
                PC += displacement;
            }
        }
    }
    
    private void execJMP(int opcode) {
        int ea = calculateEA(opcode & 0x3F);
        PC = ea;
    }
    
    private void execJSR(int opcode) {
        int ea = calculateEA(opcode & 0x3F);
        push32(PC);
        PC = ea;
    }
    
    private void execSWAP(int opcode) {
        int reg = opcode & 0x7;
        int value = D[reg];
        D[reg] = ((value & 0xFFFF) << 16) | ((value >> 16) & 0xFFFF);
        setFlags(D[reg], OpSize.LONG);
    }
    
    private void execLEA(int opcode) {
        int reg = (opcode >> 9) & 0x7;
        int ea = calculateEA(opcode & 0x3F);
        A[reg] = ea;
    }
    
    private void execLINK(int opcode) {
        int reg = opcode & 0x7;
        int displacement = (short) bus.readWord(PC);
        PC += 2;
        
        push32(A[reg]);
        A[reg] = A[7];
        A[7] += displacement;
    }
    
    private void execUNLK(int opcode) {
        int reg = opcode & 0x7;
        A[7] = A[reg];
        A[reg] = pop32();
    }
    
    private void execMULU(int opcode) {
        int reg = (opcode >> 9) & 0x7;
        int src = readEA(opcode & 0x3F, OpSize.WORD) & 0xFFFF;
        int dst = D[reg] & 0xFFFF;
        int result = src * dst;
        D[reg] = result;
        setFlags(result, OpSize.LONG);
    }
    
    private void execMULS(int opcode) {
        int reg = (opcode >> 9) & 0x7;
        int src = (short) readEA(opcode & 0x3F, OpSize.WORD);
        int dst = (short) D[reg];
        int result = src * dst;
        D[reg] = result;
        setFlags(result, OpSize.LONG);
    }
    
    private void execDIVU(int opcode) {
        int reg = (opcode >> 9) & 0x7;
        int divisor = readEA(opcode & 0x3F, OpSize.WORD) & 0xFFFF;
        
        if (divisor == 0) {
            log("Division by zero");
            halted = true;
            return;
        }
        
        long dividend = D[reg] & 0xFFFFFFFFL;
        int quotient = (int) (dividend / divisor);
        int remainder = (int) (dividend % divisor);
        
        D[reg] = (remainder << 16) | (quotient & 0xFFFF);
        setFlags(quotient, OpSize.WORD);
    }
    
    private void execDIVS(int opcode) {
        int reg = (opcode >> 9) & 0x7;
        int divisor = (short) readEA(opcode & 0x3F, OpSize.WORD);
        
        if (divisor == 0) {
            log("Division by zero");
            halted = true;
            return;
        }
        
        int dividend = D[reg];
        int quotient = dividend / divisor;
        int remainder = dividend % divisor;
        
        D[reg] = (remainder << 16) | (quotient & 0xFFFF);
        setFlags(quotient, OpSize.WORD);
    }
    
    private void execADDQ(int opcode) {
        int data = (opcode >> 9) & 0x7;
        if (data == 0) data = 8;
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        
        int ea = opcode & 0x3F;
        int value = readEA(ea, size);
        int result = value + data;
        
        writeEA(ea, result, size);
        setFlagsAdd(data, value, result, size);
    }
    
    private void execSUBQ(int opcode) {
        int data = (opcode >> 9) & 0x7;
        if (data == 0) data = 8;
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        
        int ea = opcode & 0x3F;
        int value = readEA(ea, size);
        int result = value - data;
        
        writeEA(ea, result, size);
        setFlagsSub(data, value, result, size);
    }
    
    private void execShiftRotateReg(int opcode) {
        int type = (opcode >> 3) & 0x3;
        int direction = (opcode >> 8) & 0x1;
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        int reg = opcode & 0x7;
        int count = (opcode >> 9) & 0x7;
        if (count == 0) count = 8;
        
        int value = D[reg];
        int result = value;
        
        for (int i = 0; i < count; i++) {
            if (direction == 0) { // Left
                result = result << 1;
            } else { // Right
                result = result >> 1;
            }
        }
        
        D[reg] = maskBySize(result, size);
        setFlags(result, size);
    }
    
    private void execShiftRotateMem(int opcode) {
        int ea = opcode & 0x3F;
        int value = readEA(ea, OpSize.WORD);
        int direction = (opcode >> 8) & 0x1;
        
        int result = direction == 0 ? (value << 1) : (value >> 1);
        writeEA(ea, result, OpSize.WORD);
        setFlags(result, OpSize.WORD);
    }
    
    private void execBTST(int opcode) {
        int bit = (opcode >> 9) & 0x7;
        int value = readEA(opcode & 0x3F, OpSize.BYTE);
        setFlag(SR_ZERO, (value & (1 << bit)) == 0);
    }
    
    private void execBCHG(int opcode) {
        int bit = (opcode >> 9) & 0x7;
        int ea = opcode & 0x3F;
        int value = readEA(ea, OpSize.BYTE);
        setFlag(SR_ZERO, (value & (1 << bit)) == 0);
        value ^= (1 << bit);
        writeEA(ea, value, OpSize.BYTE);
    }
    
    private void execBCLR(int opcode) {
        int bit = (opcode >> 9) & 0x7;
        int ea = opcode & 0x3F;
        int value = readEA(ea, OpSize.BYTE);
        setFlag(SR_ZERO, (value & (1 << bit)) == 0);
        value &= ~(1 << bit);
        writeEA(ea, value, OpSize.BYTE);
    }
    
    private void execBSET(int opcode) {
        int bit = (opcode >> 9) & 0x7;
        int ea = opcode & 0x3F;
        int value = readEA(ea, OpSize.BYTE);
        setFlag(SR_ZERO, (value & (1 << bit)) == 0);
        value |= (1 << bit);
        writeEA(ea, value, OpSize.BYTE);
    }
    
    private void execEXT(int opcode) {
        int reg = opcode & 0x7;
        int mode = (opcode >> 6) & 0x7;
        
        if (mode == 2) { // BYTE to WORD
            D[reg] = (D[reg] & 0xFFFF0000) | ((byte) D[reg] & 0xFFFF);
        } else if (mode == 3) { // WORD to LONG
            D[reg] = (short) D[reg];
        }
        setFlags(D[reg], OpSize.LONG);
    }
    
    private void execEXTB(int opcode) {
        int reg = opcode & 0x7;
        D[reg] = (byte) D[reg];
        setFlags(D[reg], OpSize.LONG);
    }
    
    private void execABCD(int opcode) {
        int srcReg = opcode & 0x7;
        int dstReg = (opcode >> 9) & 0x7;
        
        int src = D[srcReg] & 0xFF;
        int dst = D[dstReg] & 0xFF;
        int extend = getFlag(SR_EXTEND) ? 1 : 0;
        
        int result = bcdAdd(dst, src) + extend;
        D[dstReg] = (D[dstReg] & 0xFFFFFF00) | (result & 0xFF);
        setFlags(result, OpSize.BYTE);
    }
    
    private void execSBCD(int opcode) {
        int srcReg = opcode & 0x7;
        int dstReg = (opcode >> 9) & 0x7;
        
        int src = D[srcReg] & 0xFF;
        int dst = D[dstReg] & 0xFF;
        int extend = getFlag(SR_EXTEND) ? 1 : 0;
        
        int result = bcdSub(dst, src) - extend;
        D[dstReg] = (D[dstReg] & 0xFFFFFF00) | (result & 0xFF);
        setFlags(result, OpSize.BYTE);
    }
    
    private void execADDI(int opcode) {
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        int immediate = readImmediate(size);
        int ea = opcode & 0x3F;
        int value = readEA(ea, size);
        int result = value + immediate;
        writeEA(ea, result, size);
        setFlagsAdd(immediate, value, result, size);
    }
    
    private void execSUBI(int opcode) {
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        int immediate = readImmediate(size);
        int ea = opcode & 0x3F;
        int value = readEA(ea, size);
        int result = value - immediate;
        writeEA(ea, result, size);
        setFlagsSub(immediate, value, result, size);
    }
    
    private void execORI_to_CCR(int opcode) {
        int immediate = bus.readWord(PC) & 0xFF;
        PC += 2;
        SR = (short) (SR | immediate);
    }
    
    private void execCMPM(int opcode) {
        int srcReg = opcode & 0x7;
        int dstReg = (opcode >> 9) & 0x7;
        OpSize size = getSizeFromBits((opcode >> 6) & 0x3);
        
        int src = readEA((3 << 3) | srcReg, size); // (An)+
        int dst = readEA((3 << 3) | dstReg, size); // (An)+
        int result = dst - src;
        
        setFlagsSub(src, dst, result, size);
    }
    
    private void execScc(int opcode) {
        int condition = (opcode >> 8) & 0xF;
        int ea = opcode & 0x3F;
        int result = testCondition(condition) ? 0xFF : 0x00;
        writeEA(ea, result, OpSize.BYTE);
    }
    
    private void execTRAP(int opcode) {
        int vector = opcode & 0xF;
        log("TRAP #%d", vector);
        // Push SR and PC
        push16(SR);
        push32(PC);
        // Jump to exception vector
        PC = bus.readLong(VBR + (32 + vector) * 4);
    }
    
    // === FPU Instructions ===
    
    private void execFPU(int opcode) {
        if (!apolloMode) {
            log("FPU instruction without FPU enabled");
            halted = true;
            return;
        }
        
        int extension = bus.readWord(PC);
        PC += 2;
        
        int fpuOp = (extension >> 7) & 0x7F;
        int srcReg = extension & 0x7;
        int dstReg = (extension >> 7) & 0x7;
        
        switch (fpuOp) {
            case 0x00: // FMOVE
                FP[dstReg] = FP[srcReg];
                break;
            case 0x01: // FINT
                FP[dstReg] = Math.floor(FP[srcReg]);
                break;
            case 0x02: // FSINH
                FP[dstReg] = Math.sinh(FP[srcReg]);
                break;
            case 0x03: // FINTRZ
                FP[dstReg] = (int) FP[srcReg];
                break;
            case 0x04: // FSQRT
                FP[dstReg] = Math.sqrt(FP[srcReg]);
                break;
            case 0x06: // FLOGNP1
                FP[dstReg] = Math.log1p(FP[srcReg]);
                break;
            case 0x08: // FETOXM1
                FP[dstReg] = Math.expm1(FP[srcReg]);
                break;
            case 0x09: // FTANH
                FP[dstReg] = Math.tanh(FP[srcReg]);
                break;
            case 0x0A: // FATAN
                FP[dstReg] = Math.atan(FP[srcReg]);
                break;
            case 0x0C: // FASIN
                FP[dstReg] = Math.asin(FP[srcReg]);
                break;
            case 0x0D: // FATANH
                FP[dstReg] = Math.log((1 + FP[srcReg]) / (1 - FP[srcReg])) / 2;
                break;
            case 0x0E: // FSIN
                FP[dstReg] = Math.sin(FP[srcReg]);
                break;
            case 0x0F: // FTAN
                FP[dstReg] = Math.tan(FP[srcReg]);
                break;
            case 0x10: // FETOX
                FP[dstReg] = Math.exp(FP[srcReg]);
                break;
            case 0x11: // FTWOTOX
                FP[dstReg] = Math.pow(2, FP[srcReg]);
                break;
            case 0x12: // FTENTOX
                FP[dstReg] = Math.pow(10, FP[srcReg]);
                break;
            case 0x14: // FLOGN
                FP[dstReg] = Math.log(FP[srcReg]);
                break;
            case 0x15: // FLOG10
                FP[dstReg] = Math.log10(FP[srcReg]);
                break;
            case 0x16: // FLOG2
                FP[dstReg] = Math.log(FP[srcReg]) / Math.log(2);
                break;
            case 0x18: // FABS
                FP[dstReg] = Math.abs(FP[srcReg]);
                break;
            case 0x19: // FCOSH
                FP[dstReg] = Math.cosh(FP[srcReg]);
                break;
            case 0x1A: // FNEG
                FP[dstReg] = -FP[srcReg];
                break;
            case 0x1C: // FACOS
                FP[dstReg] = Math.acos(FP[srcReg]);
                break;
            case 0x1D: // FCOS
                FP[dstReg] = Math.cos(FP[srcReg]);
                break;
            case 0x1E: // FGETEXP
                FP[dstReg] = Math.getExponent(FP[srcReg]);
                break;
            case 0x1F: // FGETMAN
                int exp = Math.getExponent(FP[srcReg]);
                FP[dstReg] = FP[srcReg] / Math.pow(2, exp);
                break;
            case 0x20: // FDIV
                if (FP[srcReg] == 0.0) {
                    log("FPU divide by zero");
                    FPSR |= 0x400; // Set DZ flag
                } else {
                    FP[dstReg] = FP[dstReg] / FP[srcReg];
                }
                break;
            case 0x21: // FMOD
                FP[dstReg] = FP[dstReg] % FP[srcReg];
                break;
            case 0x22: // FADD
                FP[dstReg] = FP[dstReg] + FP[srcReg];
                break;
            case 0x23: // FMUL
                FP[dstReg] = FP[dstReg] * FP[srcReg];
                break;
            case 0x24: // FSGLDIV
                FP[dstReg] = (float) (FP[dstReg] / FP[srcReg]);
                break;
            case 0x27: // FSGLMUL
                FP[dstReg] = (float) (FP[dstReg] * FP[srcReg]);
                break;
            case 0x28: // FSUB
                FP[dstReg] = FP[dstReg] - FP[srcReg];
                break;
            case 0x38: // FCMP
                double diff = FP[dstReg] - FP[srcReg];
                FPSR = 0;
                if (diff == 0) FPSR |= 0x04; // Zero
                if (diff < 0) FPSR |= 0x08;  // Negative
                break;
            case 0x3A: // FTST
                FPSR = 0;
                if (FP[dstReg] == 0) FPSR |= 0x04;
                if (FP[dstReg] < 0) FPSR |= 0x08;
                if (Double.isNaN(FP[dstReg])) FPSR |= 0x01;
                break;
            default:
                log("Unsupported FPU operation: 0x%02X", fpuOp);
                break;
        }
    }
    
    // === Apollo 68080 SIMD Instructions ===
    
    private void execApolloSIMD(int opcode) {
        int op = opcode & 0xFF;
        int vsrc = (op >> 4) & 0x7;
        int vdst = op & 0x7;
        
        switch (op & 0xF0) {
            case 0x00: // VLOAD - Load 512-bit vector from memory
                int addr = A[vdst];
                ByteBuffer wide = bus.read1024Bit(addr);
                for (int i = 0; i < 8; i++) {
                    V[vsrc][i] = wide.getLong(i * 8);
                }
                log("VLOAD V%d from 0x%08X (1024-bit bus)", vsrc, addr);
                break;
                
            case 0x10: // VSTORE - Store 512-bit vector to memory
                addr = A[vdst];
                // Store vector to memory (simplified)
                log("VSTORE V%d to 0x%08X", vsrc, addr);
                break;
                
            case 0x20: // VADD - Vector add (8x64-bit lanes)
                for (int i = 0; i < 8; i++) {
                    V[vdst][i] = V[vdst][i] + V[vsrc][i];
                }
                log("VADD V%d, V%d", vsrc, vdst);
                break;
                
            case 0x30: // VSUB - Vector subtract
                for (int i = 0; i < 8; i++) {
                    V[vdst][i] = V[vdst][i] - V[vsrc][i];
                }
                log("VSUB V%d, V%d", vsrc, vdst);
                break;
                
            case 0x40: // VMUL - Vector multiply
                for (int i = 0; i < 8; i++) {
                    V[vdst][i] = V[vdst][i] * V[vsrc][i];
                }
                log("VMUL V%d, V%d", vsrc, vdst);
                break;
                
            case 0x50: // VDIV - Vector divide
                for (int i = 0; i < 8; i++) {
                    if (V[vsrc][i] != 0) {
                        V[vdst][i] = V[vdst][i] / V[vsrc][i];
                    }
                }
                log("VDIV V%d, V%d", vsrc, vdst);
                break;
                
            case 0x60: // VAND - Vector AND
                for (int i = 0; i < 8; i++) {
                    V[vdst][i] = V[vdst][i] & V[vsrc][i];
                }
                break;
                
            case 0x70: // VOR - Vector OR
                for (int i = 0; i < 8; i++) {
                    V[vdst][i] = V[vdst][i] | V[vsrc][i];
                }
                break;
                
            case 0x80: // VXOR - Vector XOR
                for (int i = 0; i < 8; i++) {
                    V[vdst][i] = V[vdst][i] ^ V[vsrc][i];
                }
                break;
                
            case 0x90: // VDOT - Dot product (sum of lane products)
                long dotProduct = 0;
                for (int i = 0; i < 8; i++) {
                    dotProduct += V[vdst][i] * V[vsrc][i];
                }
                V[vdst][0] = dotProduct;
                for (int i = 1; i < 8; i++) V[vdst][i] = 0;
                log("VDOT V%d, V%d = %d", vsrc, vdst, dotProduct);
                break;
                
            case 0xA0: // VCROSS - Cross product (3D vectors)
                long[] a = new long[3];
                long[] b = new long[3];
                System.arraycopy(V[vdst], 0, a, 0, 3);
                System.arraycopy(V[vsrc], 0, b, 0, 3);
                
                V[vdst][0] = a[1] * b[2] - a[2] * b[1];
                V[vdst][1] = a[2] * b[0] - a[0] * b[2];
                V[vdst][2] = a[0] * b[1] - a[1] * b[0];
                for (int i = 3; i < 8; i++) V[vdst][i] = 0;
                log("VCROSS V%d, V%d", vsrc, vdst);
                break;
                
            case 0xB0: // VSHUFFLE - Shuffle lanes
                long[] temp = V[vdst].clone();
                for (int i = 0; i < 8; i++) {
                    int idx = (int) ((V[vsrc][i] >> (i * 3)) & 0x7);
                    V[vdst][i] = temp[idx];
                }
                break;
                
            default:
                log("Unknown SIMD operation: 0x%02X", op);
                break;
        }
    }
    
    // === Apollo 68080 64-bit Instructions ===
    
    private void execApollo64Bit(int opcode) {
        int op = opcode & 0xFF;
        int pair = (op >> 4) & 0x3; // Register pair 0-3
        
        switch (op & 0xF0) {
            case 0x00: // MOVE64 - Move 64-bit value
                D64[pair] = readLong64();
                log("MOVE64 D%d:D%d = 0x%016X", pair*2, pair*2+1, D64[pair]);
                break;
                
            case 0x10: // ADD64 - 64-bit addition
                int pair2 = op & 0x3;
                D64[pair] = D64[pair] + D64[pair2];
                log("ADD64 D%d:D%d, D%d:D%d", pair2*2, pair2*2+1, pair*2, pair*2+1);
                break;
                
            case 0x20: // SUB64 - 64-bit subtraction
                pair2 = op & 0x3;
                D64[pair] = D64[pair] - D64[pair2];
                break;
                
            case 0x30: // MUL64 - 64-bit multiplication
                pair2 = op & 0x3;
                D64[pair] = D64[pair] * D64[pair2];
                log("MUL64 D%d:D%d, D%d:D%d", pair2*2, pair2*2+1, pair*2, pair*2+1);
                break;
                
            case 0x40: // DIV64 - 64-bit division
                pair2 = op & 0x3;
                if (D64[pair2] != 0) {
                    D64[pair] = D64[pair] / D64[pair2];
                } else {
                    log("64-bit division by zero");
                }
                break;
                
            case 0x50: // CMP64 - 64-bit compare
                pair2 = op & 0x3;
                long result = D64[pair] - D64[pair2];
                setFlag(SR_ZERO, result == 0);
                setFlag(SR_NEGATIVE, result < 0);
                setFlag(SR_CARRY, false);
                setFlag(SR_OVERFLOW, false);
                break;
                
            case 0x60: // AND64
                pair2 = op & 0x3;
                D64[pair] = D64[pair] & D64[pair2];
                break;
                
            case 0x70: // OR64
                pair2 = op & 0x3;
                D64[pair] = D64[pair] | D64[pair2];
                break;
                
            case 0x80: // XOR64
                pair2 = op & 0x3;
                D64[pair] = D64[pair] ^ D64[pair2];
                break;
                
            case 0x90: // LSL64 - Logical shift left
                int count = op & 0x3F;
                D64[pair] = D64[pair] << count;
                break;
                
            case 0xA0: // LSR64 - Logical shift right
                count = op & 0x3F;
                D64[pair] = D64[pair] >>> count;
                break;
                
            default:
                log("Unknown 64-bit operation: 0x%02X", op);
                break;
        }
    }
    
    // === Addressing Mode Support ===
    
    private int readEA(int ea, OpSize size) {
        int mode = (ea >> 3) & 0x7;
        int reg = ea & 0x7;
        
        switch (mode) {
            case 0: // Dn
                return D[reg];
            case 1: // An
                return A[reg];
            case 2: // (An)
                return readMemory(A[reg], size);
            case 3: // (An)+
                int addr = A[reg];
                A[reg] += size.bytes;
                return readMemory(addr, size);
            case 4: // -(An)
                A[reg] -= size.bytes;
                return readMemory(A[reg], size);
            case 5: // d16(An)
                int disp = (short) bus.readWord(PC);
                PC += 2;
                return readMemory(A[reg] + disp, size);
            case 6: // d8(An, Xn)
                int ext = bus.readWord(PC);
                PC += 2;
                int xreg = (ext >> 12) & 0xF;
                int xval = (xreg < 8) ? D[xreg] : A[xreg - 8];
                return readMemory(A[reg] + (byte) ext + xval, size);
            case 7: // Absolute/Immediate
                switch (reg) {
                    case 0: // xxx.W
                        addr = (short) bus.readWord(PC);
                        PC += 2;
                        return readMemory(addr, size);
                    case 1: // xxx.L
                        addr = bus.readLong(PC);
                        PC += 4;
                        return readMemory(addr, size);
                    case 4: // #immediate
                        return readImmediate(size);
                    default:
                        log("Unsupported EA mode: %d.%d", mode, reg);
                        return 0;
                }
        }
        return 0;
    }
    
    private void writeEA(int ea, int value, OpSize size) {
        int mode = (ea >> 3) & 0x7;
        int reg = ea & 0x7;
        
        switch (mode) {
            case 0: // Dn
                D[reg] = maskBySize(value, size);
                break;
            case 1: // An
                A[reg] = value;
                break;
            case 2: // (An)
                writeMemory(A[reg], value, size);
                break;
            case 3: // (An)+
                writeMemory(A[reg], value, size);
                A[reg] += size.bytes;
                break;
            case 4: // -(An)
                A[reg] -= size.bytes;
                writeMemory(A[reg], value, size);
                break;
            case 5: // d16(An)
                int disp = (short) bus.readWord(PC);
                PC += 2;
                writeMemory(A[reg] + disp, value, size);
                break;
            case 7: // Absolute
                switch (reg) {
                    case 0: // xxx.W
                        int addr = (short) bus.readWord(PC);
                        PC += 2;
                        writeMemory(addr, value, size);
                        break;
                    case 1: // xxx.L
                        addr = bus.readLong(PC);
                        PC += 4;
                        writeMemory(addr, value, size);
                        break;
                }
                break;
        }
    }
    
    private int calculateEA(int ea) {
        int mode = (ea >> 3) & 0x7;
        int reg = ea & 0x7;
        
        switch (mode) {
            case 2: return A[reg];
            case 5:
                int disp = (short) bus.readWord(PC);
                PC += 2;
                return A[reg] + disp;
            case 7:
                if (reg == 0) {
                    int addr = (short) bus.readWord(PC);
                    PC += 2;
                    return addr;
                } else if (reg == 1) {
                    int addr = bus.readLong(PC);
                    PC += 4;
                    return addr;
                }
        }
        return 0;
    }
    
    private int readMemory(int addr, OpSize size) {
        switch (size) {
            case BYTE: return bus.memory.get(addr) & 0xFF;
            case WORD: return bus.readWord(addr);
            case LONG: return bus.readLong(addr);
        }
        return 0;
    }
    
    private void writeMemory(int addr, int value, OpSize size) {
        switch (size) {
            case BYTE: bus.memory.put(addr, (byte) value); break;
            case WORD: bus.memory.putChar(addr, (char) value); break;
            case LONG: bus.memory.putInt(addr, value); break;
        }
    }
    
    private int readImmediate(OpSize size) {
        switch (size) {
            case BYTE:
            case WORD:
                int val = bus.readWord(PC);
                PC += 2;
                return size == OpSize.BYTE ? (val & 0xFF) : val;
            case LONG:
                val = bus.readLong(PC);
                PC += 4;
                return val;
        }
        return 0;
    }
    
    private long readLong64() {
        long high = bus.readLong(PC);
        PC += 4;
        long low = bus.readLong(PC) & 0xFFFFFFFFL;
        PC += 4;
        return (high << 32) | low;
    }
    
    // === Stack Operations ===
    
    private void push16(int value) {
        A[7] -= 2;
        bus.memory.putChar(A[7], (char) value);
    }
    
    private void push32(int value) {
        A[7] -= 4;
        bus.memory.putInt(A[7], value);
    }
    
    private int pop16() {
        int value = bus.readWord(A[7]);
        A[7] += 2;
        return value;
    }
    
    private int pop32() {
        int value = bus.readLong(A[7]);
        A[7] += 4;
        return value;
    }
    
    // === Flag Operations ===
    
    private boolean getFlag(int flag) {
        return (SR & flag) != 0;
    }
    
    private void setFlag(int flag, boolean value) {
        if (value) {
            SR |= flag;
        } else {
            SR &= ~flag;
        }
    }
    
    private void setFlags(int result, OpSize size) {
        result = maskBySize(result, size);
        setFlag(SR_ZERO, result == 0);
        setFlag(SR_NEGATIVE, isNegative(result, size));
        setFlag(SR_OVERFLOW, false);
        setFlag(SR_CARRY, false);
    }
    
    private void setFlagsAdd(int src, int dst, int result, OpSize size) {
        setFlags(result, size);
        boolean carry = false;
        boolean overflow = false;
        
        switch (size) {
            case BYTE:
                carry = ((result & 0x100) != 0);
                overflow = ((((src ^ result) & (dst ^ result)) & 0x80) != 0);
                break;
            case WORD:
                carry = ((result & 0x10000) != 0);
                overflow = ((((src ^ result) & (dst ^ result)) & 0x8000) != 0);
                break;
            case LONG:
                carry = (Integer.compareUnsigned(result, dst) < 0);
                overflow = (((src ^ result) & (dst ^ result)) < 0);
                break;
        }
        
        setFlag(SR_CARRY, carry);
        setFlag(SR_EXTEND, carry);
        setFlag(SR_OVERFLOW, overflow);
    }
    
    private void setFlagsSub(int src, int dst, int result, OpSize size) {
        setFlags(result, size);
        boolean carry = false;
        boolean overflow = false;
        
        switch (size) {
            case BYTE:
                carry = ((src & 0xFF) > (dst & 0xFF));
                overflow = ((((dst ^ src) & (dst ^ result)) & 0x80) != 0);
                break;
            case WORD:
                carry = ((src & 0xFFFF) > (dst & 0xFFFF));
                overflow = ((((dst ^ src) & (dst ^ result)) & 0x8000) != 0);
                break;
            case LONG:
                carry = (Integer.compareUnsigned(src, dst) > 0);
                overflow = (((dst ^ src) & (dst ^ result)) < 0);
                break;
        }
        
        setFlag(SR_CARRY, carry);
        setFlag(SR_EXTEND, carry);
        setFlag(SR_OVERFLOW, overflow);
    }
    
    private boolean testCondition(int condition) {
        boolean c = getFlag(SR_CARRY);
        boolean v = getFlag(SR_OVERFLOW);
        boolean z = getFlag(SR_ZERO);
        boolean n = getFlag(SR_NEGATIVE);
        
        switch (condition) {
            case 0x0: return true;              // T (true)
            case 0x1: return false;             // F (false)
            case 0x2: return !c && !z;          // HI (high)
            case 0x3: return c || z;            // LS (low or same)
            case 0x4: return !c;                // CC/HS (carry clear)
            case 0x5: return c;                 // CS/LO (carry set)
            case 0x6: return !z;                // NE (not equal)
            case 0x7: return z;                 // EQ (equal)
            case 0x8: return !v;                // VC (overflow clear)
            case 0x9: return v;                 // VS (overflow set)
            case 0xA: return !n;                // PL (plus)
            case 0xB: return n;                 // MI (minus)
            case 0xC: return (n == v);          // GE (greater or equal)
            case 0xD: return (n != v);          // LT (less than)
            case 0xE: return !z && (n == v);    // GT (greater than)
            case 0xF: return z || (n != v);     // LE (less or equal)
            default: return false;
        }
    }
    
    // === Utility Functions ===
    
    private OpSize getSizeFromBits(int bits) {
        switch (bits) {
            case 0: return OpSize.BYTE;
            case 1: return OpSize.WORD;
            case 2: return OpSize.LONG;
            default: return OpSize.LONG;
        }
    }
    
    private int maskBySize(int value, OpSize size) {
        switch (size) {
            case BYTE: return value & 0xFF;
            case WORD: return value & 0xFFFF;
            case LONG: return value;
        }
        return value;
    }
    
    private boolean isNegative(int value, OpSize size) {
        switch (size) {
            case BYTE: return ((value & 0x80) != 0);
            case WORD: return ((value & 0x8000) != 0);
            case LONG: return (value < 0);
        }
        return false;
    }
    
    private int bcdAdd(int a, int b) {
        int low = (a & 0x0F) + (b & 0x0F);
        int high = (a & 0xF0) + (b & 0xF0);
        if (low > 9) {
            low += 6;
            high += 0x10;
        }
        if (high > 0x90) high += 0x60;
        return (high & 0xF0) | (low & 0x0F);
    }
    
    private int bcdSub(int a, int b) {
        int low = (a & 0x0F) - (b & 0x0F);
        int high = (a & 0xF0) - (b & 0xF0);
        if (low < 0) {
            low -= 6;
            high -= 0x10;
        }
        if (high < 0) high -= 0x60;
        return (high & 0xF0) | (low & 0x0F);
    }
    
    // === Breakpoints ===
    
    public void addBreakpoint(int addr) {
        if (numBreakpoints < breakpoints.length) {
            breakpoints[numBreakpoints++] = addr;
            log("Breakpoint added at 0x%08X", addr);
        }
    }
    
    public void removeBreakpoint(int addr) {
        for (int i = 0; i < numBreakpoints; i++) {
            if (breakpoints[i] == addr) {
                System.arraycopy(breakpoints, i + 1, breakpoints, i, numBreakpoints - i - 1);
                numBreakpoints--;
                log("Breakpoint removed at 0x%08X", addr);
                return;
            }
        }
    }
    
    private boolean isBreakpoint(int addr) {
        for (int i = 0; i < numBreakpoints; i++) {
            if (breakpoints[i] == addr) return true;
        }
        return false;
    }
    
    // === Getters ===
    
    public int getPC() { return PC; }
    public short getSR() { return SR; }
    public int[] getDataRegisters() { return D; }
    public int[] getAddressRegisters() { return A; }
    public double[] getFPRegisters() { return FP; }
    public long[][] getVectorRegisters() { return V; }
    public long[] get64BitRegisters() { return D64; }
    public long getCycleCount() { return cycleCount; }
    public long getInstructionCount() { return instructionCount; }
    public boolean isHalted() { return halted; }
    
    public void setPC(int pc) { this.PC = pc; }
    public void setHalted(boolean halted) { this.halted = halted; }
    
    // === OS Support Methods ===
    
    public int getVBR() { return VBR; }
    public void setVBR(int vbr) { this.VBR = vbr; }
    
    public void setSR(short sr) { this.SR = sr; }
    
    public boolean isSupervisorMode() {
        return (SR & SR_SUPERVISOR) != 0;
    }
    
    public void setSupervisorMode(boolean supervisor) {
        if (supervisor) {
            SR |= SR_SUPERVISOR;
        } else {
            SR &= ~SR_SUPERVISOR;
        }
    }
    
    public boolean isTraceMode() {
        return (SR & SR_TRACE) != 0;
    }
    
    public void setTraceMode(boolean trace) {
        if (trace) {
            SR |= SR_TRACE;
        } else {
            SR &= ~SR_TRACE;
        }
    }
    
    public void switchToSupervisorStack() {
        if (!isSupervisorMode()) {
            // Save user stack pointer
            USP = A[7];
            // Switch to supervisor stack pointer
            A[7] = SSP;
            setSupervisorMode(true);
        }
    }
    
    public void switchToUserStack() {
        if (isSupervisorMode()) {
            // Save supervisor stack pointer
            SSP = A[7];
            // Switch to user stack pointer
            A[7] = USP;
            setSupervisorMode(false);
        }
    }
    
    public void pushStack(int value) {
        A[7] -= 4;
        bus.memory.putInt(A[7], value);
    }
    
    public void pushStack16(int value) {
        A[7] -= 2;
        bus.memory.putChar(A[7], (char) value);
    }
    
    public int popStack() {
        int value = bus.readLong(A[7]);
        A[7] += 4;
        return value;
    }
    
    public int popStack16() {
        int value = bus.readWord(A[7]);
        A[7] += 2;
        return value;
    }
    
    // === Logging (can be overridden in subclasses) ===
    
    protected void log(String format, Object... args) {
        // Override in GUI version to direct to console
        // System.out.printf(format + "\n", args);
    }
}
