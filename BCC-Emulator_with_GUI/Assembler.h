#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <cstdint>

using Word24 = uint32_t;
using Addr18 = uint32_t;

// BCC-500 Assembler
// Syntax:
//   LABEL: OPCODE OPERAND  ; comment
//   LABEL: .WORD value
//   LABEL: .STRING "text"
//   LABEL: .BYTE value

class Assembler {
private:
    std::map<std::string, uint8_t> opcodes;
    std::map<std::string, Addr18> labels;
    std::vector<Word24> machineCode;
    std::vector<std::string> errors;

    void initOpcodes() {
        // Basic operations
        opcodes["HALT"] = 0x00;
        opcodes["NOP"] = 0x01;
        opcodes["LOAD"] = 0x02;
        opcodes["STORE"] = 0x03;
        opcodes["ADD"] = 0x04;
        opcodes["SUB"] = 0x05;
        opcodes["AND"] = 0x06;
        opcodes["OR"] = 0x07;
        opcodes["XOR"] = 0x08;

        // Control flow
        opcodes["JUMP"] = 0x09;
        opcodes["JMP"] = 0x09;  // Alias
        opcodes["JZE"] = 0x0A;
        opcodes["JZ"] = 0x0A;   // Alias
        opcodes["JNZ"] = 0x0B;
        opcodes["JGT"] = 0x0C;
        opcodes["JLT"] = 0x0D;
        opcodes["CALL"] = 0x0E;
        opcodes["RET"] = 0x0F;

        // Stack
        opcodes["PUSH"] = 0x10;
        opcodes["POP"] = 0x11;

        // Shift
        opcodes["SHL"] = 0x12;
        opcodes["SHR"] = 0x13;

        // Arithmetic
        opcodes["MUL"] = 0x14;
        opcodes["DIV"] = 0x15;
        opcodes["MOD"] = 0x16;
        opcodes["CMP"] = 0x17;

        // Data movement
        opcodes["MOVE"] = 0x18;
        opcodes["INPUT"] = 0x19;
        opcodes["IN"] = 0x19;   // Alias
        opcodes["OUTPUT"] = 0x1A;
        opcodes["OUT"] = 0x1A;  // Alias

        // Extended
        opcodes["LDA"] = 0x1B;
        opcodes["LDB"] = 0x1C;
        opcodes["STA"] = 0x1D;
        opcodes["STB"] = 0x1E;
        opcodes["LDX"] = 0x1F;
        opcodes["STX"] = 0x20;
        opcodes["EOR"] = 0x21;
        opcodes["ADC"] = 0x22;
        opcodes["SBC"] = 0x23;
        opcodes["NEG"] = 0x24;
        opcodes["COM"] = 0x25;
        opcodes["NOT"] = 0x25;  // Alias
        opcodes["INC"] = 0x26;
        opcodes["DEC"] = 0x27;
        opcodes["ROL"] = 0x28;
        opcodes["ROR"] = 0x29;
        opcodes["SKE"] = 0x2A;
        opcodes["SKN"] = 0x2B;
        opcodes["SKG"] = 0x2C;
        opcodes["SKL"] = 0x2D;
        opcodes["JOV"] = 0x2E;
        opcodes["BRU"] = 0x2F;
        opcodes["EXU"] = 0x30;
        opcodes["MIY"] = 0x31;
        opcodes["INT"] = 0x32;
        opcodes["RTI"] = 0x33;
    }

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    std::string toUpper(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    Addr18 parseOperand(const std::string& operand) {
        std::string op = trim(operand);

        // Hex number
        if (op.find("0x") == 0 || op.find("0X") == 0) {
            return std::stoul(op, nullptr, 16) & 0x3FFFF;
        }

        // Decimal number
        if (std::isdigit(op[0]) || (op[0] == '-' && op.length() > 1)) {
            return std::stoul(op, nullptr, 10) & 0x3FFFF;
        }

        // Character literal
        if (op[0] == '\'' && op.length() >= 3 && op[op.length()-1] == '\'') {
            return static_cast<Addr18>(op[1]) & 0x3FFFF;
        }

        // Label reference
        std::string upperOp = toUpper(op);
        if (labels.find(upperOp) != labels.end()) {
            return labels[upperOp];
        }

        // Unknown - assume 0 for now (forward reference)
        return 0;
    }

public:
    Assembler() {
        initOpcodes();
    }

    bool assemble(const std::string& sourceCode) {
        errors.clear();
        machineCode.clear();
        labels.clear();

        std::istringstream iss(sourceCode);
        std::string line;
        Addr18 currentAddress = 0;

        // First pass: collect labels
        while (std::getline(iss, line)) {
            // Remove comments
            size_t commentPos = line.find(';');
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }

            line = trim(line);
            if (line.empty()) continue;

            // Check for label
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string label = toUpper(trim(line.substr(0, colonPos)));
                labels[label] = currentAddress;
                line = trim(line.substr(colonPos + 1));
            }

            // Count instructions/directives
            if (!line.empty()) {
                std::istringstream lineStream(line);
                std::string mnemonic;
                lineStream >> mnemonic;
                mnemonic = toUpper(mnemonic);

                if (mnemonic == ".STRING") {
                    // Count string characters
                    size_t start = line.find('"');
                    size_t end = line.find('"', start + 1);
                    if (start != std::string::npos && end != std::string::npos) {
                        currentAddress += (end - start - 1);
                    }
                } else {
                    currentAddress++;
                }
            }
        }

        // Second pass: generate code
        iss.clear();
        iss.seekg(0);
        currentAddress = 0;

        while (std::getline(iss, line)) {
            // Remove comments
            size_t commentPos = line.find(';');
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }

            line = trim(line);
            if (line.empty()) continue;

            // Remove label
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                line = trim(line.substr(colonPos + 1));
            }

            if (line.empty()) continue;

            std::istringstream lineStream(line);
            std::string mnemonic;
            lineStream >> mnemonic;
            mnemonic = toUpper(mnemonic);

            // Handle directives
            if (mnemonic == ".WORD") {
                std::string value;
                lineStream >> value;
                machineCode.push_back(parseOperand(value) & 0xFFFFFF);
                currentAddress++;
            } else if (mnemonic == ".BYTE") {
                std::string value;
                lineStream >> value;
                machineCode.push_back(parseOperand(value) & 0xFF);
                currentAddress++;
            } else if (mnemonic == ".STRING") {
                size_t start = line.find('"');
                size_t end = line.find('"', start + 1);
                if (start != std::string::npos && end != std::string::npos) {
                    std::string str = line.substr(start + 1, end - start - 1);
                    for (char ch : str) {
                        machineCode.push_back(static_cast<Word24>(ch) & 0xFF);
                        currentAddress++;
                    }
                }
            } else {
                // Regular instruction
                if (opcodes.find(mnemonic) == opcodes.end()) {
                    errors.push_back("Unknown mnemonic: " + mnemonic);
                    continue;
                }

                uint8_t opcode = opcodes[mnemonic];
                Addr18 operand = 0;

                std::string operandStr;
                std::getline(lineStream, operandStr);
                operandStr = trim(operandStr);

                if (!operandStr.empty()) {
                    operand = parseOperand(operandStr);
                }

                Word24 instruction = (static_cast<Word24>(opcode) << 18) | (operand & 0x3FFFF);
                machineCode.push_back(instruction);
                currentAddress++;
            }
        }

        return errors.empty();
    }

    const std::vector<Word24>& getMachineCode() const {
        return machineCode;
    }

    const std::vector<std::string>& getErrors() const {
        return errors;
    }

    const std::map<std::string, Addr18>& getLabels() const {
        return labels;
    }
};

#endif // ASSEMBLER_H
