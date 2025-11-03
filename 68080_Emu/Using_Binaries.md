7. Using the Binary Files
Once you have a .bin file:

Start the simulator:

bash   java SBCGUI

Load the binary:

Click File → Load Binary...
Select your .bin file
Enter load address: 1000 (hex)
Click OK


Set PC to start:

The program will be loaded at 0x1000
Reset the system (PC will be set from reset vector)
Or manually set PC using the reset vectors


Run:

Click ▶ Run or Step to execute



8. Quick Test - Create All Examples
bash# Save this as create_all_tests.py
python3 create_binary.py
python3 loop_program.py
python3 m68k_assembler.py
Then you'll have:

test.bin - Simple arithmetic
loop.bin - Loop counter
example.bin - Using the assembler tool
factorial.bin - Factorial calculator

9. Inspect Binary Files
To see what's in a binary file:
bash# On Linux/Mac:
hexdump -C test.bin

# Or:
od -Ax -tx1 test.bin
Would you like me to create a specific type of program for you to test? Or help you understand the 68000 instruction encoding better?
