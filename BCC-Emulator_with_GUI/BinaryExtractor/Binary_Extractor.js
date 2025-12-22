import React, { useState } from 'react';
import { FileText, HardDrive, Cpu, Code, Play, Download, Zap, Upload } from 'lucide-react';

const BCC500BinaryDecoder = () => {
  const [binaryData, setBinaryData] = useState(null);
  const [decodedInstructions, setDecodedInstructions] = useState([]);
  const [view, setView] = useState('instructions');
  const [selectedAddr, setSelectedAddr] = useState(null);
  const [stats, setStats] = useState(null);
  const [fileName, setFileName] = useState('');

  // Opcode mapping - FIXED to match Assembler.h mnemonics exactly
  const opcodeMap = {
    0x00: 'HALT', 0x01: 'NOP', 0x02: 'LOAD', 0x03: 'STORE',
    0x04: 'ADD', 0x05: 'SUB', 0x06: 'AND', 0x07: 'OR',
    0x08: 'XOR', 0x09: 'JUMP', 0x0A: 'JZE', 0x0B: 'JNZ',
    0x0C: 'JGT', 0x0D: 'JLT', 0x0E: 'CALL', 0x0F: 'RET',
    0x10: 'PUSH', 0x11: 'POP', 
    0x12: 'SHL', 0x13: 'SHR',  // FIXED: was SHIFT_L/SHIFT_R
    0x14: 'MUL', 0x15: 'DIV', 0x16: 'MOD', 0x17: 'CMP',
    0x18: 'MOVE', 0x19: 'INPUT', 0x1A: 'OUTPUT', 0x1B: 'LDA',
    0x1C: 'LDB', 0x1D: 'STA', 0x1E: 'STB', 0x1F: 'LDX',
    0x20: 'STX', 0x21: 'EOR', 0x22: 'ADC', 0x23: 'SBC',
    0x24: 'NEG', 0x25: 'COM', 0x26: 'INC', 0x27: 'DEC',
    0x28: 'ROL', 0x29: 'ROR', 0x2A: 'SKE', 0x2B: 'SKN',
    0x2C: 'SKG', 0x2D: 'SKL', 0x2E: 'JOV', 0x2F: 'BRU',
    0x30: 'EXU', 0x31: 'MIY', 0x32: 'INT', 0x33: 'RTI'
  };

  // Instructions that don't use an operand (or operand is implicit)
  const noOperandOpcodes = new Set([
    0x00, // HALT
    0x01, // NOP
    0x0F, // RET
    0x11, // POP (operand is register, often 0)
    0x24, // NEG
    0x25, // COM
    0x26, // INC
    0x27, // DEC
    0x33  // RTI
  ]);

  const handleFileUpload = (event) => {
    const file = event.target.files[0];
    if (!file) return;

    setFileName(file.name);
    const reader = new FileReader();
    
    reader.onload = (e) => {
      const arrayBuffer = e.target.result;
      const uint8Data = new Uint8Array(arrayBuffer);
      decodeData(uint8Data);
    };
    
    reader.readAsArrayBuffer(file);
  };

  const decodeData = (uint8Data) => {
    const words = [];
    const instructions = [];
    const opcodeFreq = {};
    
    // Check if file size is valid (should be multiple of 3 for 24-bit words)
    const isAligned = uint8Data.length % 3 === 0;
    
    // Parse 24-bit words from the binary data
    for (let i = 0; i + 2 < uint8Data.length; i += 3) {
      const byte0 = uint8Data[i];
      const byte1 = uint8Data[i + 1];
      const byte2 = uint8Data[i + 2];
      
      // Read 3 bytes as a 24-bit word (big-endian, matching BinaryLoader)
      const word24 = ((byte0 & 0xFF) << 16) | ((byte1 & 0xFF) << 8) | (byte2 & 0xFF);
      
      // Extract opcode (top 6 bits) and address (bottom 18 bits)
      const opcode = (word24 >> 18) & 0x3F;
      const address = word24 & 0x3FFFF;
      
      // Check if this is a valid opcode
      const isValidOpcode = opcode in opcodeMap;
      
      // Heuristic: check if operand looks reasonable for this instruction type
      const isLikelyInstruction = isValidOpcode && (
        noOperandOpcodes.has(opcode) || // No-operand instructions: any operand OK
        address < 0x10000 ||            // Reasonable code/data address
        (address >= 0x3F000 && address <= 0x3FFFF) // MMIO region
      );
      
      // Check if this might be ASCII string data
      const isAsciiLike = byte0 >= 0x20 && byte0 <= 0x7E &&
                          byte1 >= 0x20 && byte1 <= 0x7E &&
                          byte2 >= 0x20 && byte2 <= 0x7E;
      
      if (isLikelyInstruction && !isAsciiLike) {
        const instruction = {
          address: Math.floor(i / 3),
          offset: i,
          word: word24,
          opcode: opcode,
          operand: address,
          mnemonic: opcodeMap[opcode],
          bytes: [byte0, byte1, byte2],
          isValid: true,
          hasOperand: !noOperandOpcodes.has(opcode)
        };
        
        instructions.push(instruction);
        words.push(word24);
        
        // Track opcode frequency
        opcodeFreq[opcodeMap[opcode]] = (opcodeFreq[opcodeMap[opcode]] || 0) + 1;
      } else {
        // Store as data word
        instructions.push({
          address: Math.floor(i / 3),
          offset: i,
          word: word24,
          opcode: null,
          operand: null,
          mnemonic: isAsciiLike ? 'ASCII' : 'DATA',
          bytes: [byte0, byte1, byte2],
          isValid: false,
          asciiChars: isAsciiLike ? String.fromCharCode(byte0, byte1, byte2) : null
        });
        words.push(word24);
      }
    }
    
    setBinaryData(words);
    setDecodedInstructions(instructions);
    
    // Calculate statistics
    const validInstructions = instructions.filter(i => i.isValid);
    const asciiWords = instructions.filter(i => i.mnemonic === 'ASCII');
    setStats({
      totalWords: words.length,
      totalBytes: uint8Data.length,
      validInstructions: validInstructions.length,
      dataWords: instructions.length - validInstructions.length,
      asciiWords: asciiWords.length,
      opcodeFrequency: opcodeFreq,
      uniqueOpcodes: Object.keys(opcodeFreq).length,
      isAligned: isAligned
    });
  };

  const formatHex = (value, digits) => {
    return '0x' + value.toString(16).toUpperCase().padStart(digits, '0');
  };

  const formatBinary = (value, bits) => {
    return value.toString(2).padStart(bits, '0');
  };

  const getInstructionColor = (mnemonic) => {
    if (mnemonic === 'DATA') return 'text-gray-500';
    if (mnemonic === 'ASCII') return 'text-pink-400';
    if (mnemonic === 'HALT') return 'text-red-400';
    if (['JUMP', 'JMP', 'JZE', 'JZ', 'JNZ', 'JGT', 'JLT', 'JOV', 'CALL', 'RET', 'BRU', 'RTI'].includes(mnemonic))
      return 'text-yellow-400';
    if (['LOAD', 'STORE', 'LDA', 'LDB', 'STA', 'STB', 'LDX', 'STX', 'MOVE'].includes(mnemonic))
      return 'text-blue-400';
    if (['ADD', 'SUB', 'MUL', 'DIV', 'MOD', 'ADC', 'SBC', 'INC', 'DEC'].includes(mnemonic))
      return 'text-green-400';
    if (['AND', 'OR', 'XOR', 'EOR', 'NEG', 'COM', 'NOT', 'SHL', 'SHR', 'ROL', 'ROR'].includes(mnemonic))
      return 'text-purple-400';
    if (['INPUT', 'IN', 'OUTPUT', 'OUT'].includes(mnemonic))
      return 'text-cyan-400';
    if (['PUSH', 'POP'].includes(mnemonic))
      return 'text-orange-400';
    if (['SKE', 'SKN', 'SKG', 'SKL', 'CMP'].includes(mnemonic))
      return 'text-teal-400';
    return 'text-gray-300';
  };

  const downloadDisassembly = () => {
    let output = '; BCC-500 Disassembly\n';
    output += `; Source: ${fileName}\n`;
    output += `; Total Words: ${stats.totalWords}\n`;
    output += `; Valid Instructions: ${stats.validInstructions}\n`;
    output += `; Generated by BCC-500 Binary Decoder\n\n`;
    
    decodedInstructions.forEach(instr => {
      const addrStr = formatHex(instr.address, 5);
      const wordStr = formatHex(instr.word, 6);
      
      if (instr.isValid) {
        if (instr.hasOperand) {
          output += `${addrStr}:  ${wordStr}  ${instr.mnemonic.padEnd(10)} ${formatHex(instr.operand, 5)}\n`;
        } else {
          output += `${addrStr}:  ${wordStr}  ${instr.mnemonic}\n`;
        }
      } else if (instr.mnemonic === 'ASCII') {
        output += `${addrStr}:  ${wordStr}  .BYTE      ; "${instr.asciiChars}"\n`;
      } else {
        output += `${addrStr}:  ${wordStr}  .WORD      ${formatHex(instr.word, 6)}\n`;
      }
    });
    
    const blob = new Blob([output], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = fileName ? fileName.replace(/\.[^.]+$/, '.asm') : 'bcc500_disassembly.asm';
    a.click();
    URL.revokeObjectURL(url);
  };

  const downloadBinary = () => {
    if (!binaryData) return;
    
    const buffer = new ArrayBuffer(binaryData.length * 3);
    const view = new DataView(buffer);
    
    binaryData.forEach((word, i) => {
      view.setUint8(i * 3, (word >> 16) & 0xFF);
      view.setUint8(i * 3 + 1, (word >> 8) & 0xFF);
      view.setUint8(i * 3 + 2, word & 0xFF);
    });
    
    const blob = new Blob([buffer], { type: 'application/octet-stream' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = fileName ? fileName.replace(/\.[^.]+$/, '.bin') : 'bcc500_program.bin';
    a.click();
    URL.revokeObjectURL(url);
  };

  if (!binaryData) {
    return (
      <div className="flex items-center justify-center min-h-screen bg-gray-900 text-white p-4">
        <div className="text-center max-w-2xl">
          <div className="bg-gradient-to-r from-blue-600 to-cyan-600 rounded-lg p-8 mb-6 shadow-lg">
            <Cpu className="w-20 h-20 text-white mx-auto mb-4" />
            <h1 className="text-4xl font-bold text-white mb-2">BCC-500 Binary Decoder</h1>
            <p className="text-blue-100">24-bit Instruction Disassembler</p>
          </div>
          
          <div className="bg-gray-800 rounded-lg p-8 border-2 border-dashed border-gray-600">
            <Upload className="w-16 h-16 text-gray-400 mx-auto mb-4" />
            <h2 className="text-xl font-bold mb-4">Upload BCC-500 Binary File</h2>
            <p className="text-gray-400 mb-6">
              Select a binary file from the BCC-500 system to decode and analyze
            </p>
            <label className="inline-block">
              <input
                type="file"
                onChange={handleFileUpload}
                className="hidden"
                accept="*"
              />
              <span className="bg-blue-600 hover:bg-blue-700 text-white px-8 py-3 rounded-lg font-medium cursor-pointer inline-flex items-center gap-2 transition-colors">
                <Upload className="w-5 h-5" />
                Choose File
              </span>
            </label>
          </div>

          <div className="mt-6 bg-gray-800 rounded-lg p-6 text-left">
            <h3 className="text-lg font-bold mb-3 text-white">Supported Features:</h3>
            <ul className="space-y-2 text-gray-300">
              <li className="flex items-center gap-2">
                <span className="text-green-400">✓</span> Decodes 24-bit BCC-500 instructions
              </li>
              <li className="flex items-center gap-2">
                <span className="text-green-400">✓</span> Supports all 52 opcodes (HALT, LOAD, STORE, ADD, etc.)
              </li>
              <li className="flex items-center gap-2">
                <span className="text-green-400">✓</span> Extracts 6-bit opcodes and 18-bit addresses
              </li>
              <li className="flex items-center gap-2">
                <span className="text-green-400">✓</span> Detects ASCII string data vs instructions
              </li>
              <li className="flex items-center gap-2">
                <span className="text-green-400">✓</span> Export to assembly format or clean binary
              </li>
              <li className="flex items-center gap-2">
                <span className="text-green-400">✓</span> Mnemonics match Assembler.h exactly
              </li>
            </ul>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gray-900 text-gray-100 p-4">
      <div className="max-w-7xl mx-auto">
        {/* Header */}
        <div className="bg-gradient-to-r from-blue-600 to-cyan-600 rounded-lg p-6 mb-6 shadow-lg">
          <div className="flex items-center justify-between flex-wrap gap-4">
            <div className="flex items-center gap-4">
              <Cpu className="w-12 h-12 text-white" />
              <div>
                <h1 className="text-3xl font-bold text-white">BCC-500 Binary Decoder</h1>
                <p className="text-blue-100">{fileName || '24-bit Instruction Disassembler'}</p>
              </div>
            </div>
            <div className="flex gap-2 flex-wrap">
              <label>
                <input
                  type="file"
                  onChange={handleFileUpload}
                  className="hidden"
                  accept="*"
                />
                <span className="flex items-center gap-2 bg-white text-blue-600 px-4 py-2 rounded-lg font-medium hover:bg-blue-50 transition-colors cursor-pointer">
                  <Upload className="w-4 h-4" />
                  Load New
                </span>
              </label>
              <button
                onClick={downloadDisassembly}
                className="flex items-center gap-2 bg-white text-blue-600 px-4 py-2 rounded-lg font-medium hover:bg-blue-50 transition-colors"
              >
                <Download className="w-4 h-4" />
                Export ASM
              </button>
              <button
                onClick={downloadBinary}
                className="flex items-center gap-2 bg-blue-500 text-white px-4 py-2 rounded-lg font-medium hover:bg-blue-400 transition-colors"
              >
                <Download className="w-4 h-4" />
                Export BIN
              </button>
            </div>
          </div>
        </div>

        {/* Statistics */}
        {stats && (
          <div className="grid grid-cols-1 md:grid-cols-5 gap-4 mb-6">
            <div className="bg-gray-800 rounded-lg p-4 border border-gray-700">
              <div className="flex items-center gap-2 mb-2">
                <HardDrive className="w-5 h-5 text-blue-400" />
                <span className="text-sm text-gray-400">Total Words</span>
              </div>
              <p className="text-2xl font-bold text-white">{stats.totalWords.toLocaleString()}</p>
              <p className="text-xs text-gray-500">{stats.totalBytes} bytes</p>
            </div>
            <div className="bg-gray-800 rounded-lg p-4 border border-gray-700">
              <div className="flex items-center gap-2 mb-2">
                <Zap className="w-5 h-5 text-green-400" />
                <span className="text-sm text-gray-400">Instructions</span>
              </div>
              <p className="text-2xl font-bold text-white">{stats.validInstructions.toLocaleString()}</p>
              <p className="text-xs text-gray-500">{((stats.validInstructions / stats.totalWords) * 100).toFixed(1)}%</p>
            </div>
            <div className="bg-gray-800 rounded-lg p-4 border border-gray-700">
              <div className="flex items-center gap-2 mb-2">
                <FileText className="w-5 h-5 text-purple-400" />
                <span className="text-sm text-gray-400">Data Words</span>
              </div>
              <p className="text-2xl font-bold text-white">{stats.dataWords.toLocaleString()}</p>
              <p className="text-xs text-gray-500">{stats.asciiWords} ASCII</p>
            </div>
            <div className="bg-gray-800 rounded-lg p-4 border border-gray-700">
              <div className="flex items-center gap-2 mb-2">
                <Code className="w-5 h-5 text-yellow-400" />
                <span className="text-sm text-gray-400">Unique Opcodes</span>
              </div>
              <p className="text-2xl font-bold text-white">{stats.uniqueOpcodes}</p>
              <p className="text-xs text-gray-500">of 52 possible</p>
            </div>
            <div className="bg-gray-800 rounded-lg p-4 border border-gray-700">
              <div className="flex items-center gap-2 mb-2">
                <HardDrive className={`w-5 h-5 ${stats.isAligned ? 'text-green-400' : 'text-yellow-400'}`} />
                <span className="text-sm text-gray-400">Alignment</span>
              </div>
              <p className="text-2xl font-bold text-white">{stats.isAligned ? 'OK' : 'Warn'}</p>
              <p className="text-xs text-gray-500">{stats.isAligned ? '3-byte aligned' : 'Not aligned'}</p>
            </div>
          </div>
        )}

        {/* Navigation */}
        <div className="flex gap-4 mb-6 flex-wrap">
          <button
            onClick={() => setView('instructions')}
            className={`px-6 py-3 rounded-lg font-medium transition-colors ${
              view === 'instructions'
                ? 'bg-blue-600 text-white'
                : 'bg-gray-800 text-gray-300 hover:bg-gray-700'
            }`}
          >
            <Code className="w-5 h-5 inline mr-2" />
            Disassembly
          </button>
          <button
            onClick={() => setView('opcodes')}
            className={`px-6 py-3 rounded-lg font-medium transition-colors ${
              view === 'opcodes'
                ? 'bg-blue-600 text-white'
                : 'bg-gray-800 text-gray-300 hover:bg-gray-700'
            }`}
          >
            <Zap className="w-5 h-5 inline mr-2" />
            Opcode Analysis
          </button>
        </div>

        {/* Content */}
        {view === 'instructions' && (
          <div className="bg-gray-800 rounded-lg shadow-lg overflow-hidden">
            <div className="overflow-x-auto">
              <div className="bg-gray-900 px-4 py-2 border-b border-gray-700">
                <div className="grid grid-cols-12 gap-4 text-xs font-mono text-gray-400 font-bold">
                  <div className="col-span-2">ADDRESS</div>
                  <div className="col-span-2">WORD</div>
                  <div className="col-span-1">OPCODE</div>
                  <div className="col-span-2">OPERAND</div>
                  <div className="col-span-2">MNEMONIC</div>
                  <div className="col-span-3">BINARY</div>
                </div>
              </div>
              <div className="max-h-[600px] overflow-y-auto">
                {decodedInstructions.slice(0, 500).map((instr, idx) => (
                  <div
                    key={idx}
                    className={`px-4 py-2 border-b border-gray-700 hover:bg-gray-700 transition-colors cursor-pointer ${
                      selectedAddr === instr.address ? 'bg-gray-700' : ''
                    }`}
                    onClick={() => setSelectedAddr(instr.address)}
                  >
                    <div className="grid grid-cols-12 gap-4 text-sm font-mono">
                      <div className="col-span-2 text-blue-400">{formatHex(instr.address, 5)}</div>
                      <div className="col-span-2 text-gray-300">{formatHex(instr.word, 6)}</div>
                      <div className="col-span-1 text-yellow-400">
                        {instr.opcode !== null ? formatHex(instr.opcode, 2) : '--'}
                      </div>
                      <div className="col-span-2 text-cyan-400">
                        {instr.operand !== null ? formatHex(instr.operand, 5) : 
                         instr.asciiChars ? `"${instr.asciiChars}"` : '-----'}
                      </div>
                      <div className={`col-span-2 font-bold ${getInstructionColor(instr.mnemonic)}`}>
                        {instr.mnemonic}
                      </div>
                      <div className="col-span-3 text-gray-500 text-xs">
                        {formatBinary(instr.word, 24).match(/.{1,8}/g).join(' ')}
                      </div>
                    </div>
                  </div>
                ))}
                {decodedInstructions.length > 500 && (
                  <div className="px-4 py-4 text-center text-gray-500">
                    ... and {decodedInstructions.length - 500} more instructions
                  </div>
                )}
              </div>
            </div>
          </div>
        )}

        {view === 'opcodes' && stats && (
          <div className="bg-gray-800 rounded-lg p-6 shadow-lg">
            <h3 className="text-xl font-bold mb-4 text-white">Opcode Frequency Analysis</h3>
            <div className="space-y-2">
              {Object.entries(stats.opcodeFrequency)
                .sort((a, b) => b[1] - a[1])
                .map(([opcode, count]) => {
                  const percentage = ((count / stats.validInstructions) * 100).toFixed(1);
                  return (
                    <div key={opcode} className="bg-gray-700 rounded-lg p-3">
                      <div className="flex items-center justify-between mb-1">
                        <span className={`font-mono font-bold ${getInstructionColor(opcode)}`}>
                          {opcode}
                        </span>
                        <div className="flex items-center gap-4">
                          <span className="text-gray-300">{count} times</span>
                          <span className="text-gray-400">{percentage}%</span>
                        </div>
                      </div>
                      <div className="w-full bg-gray-600 rounded-full h-2">
                        <div
                          className="bg-blue-500 h-2 rounded-full transition-all"
                          style={{ width: `${percentage}%` }}
                        />
                      </div>
                    </div>
                  );
                })}
            </div>
          </div>
        )}

        {/* Instruction Detail Panel */}
        {selectedAddr !== null && (
          <div className="mt-6 bg-gray-800 rounded-lg p-6 shadow-lg border-2 border-blue-500">
            <h3 className="text-lg font-bold mb-4 text-white flex items-center gap-2">
              <Play className="w-5 h-5" />
              Instruction Detail - Address {formatHex(selectedAddr, 5)}
            </h3>
            {(() => {
              const instr = decodedInstructions.find(i => i.address === selectedAddr);
              if (!instr) return null;
              
              return (
                <div className="grid grid-cols-2 gap-4">
                  <div className="bg-gray-700 rounded p-3">
                    <span className="text-gray-400 text-sm">Full Word:</span>
                    <p className="text-white font-mono text-lg">{formatHex(instr.word, 6)}</p>
                  </div>
                  <div className="bg-gray-700 rounded p-3">
                    <span className="text-gray-400 text-sm">Binary:</span>
                    <p className="text-white font-mono text-sm">
                      {formatBinary(instr.word, 24).match(/.{1,6}/g).join(' ')}
                    </p>
                  </div>
                  <div className="bg-gray-700 rounded p-3">
                    <span className="text-gray-400 text-sm">Opcode:</span>
                    <p className={`font-mono text-lg ${getInstructionColor(instr.mnemonic)}`}>
                      {instr.mnemonic} ({instr.opcode !== null ? formatHex(instr.opcode, 2) : 'N/A'})
                    </p>
                  </div>
                  <div className="bg-gray-700 rounded p-3">
                    <span className="text-gray-400 text-sm">Operand:</span>
                    <p className="text-white font-mono text-lg">
                      {instr.operand !== null ? formatHex(instr.operand, 5) : 
                       instr.asciiChars ? `"${instr.asciiChars}"` : 'N/A'}
                    </p>
                  </div>
                  <div className="bg-gray-700 rounded p-3">
                    <span className="text-gray-400 text-sm">File Offset:</span>
                    <p className="text-white font-mono text-lg">
                      {formatHex(instr.offset, 6)} ({instr.offset} bytes)
                    </p>
                  </div>
                  <div className="bg-gray-700 rounded p-3">
                    <span className="text-gray-400 text-sm">Raw Bytes:</span>
                    <p className="text-white font-mono">
                      [{instr.bytes.map(b => formatHex(b, 2)).join(', ')}]
                    </p>
                  </div>
                </div>
              );
            })()}
          </div>
        )}
      </div>
    </div>
  );
};

export default BCC500BinaryDecoder;
