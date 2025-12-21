; Fibonacci Sequence Generator for BCC-500
; Calculates and displays first 10 Fibonacci numbers

START:
    LOAD ZERO       ; F(0) = 0
    STORE FIB_A

    LOAD ONE        ; F(1) = 1
    STORE FIB_B

    LOAD TEN        ; Counter = 10
    STORE COUNTER

LOOP:
    ; Display current Fibonacci number (FIB_A)
    LOAD FIB_A
    ; Simple display - just output the low byte
    OUTPUT 0x3F031

    LOAD SPACE
    OUTPUT 0x3F031

    ; Calculate next: FIB_C = FIB_A + FIB_B
    LOAD FIB_A
    ADD  FIB_B
    STORE FIB_C

    ; Shift values: FIB_A = FIB_B, FIB_B = FIB_C
    LOAD FIB_B
    STORE FIB_A

    LOAD FIB_C
    STORE FIB_B

    ; Decrement counter
    LOAD COUNTER
    DEC
    STORE COUNTER

    ; Continue if not zero
    JNZ LOOP

    ; Output newline and halt
    LOAD NEWLINE
    OUTPUT 0x3F031

    HALT

; Data
ZERO:    .WORD 0
ONE:     .WORD 1
TEN:     .WORD 10
FIB_A:   .WORD 0
FIB_B:   .WORD 0
FIB_C:   .WORD 0
COUNTER: .WORD 0
SPACE:   .WORD 32
NEWLINE: .WORD 10
