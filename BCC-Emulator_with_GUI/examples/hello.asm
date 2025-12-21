; Hello World Program for BCC-500
; Prints "Hello, World!" to the terminal display

START:
    LOAD MSG        ; Load first character address
    STORE PTR       ; Store in pointer

LOOP:
    LOAD PTR        ; Load current pointer
    LDX  0          ; Clear X register
    ; Load character indirectly (using the address stored at PTR)
    LDA  [PTR]      ; This would need indirect addressing
    ; Simplified version - direct character loading

; Direct approach - load each character and output
    LOAD CHAR_H
    OUTPUT 0x3F031  ; Terminal data register

    LOAD CHAR_E
    OUTPUT 0x3F031

    LOAD CHAR_L
    OUTPUT 0x3F031

    LOAD CHAR_L
    OUTPUT 0x3F031

    LOAD CHAR_O
    OUTPUT 0x3F031

    LOAD CHAR_COMMA
    OUTPUT 0x3F031

    LOAD CHAR_SPACE
    OUTPUT 0x3F031

    LOAD CHAR_W
    OUTPUT 0x3F031

    LOAD CHAR_O
    OUTPUT 0x3F031

    LOAD CHAR_R
    OUTPUT 0x3F031

    LOAD CHAR_L
    OUTPUT 0x3F031

    LOAD CHAR_D
    OUTPUT 0x3F031

    LOAD CHAR_EXCLAIM
    OUTPUT 0x3F031

    LOAD CHAR_NEWLINE
    OUTPUT 0x3F031

    HALT

; Character data
CHAR_H:      .BYTE 'H'
CHAR_E:      .BYTE 'e'
CHAR_L:      .BYTE 'l'
CHAR_O:      .BYTE 'o'
CHAR_COMMA:  .BYTE ','
CHAR_SPACE:  .BYTE ' '
CHAR_W:      .BYTE 'W'
CHAR_R:      .BYTE 'r'
CHAR_D:      .BYTE 'd'
CHAR_EXCLAIM: .BYTE '!'
CHAR_NEWLINE: .BYTE 10

MSG:  .WORD CHAR_H
PTR:  .WORD 0
