; Multiplication by Repeated Addition
; Calculates 7 * 6 = 42

START:
    LOAD ZERO       ; Initialize result to 0
    STORE RESULT

    LOAD NUM2       ; Load counter (6)
    STORE COUNTER

MULT_LOOP:
    LOAD RESULT     ; Load current result
    ADD  NUM1       ; Add 7
    STORE RESULT    ; Store back

    LOAD COUNTER    ; Decrement counter
    DEC
    STORE COUNTER

    JNZ MULT_LOOP   ; Continue if not zero

    ; Result is now 42 - display it
    LOAD RESULT
    OUTPUT 0x3F031  ; Output to terminal

    LOAD NEWLINE
    OUTPUT 0x3F031

    HALT

; Data
ZERO:    .WORD 0
NUM1:    .WORD 7
NUM2:    .WORD 6
RESULT:  .WORD 0
COUNTER: .WORD 0
NEWLINE: .WORD 10
