Key points about them:

Drop-in replacements: They were designed to be pin-compatible with Intel's 8088 (V20) and 8086 (V30) processors, meaning you could literally pull out your Intel chip and pop in the NEC equivalent without any other modifications.
Performance boost: They ran about 10-30% faster than the Intel chips they replaced at the same clock speed, thanks to improved microcode and architecture. This made them popular upgrades for IBM PCs and compatibles.
Additional features: They included an 8080 emulation mode, allowing them to run older CP/M software, and had a few extra instructions beyond the standard 8086 instruction set.
Legal battles: Intel sued NEC over these chips, claiming copyright infringement of their microcode. NEC ultimately won the case in 1992, establishing an important precedent that microcode couldn't be copyrighted in the same way as software.
The V30 specifically was the 16-bit data bus version (like the 8086), while the V20 had an 8-bit external data bus (like the 8088).

These chips were quite popular in the late 80s PC clone market as an affordable performance upgrade. Are you working on a retro computing project, or just curious about computer history?

Any standard x86 emulator should work fine for V20/V30 systems. Here's why:
Compatibility: The V20/V30 were designed to be software-compatible with the Intel 8088/8086, so they run the same software. For the vast majority of programs, there's no difference between running on Intel or NEC chips.
Popular emulators that work:

DOSBox - Great for running DOS games and applications
PCem or 86Box - More accurate hardware emulation of entire PC systems from that era
QEMU - Can emulate various x86 systems
MAME - Emulates some specific arcade and computer systems that used these chips

The catch: Most emulators don't specifically emulate the V20/V30's unique features like:

The exact timing differences that gave them their speed advantage
The 8080 emulation mode
The few extra instructions NEC added

So if you're running typical DOS software or games, any x86 emulator will work perfectly. But if you wanted to test software that specifically relied on V20/V30 unique features (which was rare), you'd need a more specialized emulator. 86Box actually does have some V20/V30-specific emulation options if you want that level of accuracy.
For most retro computing purposes though, DOSBox or 86Box set to emulate a generic 8088/8086 PC will give you the experience you're looking for.

