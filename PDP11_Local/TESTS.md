# Sample PDP-11 Test Programs

## Test Program 1: Simple Loop (Octal)

This program counts from 0 to 10 in R0:

```
        .=1000                  ; Origin at octal 1000
start:  mov     #0,r0          ; 012700 000000
loop:   cmp     #10,r0         ; 020027 000012
        beq     done           ; 001401
        mov     #1,r1          ; 012701 000001
        add     r1,r0          ; 060001
        br      loop           ; 000773
done:   halt                   ; 000000
```

Assembled in octal:
```
012700 000000    ; MOV #0, R0
020027 000012    ; CMP #10., R0
001401            ; BEQ done
012701 000001    ; MOV #1, R1
060001            ; ADD R1, R0
000773            ; BR loop
000000            ; HALT
```

## Test Program 2: Serial Echo (Bootstrap)

This is the bootstrap program included in the emulator:

```assembly
        .=1000                          ; Origin
        mov     #177560,r0              ; RCSR address
        mov     #177562,r1              ; RBUF address
        mov     #177564,r2              ; XCSR address
        mov     #177566,r3              ; XBUF address
loop:
        tstb    (r0)                    ; Test RCSR
        beq     loop                    ; Wait for character
        movb    (r1),r4                 ; Read character
wait:
        tstb    (r2)                    ; Test XCSR
        beq     wait                    ; Wait until ready
        movb    r4,(r3)                 ; Output character
        br      loop                    ; Repeat
```

## Test Program 3: Memory Test

Tests basic memory operations:

```assembly
        .=1000
        mov     #2000,r0                ; Test address
        mov     #52525,r1               ; Test pattern
        mov     r1,(r0)                 ; Write to memory
        cmp     r1,(r0)                 ; Compare
        bne     fail                    ; Branch if not equal
        clr     (r0)                    ; Clear location
        tst     (r0)                    ; Test if zero
        bne     fail                    ; Should be zero
        halt                            ; Success
fail:   mov     #1,r2                   ; Error code
        halt
```

## Test Program 4: Stack Operations

Tests push/pop and subroutine calls:

```assembly
        .=1000
        mov     #2000,sp                ; Set up stack
        mov     #123,r0                 ; Test value
        mov     r0,-(sp)                ; Push onto stack
        clr     r0                      ; Clear register
        mov     (sp)+,r0                ; Pop from stack
        cmp     #123,r0                 ; Should be same
        bne     error
        halt
error:  halt
```

## Loading Test Programs

To load a test program into the emulator, you can:

1. **Modify the bootstrap in main.c:**
   Replace the bootstrap array with your test program

2. **Create a disk image:**
   Use a tool to write your program to a disk image

3. **Add a loader function:**
   Add a function to load from a file:
   ```c
   void load_program(pdp11_t *cpu, const char *filename) {
       FILE *f = fopen(filename, "rb");
       if (f) {
           uint16_t addr = 01000;  // Start address
           uint16_t word;
           while (fread(&word, sizeof(word), 1, f) == 1) {
               mem_write_word(cpu, addr, word);
               addr += 2;
           }
           fclose(f);
       }
   }
   ```

## Debugging Test Programs

Add this to `pdp11_step()` for instruction tracing:

```c
static int debug = 1;  // Set to 1 for debug output

int pdp11_step(pdp11_t *cpu) {
    // ... existing code ...
    
    if (debug) {
        printf("PC=%06o R0=%06o R1=%06o INSTR=%06o PSW=%06o\n",
               cpu->regs[REG_PC], cpu->regs[0], cpu->regs[1], 
               instr, cpu->psw);
    }
    
    // ... rest of function ...
}
```

## Expected Behavior

### Loop Test:
- R0 should increment from 0 to 10
- Should halt with R0=10

### Serial Echo:
- Connect via telnet
- Type characters
- They should echo back

### Memory Test:
- Should halt with R2=0 (success)
- If R2=1, memory operations failed

### Stack Test:
- Should halt successfully
- If fails, stack operations are broken

## Common Issues

1. **Wrong condition codes:**
   - Check N, Z, V, C flags after each operation
   - Compare with PDP-11 handbook

2. **Addressing mode errors:**
   - Double-check autoincrement/autodecrement
   - PC should always increment by 2

3. **Byte vs word:**
   - Byte operations only affect low 8 bits
   - Word operations affect all 16 bits

4. **Octal notation:**
   - PDP-11 uses octal everywhere!
   - 177560 is octal, not decimal
   - = 0x1F70 in hex = 8048 in decimal

## Writing Your Own Tests

Start simple:
1. Test individual instructions
2. Test addressing modes
3. Test condition codes
4. Test branches
5. Combine into more complex programs

Remember: The PDP-11 uses octal (base 8) for everything in documentation!
