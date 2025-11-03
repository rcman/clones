# fpu_test.py
data = [
    # This would require setting up FPU ops
    # Load value into FP0, calculate sin, store result
    0xF2, 0x00, 0x0E, 0x00,  # FSIN FP0
    0x4E, 0x72, 0x27, 0x00   # STOP
]

with open('fpu_test.bin', 'wb') as f:
    f.write(bytes(data))
