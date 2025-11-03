# loop_program.py
data = [
    # Initialize counter: MOVE.W #10, D0
    0x30, 0x3C, 0x00, 0x0A,
    
    # Loop start at offset 4:
    # SUBQ.W #1, D0
    0x51, 0x40,
    
    # BNE (branch back if not zero) - offset -4
    0x66, 0xFC,
    
    # Exit: STOP
    0x4E, 0x72, 0x27, 0x00
]

with open('loop.bin', 'wb') as f:
    f.write(bytes(data))
