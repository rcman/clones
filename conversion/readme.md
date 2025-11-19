Key Algorithms Converted:

Bit Transition Detection - XOR-based detection of bit changes with sine table lookups
Phase Calculation - Signed 8-bit multiplication with shift operations
I/Q Demodulation - Quadrature demodulation with 4-group averaging
M-Value Calculation - Signal strength metric for bit decisions
Message Reception - Bit decision logic using timing and comparisons
BCH Checksum - 6-bit error detection algorithm

The C code preserves the original 8-bit integer math and bit manipulation techniques used in the assembly. The algorithms are designed to extract digital data from noisy power line carrier signals using only simple arithmetic operations - no floating point needed!
The demodulation technique is quite clever - it uses four parallel groups to maintain running averages and calculates both in-phase (I) and quadrature (Q) components to robustly detect phase shifts in the carrier signal.
