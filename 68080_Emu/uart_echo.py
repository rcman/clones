# uart_echo.py
# Reads from UART and echoes back
data = [
    # Load UART base address into A0: MOVEA.L #0xF4000000, A0
    0x20, 0x7C, 0xF4, 0x00, 0x00, 0x00,
    
    # Loop: Check if data available (bit 0 of status register)
    # BTST #0, 5(A0)  - Check LSR bit 0
    0x08, 0x28, 0x00, 0x00, 0x00, 0x05,
    
    # BEQ loop (if zero, no data, loop back)
    0x67, 0xF8,
    
    # Read byte: MOVE.B (A0), D0
    0x10, 0x10,
    
    # Write byte back: MOVE.B D0, (A0)
    0x10, 0x80,
    
    # Loop back
    0x60, 0xF0,
]

with open('uart_echo.bin', 'wb') as f:
    f.write(bytes(data))
