#!/usr/bin/env python3
# m68k_assembler.py - Simple 68000 binary creator

class M68KBinary:
    def __init__(self):
        self.data = bytearray()
    
    def word(self, value):
        """Add a 16-bit word"""
        self.data.extend([(value >> 8) & 0xFF, value & 0xFF])
        return self
    
    def long(self, value):
        """Add a 32-bit long"""
        self.data.extend([
            (value >> 24) & 0xFF,
            (value >> 16) & 0xFF,
            (value >> 8) & 0xFF,
            value & 0xFF
        ])
        return self
    
    def nop(self):
        return self.word(0x4E71)
    
    def stop(self):
        return self.word(0x4E72).word(0x2700)
    
    def move_l_imm(self, value, dn):
        """MOVE.L #value, Dn"""
        return self.word(0x203C | (dn << 9)).long(value)
    
    def add_l(self, src_dn, dst_dn):
        """ADD.L Dn, Dn"""
        return self.word(0xD080 | (dst_dn << 9) | src_dn)
    
    def sub_l(self, src_dn, dst_dn):
        """SUB.L Dn, Dn"""
        return self.word(0x9080 | (dst_dn << 9) | src_dn)
    
    def save(self, filename):
        with open(filename, 'wb') as f:
            f.write(self.data)
        print(f"Created {filename} ({len(self.data)} bytes)")

# Example usage:
prog = M68KBinary()
prog.move_l_imm(0x12345678, 0)  # D0 = 0x12345678
prog.move_l_imm(0x11111111, 1)  # D1 = 0x11111111
prog.add_l(1, 0)                # D0 = D0 + D1
prog.nop()
prog.stop()
prog.save('example.bin')

# Create a factorial calculator
fact = M68KBinary()
fact.move_l_imm(5, 0)           # D0 = 5 (input)
fact.move_l_imm(1, 1)           # D1 = 1 (result)
# Loop: MULU D0, D1
fact.word(0xC2C0)               # MULU D0, D1
# SUBQ.L #1, D0
fact.word(0x5380)
# BNE loop
fact.word(0x66FC)
fact.stop()
fact.save('factorial.bin')
