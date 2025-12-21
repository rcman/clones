; Stack Operations Test
; Tests PUSH, POP, CALL, and RET instructions

START:
    LOAD VAL1
    PUSH            ; Push 100 onto stack

    LOAD VAL2
    PUSH            ; Push 200 onto stack

    LOAD VAL3
    PUSH            ; Push 300 onto stack

    ; Call subroutine
    CALL DISPLAY_SUM

    HALT

; Subroutine: Pop three values and display their sum
DISPLAY_SUM:
    POP             ; Pop 300 into ACC
    STORE TEMP1

    POP             ; Pop 200 into ACC
    STORE TEMP2

    POP             ; Pop 100 into ACC
    ADD  TEMP1      ; Add 300
    ADD  TEMP2      ; Add 200
    ; ACC now contains 600

    OUTPUT 0x3F031  ; Display result

    LOAD NEWLINE
    OUTPUT 0x3F031

    RET             ; Return to caller

; Data
VAL1:    .WORD 100
VAL2:    .WORD 200
VAL3:    .WORD 300
TEMP1:   .WORD 0
TEMP2:   .WORD 0
NEWLINE: .WORD 10
