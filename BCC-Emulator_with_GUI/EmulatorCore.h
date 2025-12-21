/*
 * Enhanced BCC-500 Computer System Emulator
 *
 * New Features:
 * - Multi-threading support for parallel processor execution
 * - Binary file loader for actual BCC-500 programs
 * - Interactive debugger with breakpoints and single-stepping
 * - I/O peripheral simulation (tape drives, card readers)
 * - Memory-mapped I/O for device communication
 * - Extended instruction set based on SDS 940
 *
 * Fixed Issues:
 * - Added missing Console class
 * - Fixed malformed reset() function definition
 * - Made mutex mutable for const methods
 * - Fixed potential deadlock in IOController
 * - Added missing opcode handlers
 * - Made state variable atomic for thread safety
 */

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <functional>

// Type definitions for BCC-500 word sizes
using Word24 = uint32_t;
using Addr18 = uint32_t;
using Opcode6 = uint8_t;

// Bit masks
constexpr Word24 WORD24_MASK = 0xFFFFFF;
constexpr Addr18 ADDR18_MASK = 0x3FFFF;
constexpr Opcode6 OPCODE6_MASK = 0x3F;
constexpr Word24 SIGN_BIT = 0x800000;

// Memory-mapped I/O region
constexpr Addr18 MMIO_START = 0x3F000;
constexpr Addr18 MMIO_END = 0x3FFFF;

// Device registers
constexpr Addr18 TAPE_STATUS = 0x3F000;
constexpr Addr18 TAPE_DATA = 0x3F001;
constexpr Addr18 TAPE_COMMAND = 0x3F002;
constexpr Addr18 CARD_STATUS = 0x3F010;
constexpr Addr18 CARD_DATA = 0x3F011;
constexpr Addr18 CONSOLE_STATUS = 0x3F020;
constexpr Addr18 CONSOLE_DATA = 0x3F021;
constexpr Addr18 TERMINAL_STATUS = 0x3F030;
constexpr Addr18 TERMINAL_DATA = 0x3F031;
constexpr Addr18 TERMINAL_COMMAND = 0x3F032;

// Extended BCC-500 instruction opcodes (based on SDS 940)
enum class Opcode : Opcode6 {
    // Basic operations
    HALT = 0x00,
    NOP = 0x01,
    LOAD = 0x02,
    STORE = 0x03,
    ADD = 0x04,
    SUB = 0x05,
    AND = 0x06,
    OR = 0x07,
    XOR = 0x08,
    
    // Control flow
    JUMP = 0x09,
    JZE = 0x0A,
    JNZ = 0x0B,
    JGT = 0x0C,
    JLT = 0x0D,
    CALL = 0x0E,
    RET = 0x0F,
    
    // Stack operations
    PUSH = 0x10,
    POP = 0x11,
    
    // Shift operations
    SHIFT_L = 0x12,
    SHIFT_R = 0x13,
    
    // Arithmetic
    MUL = 0x14,
    DIV = 0x15,
    MOD = 0x16,
    CMP = 0x17,
    
    // Data movement
    MOVE = 0x18,
    INPUT = 0x19,
    OUTPUT = 0x1A,
    
    // Extended SDS 940 instructions
    LDA = 0x1B,      // Load A register
    LDB = 0x1C,      // Load B register
    STA = 0x1D,      // Store A register
    STB = 0x1E,      // Store B register
    LDX = 0x1F,      // Load index register
    STX = 0x20,      // Store index register
    EOR = 0x21,      // Exclusive OR (same as XOR but separate for compatibility)
    ADC = 0x22,      // Add with carry
    SBC = 0x23,      // Subtract with carry
    NEG = 0x24,      // Negate
    COM = 0x25,      // Complement
    INC = 0x26,      // Increment
    DEC = 0x27,      // Decrement
    ROL = 0x28,      // Rotate left
    ROR = 0x29,      // Rotate right
    SKE = 0x2A,      // Skip if equal
    SKN = 0x2B,      // Skip if not equal
    SKG = 0x2C,      // Skip if greater
    SKL = 0x2D,      // Skip if less
    JOV = 0x2E,      // Jump on overflow
    BRU = 0x2F,      // Branch unconditional
    EXU = 0x30,      // Execute
    MIY = 0x31,      // Memory increment by Y
    INT = 0x32,      // Interrupt
    RTI = 0x33       // Return from interrupt
};

// Processor execution states
enum class ProcessorState {
    IDLE,
    RUNNING,
    WAITING,
    HALTED,
    ERROR,
    BREAKPOINT
};

// Processor flags
struct Flags {
    bool zero = false;
    bool negative = false;
    bool overflow = false;
    bool carry = false;
    bool interrupt_enabled = false;
};

// Forward declarations
class SharedMemory;
class IOController;

// I/O Device base class
class IODevice {
protected:
    std::string name;
    bool ready;
    
public:
    IODevice(const std::string& deviceName) : name(deviceName), ready(true) {}
    virtual ~IODevice() = default;
    
    virtual Word24 read() = 0;
    virtual void write(Word24 data) = 0;
    virtual Word24 getStatus() = 0;
    virtual void command(Word24 cmd) = 0;
    
    bool isReady() const { return ready; }
    std::string getName() const { return name; }
};

// Tape Drive Simulation
class TapeDrive : public IODevice {
private:
    std::vector<Word24> tape;
    size_t position;
    bool mounted;
    
public:
    TapeDrive() : IODevice("Tape Drive"), position(0), mounted(false) {}
    
    void mount(const std::vector<Word24>& data) {
        tape = data;
        position = 0;
        mounted = true;
        ready = true;
    }
    
    void unmount() {
        tape.clear();
        position = 0;
        mounted = false;
    }
    
    Word24 read() override {
        if (!mounted || position >= tape.size()) {
            return 0;
        }
        return tape[position++];
    }
    
    void write(Word24 data) override {
        if (!mounted) return;
        if (position >= tape.size()) {
            tape.push_back(data);
        } else {
            tape[position] = data;
        }
        position++;
    }
    
    Word24 getStatus() override {
        Word24 status = 0;
        if (mounted) status |= 0x01;
        if (ready) status |= 0x02;
        if (position >= tape.size()) status |= 0x04; // EOF
        return status;
    }
    
    void command(Word24 cmd) override {
        switch (cmd) {
            case 0: position = 0; break;           // Rewind
            case 1: if (position > 0) position--; break;  // Backward
            case 2: if (position < tape.size()) position++; break; // Forward
        }
    }
};

// Card Reader Simulation
class CardReader : public IODevice {
private:
    std::queue<Word24> cardBuffer;
    
public:
    CardReader() : IODevice("Card Reader") {}
    
    void loadCard(const std::vector<Word24>& cardData) {
        for (auto word : cardData) {
            cardBuffer.push(word);
        }
    }
    
    Word24 read() override {
        if (cardBuffer.empty()) return 0;
        Word24 data = cardBuffer.front();
        cardBuffer.pop();
        return data;
    }
    
    void write(Word24 data) override {
        // Card readers are read-only
        (void)data;
    }
    
    Word24 getStatus() override {
        Word24 status = 0;
        if (ready) status |= 0x01;
        if (!cardBuffer.empty()) status |= 0x02;
        return status;
    }
    
    void command(Word24 cmd) override {
        if (cmd == 0) {
            while (!cardBuffer.empty()) cardBuffer.pop();
        }
    }
};

// Console Device (FIX: This class was missing)
class Console : public IODevice {
private:
    std::queue<Word24> inputBuffer;
    std::queue<Word24> outputBuffer;
    mutable std::mutex consoleMutex;
    
public:
    Console() : IODevice("Console") {}
    
    Word24 read() override {
        std::lock_guard<std::mutex> lock(consoleMutex);
        if (inputBuffer.empty()) return 0;
        Word24 data = inputBuffer.front();
        inputBuffer.pop();
        return data;
    }
    
    void write(Word24 data) override {
        std::lock_guard<std::mutex> lock(consoleMutex);
        outputBuffer.push(data);
        // Echo to stdout
        char ch = static_cast<char>(data & 0xFF);
        if (ch >= 32 || ch == '\n' || ch == '\r' || ch == '\t') {
            std::cout << ch << std::flush;
        }
    }
    
    Word24 getStatus() override {
        std::lock_guard<std::mutex> lock(consoleMutex);
        Word24 status = 0;
        if (ready) status |= 0x01;
        if (!inputBuffer.empty()) status |= 0x02;  // Input available
        if (!outputBuffer.empty()) status |= 0x04; // Output pending
        return status;
    }
    
    void command(Word24 cmd) override {
        std::lock_guard<std::mutex> lock(consoleMutex);
        switch (cmd) {
            case 0: // Clear input buffer
                while (!inputBuffer.empty()) inputBuffer.pop();
                break;
            case 1: // Clear output buffer
                while (!outputBuffer.empty()) outputBuffer.pop();
                break;
        }
    }
    
    void addInput(char ch) {
        std::lock_guard<std::mutex> lock(consoleMutex);
        inputBuffer.push(static_cast<Word24>(ch));
    }
    
    void addInput(const std::string& str) {
        std::lock_guard<std::mutex> lock(consoleMutex);
        for (char ch : str) {
            inputBuffer.push(static_cast<Word24>(ch));
        }
    }
};

// Terminal Display with screen buffer
class TerminalDisplay : public IODevice {
private:
    static constexpr int WIDTH = 80;
    static constexpr int HEIGHT = 24;
    
    std::vector<char> screenBuffer;
    int cursorX, cursorY;
    std::queue<Word24> inputBuffer;
    mutable std::mutex displayMutex;  // FIX: Made mutable for const methods
    bool cursorVisible;
    
    void scrollUp() {
        for (int y = 1; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                screenBuffer[((y-1) * WIDTH) + x] = screenBuffer[(y * WIDTH) + x];
            }
        }
        // Clear last line
        for (int x = 0; x < WIDTH; x++) {
            screenBuffer[((HEIGHT-1) * WIDTH) + x] = ' ';
        }
    }
    
    void putChar(char ch) {
        if (ch == '\n') {
            cursorX = 0;
            cursorY++;
            if (cursorY >= HEIGHT) {
                cursorY = HEIGHT - 1;
                scrollUp();
            }
        } else if (ch == '\r') {
            cursorX = 0;
        } else if (ch == '\b') {
            if (cursorX > 0) {
                cursorX--;
                screenBuffer[(cursorY * WIDTH) + cursorX] = ' ';
            }
        } else if (ch == '\t') {
            cursorX = ((cursorX / 8) + 1) * 8;
            if (cursorX >= WIDTH) {
                cursorX = 0;
                cursorY++;
                if (cursorY >= HEIGHT) {
                    cursorY = HEIGHT - 1;
                    scrollUp();
                }
            }
        } else if (ch >= 32 && ch <= 126) {
            screenBuffer[(cursorY * WIDTH) + cursorX] = ch;
            cursorX++;
            if (cursorX >= WIDTH) {
                cursorX = 0;
                cursorY++;
                if (cursorY >= HEIGHT) {
                    cursorY = HEIGHT - 1;
                    scrollUp();
                }
            }
        }
    }
    
public:
    TerminalDisplay() : IODevice("Terminal"), 
                        screenBuffer(WIDTH * HEIGHT, ' '),
                        cursorX(0), cursorY(0), cursorVisible(true) {}
    
    Word24 read() override {
        std::lock_guard<std::mutex> lock(displayMutex);
        if (inputBuffer.empty()) return 0;
        Word24 data = inputBuffer.front();
        inputBuffer.pop();
        return data;
    }
    
    void write(Word24 data) override {
        std::lock_guard<std::mutex> lock(displayMutex);
        char ch = static_cast<char>(data & 0xFF);
        putChar(ch);
    }
    
    Word24 getStatus() override {
        std::lock_guard<std::mutex> lock(displayMutex);
        Word24 status = 0;
        if (ready) status |= 0x01;
        if (!inputBuffer.empty()) status |= 0x02;
        status |= (static_cast<Word24>(cursorX) << 8);
        status |= (static_cast<Word24>(cursorY) << 16);
        return status;
    }
    
    void command(Word24 cmd) override {
        std::lock_guard<std::mutex> lock(displayMutex);
        switch (cmd & 0xFF) {
            case 0: clear(); break;
            case 1: cursorX = (cmd >> 8) & 0xFF; break;
            case 2: cursorY = (cmd >> 8) & 0xFF; break;
            case 3: cursorX = (cmd >> 8) & 0xFF; cursorY = (cmd >> 16) & 0xFF; break;
            case 4: cursorVisible = ((cmd >> 8) & 1) != 0; break;
        }
    }
    
    void clear() {
        std::fill(screenBuffer.begin(), screenBuffer.end(), ' ');
        cursorX = 0;
        cursorY = 0;
    }
    
    void addInput(const std::string& str) {
        std::lock_guard<std::mutex> lock(displayMutex);
        for (char ch : str) {
            inputBuffer.push(static_cast<Word24>(ch));
        }
    }
    
    void render() const {
        std::lock_guard<std::mutex> lock(displayMutex);
        
        // Clear screen (ANSI escape codes)
        std::cout << "\033[2J\033[H";
        
        // Draw top border
        std::cout << "+";
        for (int i = 0; i < WIDTH; i++) std::cout << "-";
        std::cout << "+\n";
        
        // Draw screen content
        for (int y = 0; y < HEIGHT; y++) {
            std::cout << "|";
            for (int x = 0; x < WIDTH; x++) {
                char ch = screenBuffer[(y * WIDTH) + x];
                // Highlight cursor position
                if (x == cursorX && y == cursorY && cursorVisible) {
                    std::cout << "\033[7m" << ch << "\033[0m"; // Inverted colors
                } else {
                    std::cout << ch;
                }
            }
            std::cout << "|\n";
        }
        
        // Draw bottom border
        std::cout << "+";
        for (int i = 0; i < WIDTH; i++) std::cout << "-";
        std::cout << "+\n";
        
        // Status line
        std::cout << "Cursor: (" << cursorX << "," << cursorY << ")  ";
        std::cout << "Input buffer: " << inputBuffer.size() << " chars\n";
        
        std::cout << std::flush;
    }
    
    std::string getScreenContents() const {
        std::lock_guard<std::mutex> lock(displayMutex);
        std::string result;
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                result += screenBuffer[(y * WIDTH) + x];
            }
            result += '\n';
        }
        return result;
    }
    
    int getWidth() const { return WIDTH; }
    int getHeight() const { return HEIGHT; }
};

// I/O Controller (FIX: Removed deadlock by not calling getDevice() while holding lock)
class IOController {
private:
    std::map<std::string, std::shared_ptr<IODevice>> devices;
    mutable std::mutex ioMutex;
    
public:
    void registerDevice(const std::string& name, std::shared_ptr<IODevice> device) {
        std::lock_guard<std::mutex> lock(ioMutex);
        devices[name] = device;
    }
    
    std::shared_ptr<IODevice> getDevice(const std::string& name) {
        std::lock_guard<std::mutex> lock(ioMutex);
        auto it = devices.find(name);
        return (it != devices.end()) ? it->second : nullptr;
    }
    
    Word24 readMMIO(Addr18 address) {
        std::lock_guard<std::mutex> lock(ioMutex);
        
        // FIX: Access devices map directly instead of calling getDevice()
        if (address == TAPE_STATUS) {
            auto it = devices.find("tape");
            return (it != devices.end()) ? it->second->getStatus() : 0;
        } else if (address == TAPE_DATA) {
            auto it = devices.find("tape");
            return (it != devices.end()) ? it->second->read() : 0;
        } else if (address == CARD_STATUS) {
            auto it = devices.find("card");
            return (it != devices.end()) ? it->second->getStatus() : 0;
        } else if (address == CARD_DATA) {
            auto it = devices.find("card");
            return (it != devices.end()) ? it->second->read() : 0;
        } else if (address == CONSOLE_STATUS) {
            auto it = devices.find("console");
            return (it != devices.end()) ? it->second->getStatus() : 0;
        } else if (address == CONSOLE_DATA) {
            auto it = devices.find("console");
            return (it != devices.end()) ? it->second->read() : 0;
        } else if (address == TERMINAL_STATUS) {
            auto it = devices.find("terminal");
            return (it != devices.end()) ? it->second->getStatus() : 0;
        } else if (address == TERMINAL_DATA) {
            auto it = devices.find("terminal");
            return (it != devices.end()) ? it->second->read() : 0;
        }
        
        return 0;
    }
    
    void writeMMIO(Addr18 address, Word24 value) {
        std::lock_guard<std::mutex> lock(ioMutex);
        
        // FIX: Access devices map directly instead of calling getDevice()
        if (address == TAPE_COMMAND) {
            auto it = devices.find("tape");
            if (it != devices.end()) it->second->command(value);
        } else if (address == TAPE_DATA) {
            auto it = devices.find("tape");
            if (it != devices.end()) it->second->write(value);
        } else if (address == CONSOLE_DATA) {
            auto it = devices.find("console");
            if (it != devices.end()) it->second->write(value);
        } else if (address == TERMINAL_DATA) {
            auto it = devices.find("terminal");
            if (it != devices.end()) it->second->write(value);
        } else if (address == TERMINAL_COMMAND) {
            auto it = devices.find("terminal");
            if (it != devices.end()) it->second->command(value);
        }
    }
};

// Shared memory with MMIO support
class SharedMemory {
private:
    std::vector<Word24> memory;
    size_t size;
    std::atomic<uint64_t> accessCount;
    mutable std::mutex memMutex;
    IOController* ioController;
    
public:
    SharedMemory(size_t memSize = 0x40000) 
        : memory(memSize, 0), size(memSize), accessCount(0), ioController(nullptr) {}
    
    void setIOController(IOController* controller) {
        ioController = controller;
    }
    
    Word24 read(Addr18 address) {
        if (address >= size) {
            throw std::out_of_range("Memory address out of bounds");
        }
        
        accessCount++;
        
        // Check for memory-mapped I/O
        if (address >= MMIO_START && address <= MMIO_END && ioController) {
            return ioController->readMMIO(address);
        }
        
        std::lock_guard<std::mutex> lock(memMutex);
        return memory[address] & WORD24_MASK;
    }
    
    void write(Addr18 address, Word24 value) {
        if (address >= size) {
            throw std::out_of_range("Memory address out of bounds");
        }
        
        accessCount++;
        
        // Check for memory-mapped I/O
        if (address >= MMIO_START && address <= MMIO_END && ioController) {
            ioController->writeMMIO(address, value);
            return;
        }
        
        std::lock_guard<std::mutex> lock(memMutex);
        memory[address] = value & WORD24_MASK;
    }
    
    void loadProgram(const std::vector<Word24>& program, Addr18 startAddress = 0) {
        for (size_t i = 0; i < program.size(); i++) {
            write(startAddress + static_cast<Addr18>(i), program[i]);
        }
    }
    
    uint64_t getAccessCount() const { return accessCount; }
    size_t getSize() const { return size; }
};

// Debugger
class Debugger {
private:
    std::set<Addr18> breakpoints;
    mutable std::mutex debugMutex;
    std::condition_variable debugCV;
    bool singleStep;
    bool continueExecution;
    
public:
    Debugger() : singleStep(false), continueExecution(true) {}
    
    void addBreakpoint(Addr18 address) {
        std::lock_guard<std::mutex> lock(debugMutex);
        breakpoints.insert(address);
        std::cout << "Breakpoint set at 0x" << std::hex << address << "\n";
    }
    
    void removeBreakpoint(Addr18 address) {
        std::lock_guard<std::mutex> lock(debugMutex);
        breakpoints.erase(address);
        std::cout << "Breakpoint removed at 0x" << std::hex << address << "\n";
    }
    
    void clearBreakpoints() {
        std::lock_guard<std::mutex> lock(debugMutex);
        breakpoints.clear();
        std::cout << "All breakpoints cleared\n";
    }
    
    bool hasBreakpoint(Addr18 address) {
        std::lock_guard<std::mutex> lock(debugMutex);
        return breakpoints.find(address) != breakpoints.end();
    }
    
    void setSingleStep(bool enable) {
        std::lock_guard<std::mutex> lock(debugMutex);
        singleStep = enable;
    }
    
    bool isSingleStep() {
        std::lock_guard<std::mutex> lock(debugMutex);
        return singleStep;
    }
    
    void waitForContinue() {
        std::unique_lock<std::mutex> lock(debugMutex);
        continueExecution = false;
        debugCV.wait(lock, [this] { return continueExecution; });
    }
    
    void continueExec() {
        std::lock_guard<std::mutex> lock(debugMutex);
        continueExecution = true;
        debugCV.notify_all();
    }
    
    void listBreakpoints() {
        std::lock_guard<std::mutex> lock(debugMutex);
        if (breakpoints.empty()) {
            std::cout << "No breakpoints set\n";
        } else {
            std::cout << "Breakpoints:\n";
            for (auto addr : breakpoints) {
                std::cout << "  0x" << std::hex << addr << "\n";
            }
        }
    }
};

// BCC-500 Processor with threading support
class BCC500Processor {
private:
    int procId;
    SharedMemory* memory;
    std::atomic<ProcessorState> state;  // FIX: Made atomic for thread safety
    Debugger* debugger;
    
    // Registers
    Addr18 pc;
    Word24 acc;      // A register
    Word24 regB;     // B register
    Word24 regX;     // Index register
    Word24 ir;
    Addr18 mar;
    Word24 mdr;
    Addr18 sp;
    Flags flags;
    
    // Interrupt handling
    Addr18 interruptVector;
    Addr18 savedPC;
    
    // Statistics
    std::atomic<uint64_t> cycles;
    std::atomic<uint64_t> instructionsExecuted;
    
    // Threading
    std::thread processorThread;
    std::atomic<bool> running;
    mutable std::mutex stateMutex;
    
    void updateFlags(int64_t value);
    
public:
    BCC500Processor(int id, SharedMemory* sharedMem, Debugger* dbg = nullptr);
    ~BCC500Processor();
    
    void reset();
    Word24 fetch();
    void decode(Word24 instruction, Opcode6& opcode, Addr18& address);
    void execute(Opcode6 opcode, Addr18 address);
    void step();
    void run(uint64_t maxCycles = 0);
    void runAsync();
    void stop();
    void join();
    
    // Getters
    ProcessorState getState() const { return state.load(); }
    Addr18 getPC() const { return pc; }
    Word24 getACC() const { return acc; }
    Word24 getRegB() const { return regB; }
    Word24 getRegX() const { return regX; }
    Addr18 getSP() const { return sp; }
    uint64_t getCycles() const { return cycles; }
    uint64_t getInstructionsExecuted() const { return instructionsExecuted; }
    const Flags& getFlags() const { return flags; }
    int getId() const { return procId; }
    
    void setState(ProcessorState newState) { state.store(newState); }
    void setPC(Addr18 address) { pc = address & ADDR18_MASK; }
    
    void printState() const;
};

// Implementation
BCC500Processor::BCC500Processor(int id, SharedMemory* sharedMem, Debugger* dbg)
    : procId(id), memory(sharedMem), state(ProcessorState::IDLE), debugger(dbg),
      pc(0), acc(0), regB(0), regX(0), ir(0), mar(0), mdr(0), sp(0xFFFF),
      interruptVector(0), savedPC(0),
      cycles(0), instructionsExecuted(0), running(false) {}

BCC500Processor::~BCC500Processor() {
    stop();
    join();
}

void BCC500Processor::reset() {
    stop();
    pc = 0;
    acc = 0;
    regB = 0;
    regX = 0;
    ir = 0;
    mar = 0;
    mdr = 0;
    sp = 0xFFFF;
    flags = Flags();
    state.store(ProcessorState::IDLE);
    cycles = 0;
    instructionsExecuted = 0;
}

void BCC500Processor::stop() {
    running = false;
}

void BCC500Processor::join() {
    if (processorThread.joinable()) {
        processorThread.join();
    }
}

void BCC500Processor::printState() const {
    std::cout << "Processor " << procId << " State:\n";
    std::cout << "  State: ";
    switch (state.load()) {
        case ProcessorState::IDLE: std::cout << "IDLE\n"; break;
        case ProcessorState::RUNNING: std::cout << "RUNNING\n"; break;
        case ProcessorState::WAITING: std::cout << "WAITING\n"; break;
        case ProcessorState::HALTED: std::cout << "HALTED\n"; break;
        case ProcessorState::ERROR: std::cout << "ERROR\n"; break;
        case ProcessorState::BREAKPOINT: std::cout << "BREAKPOINT\n"; break;
    }
    std::cout << "  PC: 0x" << std::hex << std::setw(5) << std::setfill('0') << pc << "\n";
    std::cout << "  ACC: 0x" << std::hex << std::setw(6) << std::setfill('0') << acc << "\n";
    std::cout << "  B: 0x" << std::hex << std::setw(6) << std::setfill('0') << regB << "\n";
    std::cout << "  X: 0x" << std::hex << std::setw(6) << std::setfill('0') << regX << "\n";
    std::cout << "  SP: 0x" << std::hex << std::setw(5) << std::setfill('0') << sp << "\n";
    std::cout << "  Flags: Z=" << flags.zero << " N=" << flags.negative 
              << " O=" << flags.overflow << " C=" << flags.carry << "\n";
    std::cout << "  Cycles: " << std::dec << cycles << "\n";
    std::cout << "  Instructions: " << instructionsExecuted << "\n";
}

Word24 BCC500Processor::fetch() {
    mar = pc & ADDR18_MASK;
    Word24 instruction = memory->read(mar);
    ir = instruction;
    pc = (pc + 1) & ADDR18_MASK;
    return instruction;
}

void BCC500Processor::decode(Word24 instruction, Opcode6& opcode, Addr18& address) {
    opcode = (instruction >> 18) & OPCODE6_MASK;
    address = instruction & ADDR18_MASK;
}

void BCC500Processor::execute(Opcode6 opcodeVal, Addr18 address) {
    instructionsExecuted++;
    
    Opcode op = static_cast<Opcode>(opcodeVal);
    
    switch (op) {
        case Opcode::HALT:
            state.store(ProcessorState::HALTED);
            break;
            
        case Opcode::NOP:
            break;
            
        case Opcode::LOAD:
        case Opcode::LDA:
            acc = memory->read(address);
            updateFlags(acc);
            break;
            
        case Opcode::LDB:
            regB = memory->read(address);
            updateFlags(regB);
            break;
            
        case Opcode::LDX:
            regX = memory->read(address);
            break;
            
        case Opcode::STORE:
        case Opcode::STA:
            memory->write(address, acc);
            break;
            
        case Opcode::STB:
            memory->write(address, regB);
            break;
            
        case Opcode::STX:
            memory->write(address, regX);
            break;
            
        case Opcode::ADD: {
            Word24 operand = memory->read(address);
            int64_t result = static_cast<int64_t>(acc) + static_cast<int64_t>(operand);
            flags.carry = (result > WORD24_MASK);
            updateFlags(result);
            acc = result & WORD24_MASK;
            break;
        }
        
        case Opcode::ADC: {
            Word24 operand = memory->read(address);
            int64_t result = static_cast<int64_t>(acc) + static_cast<int64_t>(operand) + (flags.carry ? 1 : 0);
            flags.carry = (result > WORD24_MASK);
            updateFlags(result);
            acc = result & WORD24_MASK;
            break;
        }
        
        case Opcode::SUB: {
            Word24 operand = memory->read(address);
            int64_t result = static_cast<int64_t>(acc) - static_cast<int64_t>(operand);
            updateFlags(result);
            acc = result & WORD24_MASK;
            break;
        }
        
        case Opcode::SBC: {
            Word24 operand = memory->read(address);
            int64_t result = static_cast<int64_t>(acc) - static_cast<int64_t>(operand) - (flags.carry ? 0 : 1);
            updateFlags(result);
            acc = result & WORD24_MASK;
            break;
        }
        
        case Opcode::AND:
            acc = (acc & memory->read(address)) & WORD24_MASK;
            updateFlags(acc);
            break;
            
        case Opcode::OR:
            acc = (acc | memory->read(address)) & WORD24_MASK;
            updateFlags(acc);
            break;
            
        case Opcode::XOR:
        case Opcode::EOR:
            acc = (acc ^ memory->read(address)) & WORD24_MASK;
            updateFlags(acc);
            break;
            
        case Opcode::NEG:
            acc = (~acc + 1) & WORD24_MASK;
            updateFlags(acc);
            break;
            
        case Opcode::COM:
            acc = (~acc) & WORD24_MASK;
            updateFlags(acc);
            break;
            
        case Opcode::INC:
            acc = (acc + 1) & WORD24_MASK;
            updateFlags(acc);
            break;
            
        case Opcode::DEC:
            acc = (acc - 1) & WORD24_MASK;
            updateFlags(acc);
            break;
            
        case Opcode::JUMP:
        case Opcode::BRU:
            pc = address;
            break;
            
        case Opcode::JZE:
            if (flags.zero) pc = address;
            break;
            
        case Opcode::JNZ:
            if (!flags.zero) pc = address;
            break;
            
        case Opcode::JGT:
            if (!flags.zero && !flags.negative) pc = address;
            break;
            
        case Opcode::JLT:
            if (flags.negative) pc = address;
            break;
            
        case Opcode::JOV:
            if (flags.overflow) pc = address;
            break;
            
        case Opcode::CALL:
            memory->write(sp, pc);
            sp = (sp - 1) & ADDR18_MASK;
            pc = address;
            break;
            
        case Opcode::RET:
            sp = (sp + 1) & ADDR18_MASK;
            pc = memory->read(sp);
            break;
            
        case Opcode::PUSH:
            memory->write(sp, acc);
            sp = (sp - 1) & ADDR18_MASK;
            break;
            
        case Opcode::POP:
            sp = (sp + 1) & ADDR18_MASK;
            acc = memory->read(sp);
            break;
            
        case Opcode::SHIFT_L:
            acc = (acc << (address & 0x1F)) & WORD24_MASK;
            updateFlags(acc);
            break;
            
        case Opcode::SHIFT_R:
            acc = (acc >> (address & 0x1F)) & WORD24_MASK;
            updateFlags(acc);
            break;
            
        case Opcode::ROL: {
            int shift = address & 0x1F;
            acc = ((acc << shift) | (acc >> (24 - shift))) & WORD24_MASK;
            updateFlags(acc);
            break;
        }
        
        case Opcode::ROR: {
            int shift = address & 0x1F;
            acc = ((acc >> shift) | (acc << (24 - shift))) & WORD24_MASK;
            updateFlags(acc);
            break;
        }
        
        case Opcode::MUL: {
            Word24 operand = memory->read(address);
            uint64_t result = static_cast<uint64_t>(acc) * static_cast<uint64_t>(operand);
            acc = result & WORD24_MASK;
            updateFlags(acc);
            break;
        }
        
        case Opcode::DIV: {
            Word24 operand = memory->read(address);
            if (operand == 0) {
                state.store(ProcessorState::ERROR);
                break;
            }
            acc = (acc / operand) & WORD24_MASK;
            updateFlags(acc);
            break;
        }
        
        // FIX: Added missing MOD opcode handler
        case Opcode::MOD: {
            Word24 operand = memory->read(address);
            if (operand == 0) {
                state.store(ProcessorState::ERROR);
                break;
            }
            acc = (acc % operand) & WORD24_MASK;
            updateFlags(acc);
            break;
        }
        
        case Opcode::CMP: {
            Word24 operand = memory->read(address);
            int64_t result = static_cast<int64_t>(acc) - static_cast<int64_t>(operand);
            updateFlags(result);
            break;
        }
        
        // FIX: Added missing MOVE opcode handler
        case Opcode::MOVE: {
            Word24 value = memory->read(address);
            // MOVE copies from source address to destination (next word)
            Addr18 destAddr = memory->read((pc) & ADDR18_MASK);
            pc = (pc + 1) & ADDR18_MASK;
            memory->write(destAddr, value);
            break;
        }
        
        case Opcode::SKE:
            if (flags.zero) pc = (pc + 1) & ADDR18_MASK;
            break;
            
        case Opcode::SKN:
            if (!flags.zero) pc = (pc + 1) & ADDR18_MASK;
            break;
            
        case Opcode::SKG:
            if (!flags.zero && !flags.negative) pc = (pc + 1) & ADDR18_MASK;
            break;
            
        case Opcode::SKL:
            if (flags.negative) pc = (pc + 1) & ADDR18_MASK;
            break;
            
        case Opcode::OUTPUT:
            memory->write(CONSOLE_DATA, acc);
            break;
            
        case Opcode::INPUT:
            acc = memory->read(CONSOLE_DATA);
            break;
            
        // FIX: Added missing EXU (Execute) opcode handler
        case Opcode::EXU: {
            // Execute instruction at address without changing PC
            Word24 instruction = memory->read(address);
            Opcode6 exuOpcode;
            Addr18 exuAddress;
            decode(instruction, exuOpcode, exuAddress);
            // Recursively execute (but not another EXU to prevent infinite loops)
            if (static_cast<Opcode>(exuOpcode) != Opcode::EXU) {
                execute(exuOpcode, exuAddress);
            }
            break;
        }
        
        // FIX: Added missing MIY (Memory Increment by Y/regX) opcode handler
        case Opcode::MIY: {
            Word24 memValue = memory->read(address);
            memValue = (memValue + regX) & WORD24_MASK;
            memory->write(address, memValue);
            updateFlags(memValue);
            break;
        }
        
        // FIX: Added missing INT (Interrupt) opcode handler
        case Opcode::INT: {
            if (flags.interrupt_enabled) {
                // Save current PC and jump to interrupt vector
                savedPC = pc;
                memory->write(sp, pc);
                sp = (sp - 1) & ADDR18_MASK;
                memory->write(sp, acc);
                sp = (sp - 1) & ADDR18_MASK;
                pc = address;  // Interrupt vector address
                flags.interrupt_enabled = false;
            }
            break;
        }
        
        // FIX: Added missing RTI (Return from Interrupt) opcode handler
        case Opcode::RTI: {
            sp = (sp + 1) & ADDR18_MASK;
            acc = memory->read(sp);
            sp = (sp + 1) & ADDR18_MASK;
            pc = memory->read(sp);
            flags.interrupt_enabled = true;
            break;
        }
            
        default:
            throw std::runtime_error("Unknown opcode: " + std::to_string(opcodeVal));
    }
}

void BCC500Processor::updateFlags(int64_t value) {
    Word24 masked = value & WORD24_MASK;
    flags.zero = (masked == 0);
    flags.negative = ((masked & SIGN_BIT) != 0);
    flags.overflow = (value > static_cast<int64_t>(WORD24_MASK) || value < 0);
}

void BCC500Processor::step() {
    if (state.load() != ProcessorState::RUNNING) return;
    
    // Check for breakpoint
    if (debugger && debugger->hasBreakpoint(pc)) {
        state.store(ProcessorState::BREAKPOINT);
        std::cout << "\nBreakpoint hit at PC=0x" << std::hex << pc << "\n";
        printState();
        debugger->waitForContinue();
        state.store(ProcessorState::RUNNING);
    }
    
    Word24 instruction = fetch();
    Opcode6 opcode;
    Addr18 address;
    decode(instruction, opcode, address);
    execute(opcode, address);
    cycles++;
    
    // Single-step debugging
    if (debugger && debugger->isSingleStep()) {
        state.store(ProcessorState::BREAKPOINT);
        printState();
        debugger->waitForContinue();
        state.store(ProcessorState::RUNNING);
    }
}

void BCC500Processor::run(uint64_t maxCycles) {
    state.store(ProcessorState::RUNNING);
    uint64_t cycleCount = 0;
    
    while (state.load() == ProcessorState::RUNNING) {
        step();
        cycleCount++;
        
        if (maxCycles > 0 && cycleCount >= maxCycles) break;
    }
}

void BCC500Processor::runAsync() {
    running = true;
    processorThread = std::thread([this]() {
        this->run();
    });
}

// Binary file loader
class BinaryLoader {
public:
    static std::vector<Word24> loadBinary(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        
        std::vector<Word24> program;
        
        // Read 3 bytes at a time for 24-bit words
        while (file) {
            uint8_t bytes[3] = {0};
            file.read(reinterpret_cast<char*>(bytes), 3);
            
            if (file.gcount() == 3) {
                Word24 word = (static_cast<Word24>(bytes[0]) << 16) |
                              (static_cast<Word24>(bytes[1]) << 8) |
                              static_cast<Word24>(bytes[2]);
                program.push_back(word & WORD24_MASK);
            }
        }
        
        return program;
    }
    
    static void saveBinary(const std::string& filename, const std::vector<Word24>& program) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot create file: " + filename);
        }
        
        for (Word24 word : program) {
            uint8_t bytes[3];
            bytes[0] = (word >> 16) & 0xFF;
            bytes[1] = (word >> 8) & 0xFF;
            bytes[2] = word & 0xFF;
            file.write(reinterpret_cast<char*>(bytes), 3);
        }
    }
};

// Enhanced BCC-500 System
class BCC500System {
private:
    SharedMemory memory;
    std::vector<std::unique_ptr<BCC500Processor>> processors;
    Debugger debugger;
    IOController ioController;
    
public:
    BCC500System() : memory() {
        memory.setIOController(&ioController);
        
        // Create 6 processors
        for (int i = 0; i < 6; i++) {
            processors.push_back(std::make_unique<BCC500Processor>(i, &memory, &debugger));
        }
        
        // Setup I/O devices
        ioController.registerDevice("tape", std::make_shared<TapeDrive>());
        ioController.registerDevice("card", std::make_shared<CardReader>());
        ioController.registerDevice("console", std::make_shared<Console>());
        ioController.registerDevice("terminal", std::make_shared<TerminalDisplay>());
    }
    
    void reset() {
        for (auto& proc : processors) {
            proc->reset();
        }
        debugger.clearBreakpoints();
    }
    
    void loadProgram(const std::vector<Word24>& program, int processorId = 0, 
                     Addr18 startAddress = 0) {
        memory.loadProgram(program, startAddress);
        processors[processorId]->setPC(startAddress);
    }
    
    void loadBinaryFile(const std::string& filename, int processorId = 0, 
                        Addr18 startAddress = 0) {
        auto program = BinaryLoader::loadBinary(filename);
        loadProgram(program, processorId, startAddress);
        std::cout << "Loaded " << program.size() << " words from " << filename << "\n";
    }
    
    void runProcessor(int processorId, uint64_t maxCycles = 0) {
        if (processorId < 0 || processorId >= 6) {
            throw std::out_of_range("Invalid processor ID");
        }
        processors[processorId]->run(maxCycles);
    }
    
    void runProcessorAsync(int processorId) {
        if (processorId < 0 || processorId >= 6) {
            throw std::out_of_range("Invalid processor ID");
        }
        processors[processorId]->runAsync();
    }
    
    void runAllProcessorsAsync() {
        for (auto& proc : processors) {
            if (proc->getState() == ProcessorState::IDLE) {
                proc->setState(ProcessorState::RUNNING);
                proc->runAsync();
            }
        }
    }
    
    void stopAllProcessors() {
        for (auto& proc : processors) {
            proc->stop();
        }
    }
    
    void joinAllProcessors() {
        for (auto& proc : processors) {
            proc->join();
        }
    }
    
    BCC500Processor& getProcessor(int id) { return *processors[id]; }
    SharedMemory& getMemory() { return memory; }
    Debugger& getDebugger() { return debugger; }
    IOController& getIOController() { return ioController; }
    
    void printSystemState() const {
        std::cout << "\n=== BCC-500 System State ===\n";
        for (const auto& proc : processors) {
            proc->printState();
            std::cout << "\n";
        }
        std::cout << "Memory Accesses: " << memory.getAccessCount() << "\n";
    }
};

// Interactive Debugger Interface
class DebuggerInterface {
private:
    BCC500System& system;
    
public:
    DebuggerInterface(BCC500System& sys) : system(sys) {}
    
    void run() {
        std::string command;
        std::cout << "\nBCC-500 Interactive Debugger\n";
        std::cout << "Type 'help' for commands\n\n";
        
        while (true) {
            std::cout << "dbg> ";
            std::getline(std::cin, command);
            
            if (command.empty()) continue;
            
            std::istringstream iss(command);
            std::string cmd;
            iss >> cmd;
            
            if (cmd == "help") {
                printHelp();
            } else if (cmd == "break" || cmd == "b") {
                Addr18 addr;
                if (iss >> std::hex >> addr) {
                    system.getDebugger().addBreakpoint(addr);
                } else {
                    std::cout << "Usage: break <address>\n";
                }
            } else if (cmd == "delete" || cmd == "d") {
                Addr18 addr;
                if (iss >> std::hex >> addr) {
                    system.getDebugger().removeBreakpoint(addr);
                } else {
                    std::cout << "Usage: delete <address>\n";
                }
            } else if (cmd == "list") {
                system.getDebugger().listBreakpoints();
            } else if (cmd == "continue" || cmd == "c") {
                system.getDebugger().continueExec();
            } else if (cmd == "step" || cmd == "s") {
                system.getDebugger().setSingleStep(true);
                system.getDebugger().continueExec();
            } else if (cmd == "display" || cmd == "disp") {
                auto terminal = std::dynamic_pointer_cast<TerminalDisplay>(
                    system.getIOController().getDevice("terminal"));
                if (terminal) {
                    terminal->render();
                } else {
                    std::cout << "Terminal device not found\n";
                }
            } else if (cmd == "run" || cmd == "r") {
                int procId;
                if (iss >> procId) {
                    system.getDebugger().setSingleStep(false);
                    system.runProcessorAsync(procId);
                    std::cout << "Running processor " << procId << "\n";
                } else {
                    std::cout << "Usage: run <processor_id>\n";
                }
            } else if (cmd == "stop") {
                system.stopAllProcessors();
                std::cout << "All processors stopped\n";
            } else if (cmd == "print" || cmd == "p") {
                int procId;
                if (iss >> procId) {
                    system.getProcessor(procId).printState();
                } else {
                    system.printSystemState();
                }
            } else if (cmd == "memory" || cmd == "m") {
                Addr18 addr;
                if (iss >> std::hex >> addr) {
                    Word24 value = system.getMemory().read(addr);
                    std::cout << "Memory[0x" << std::hex << addr << "] = 0x" 
                              << std::setw(6) << std::setfill('0') << value << "\n";
                } else {
                    std::cout << "Usage: memory <address>\n";
                }
            } else if (cmd == "load") {
                std::string filename;
                int procId = 0;
                Addr18 addr = 0;
                iss >> filename >> procId >> std::hex >> addr;
                try {
                    system.loadBinaryFile(filename, procId, addr);
                } catch (const std::exception& e) {
                    std::cout << "Error: " << e.what() << "\n";
                }
            } else if (cmd == "quit" || cmd == "q") {
                system.stopAllProcessors();
                system.joinAllProcessors();
                break;
            } else {
                std::cout << "Unknown command: " << cmd << "\n";
            }
        }
    }
    
    void printHelp() {
        std::cout << "Commands:\n";
        std::cout << "  help                  - Show this help\n";
        std::cout << "  break/b <addr>        - Set breakpoint at address\n";
        std::cout << "  delete/d <addr>       - Remove breakpoint\n";
        std::cout << "  list                  - List all breakpoints\n";
        std::cout << "  continue/c            - Continue execution\n";
        std::cout << "  step/s                - Single step\n";
        std::cout << "  display/disp          - Render terminal display\n";
        std::cout << "  run/r <proc_id>       - Run processor\n";
        std::cout << "  stop                  - Stop all processors\n";
        std::cout << "  print/p [proc_id]     - Print processor/system state\n";
        std::cout << "  memory/m <addr>       - Examine memory\n";
        std::cout << "  load <file> [id] [addr] - Load binary file\n";
        std::cout << "  quit/q                - Quit debugger\n";
    }
};

// Example usage

