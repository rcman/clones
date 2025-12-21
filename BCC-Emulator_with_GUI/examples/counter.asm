; Counter Program for BCC-500
; Counts down from 10 to 0 and displays each number

START:
    LOAD COUNT      ; Load counter value (10)

LOOP:
    ; Convert number to ASCII and display
    ADD ASCII_ZERO  ; Add '0' to convert to ASCII digit
    OUTPUT 0x3F031  ; Output to terminal

    ; Output a space
    LOAD SPACE
    OUTPUT 0x3F031

    ; Decrement counter
    LOAD COUNT
    DEC
    STORE COUNT

    ; Check if zero
    JNZ LOOP        ; If not zero, continue loop

    ; Output newline
    LOAD NEWLINE
    OUTPUT 0x3F031

    HALT

; Data
COUNT:      .WORD 10
ASCII_ZERO: .WORD 48    ; ASCII '0'
SPACE:      .WORD 32    ; Space character
NEWLINE:    .WORD 10    ; Newline
