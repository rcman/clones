# PDP-11 Emulator

A high-performance PDP-11 emulator designed to run as a headless Linux service with telnet access. This emulator can run at full native speed (not cycle-accurate) and is suitable for running Unix systems like 2.11BSD.

## Features

- **Full-speed emulation**: Runs as fast as the host CPU allows
- **Telnet server**: Connect remotely to interact with the PDP-11
- **Disk support**: RK05 disk controller emulation
- **Serial terminal**: DL11 serial interface for console I/O
- **Multiple users**: Supports up to 8 simultaneous telnet connections
- **Headless operation**: No GUI, perfect for running as a service
- **Compatible**: Designed to run 2.11BSD Unix with C compiler and networking

## Architecture Emulated

- **CPU**: PDP-11/70 architecture
- **Memory**: Up to 4MB RAM (configurable)
- **Disk**: RK05 disk controller
- **Serial**: DL11 serial interface (console)
- **Addressing modes**: All 8 PDP-11 addressing modes implemented

## Building

```bash
make
```

This will create the `pdp11emu` executable.

## Installation

```bash
sudo make install
```

This installs the emulator to `/usr/local/bin/`.

## Usage

### Basic Usage

```bash
./pdp11emu --port 2311 --disk disk.img --memory 256
```

### Command Line Options

- `--port PORT` : Telnet server port (default: 2311)
- `--disk FILE` : Path to disk image file (default: disk.img)
- `--memory SIZE` : Memory size in KB (default: 256)
- `--bootstrap` : Load simple echo test program
- `--help` : Show help message

### Connecting

Once the emulator is running, connect via telnet:

```bash
telnet localhost 2311
```

## Testing the Emulator

### Bootstrap Test

To verify the emulator is working, run with the bootstrap program:

```bash
./pdp11emu --bootstrap
```

Then connect via telnet. The bootstrap program echoes back any characters you type, demonstrating that the CPU, serial interface, and telnet server are all working correctly.

## Running Unix

### Preparing a 2.11BSD Disk Image

1. Obtain a 2.11BSD disk image (search for pre-built images or create using SIMH)
2. Convert to raw format if necessary
3. Run the emulator with the disk image:

```bash
./pdp11emu --disk 2.11bsd.img --memory 4096
```

### Expected Features with 2.11BSD

Once 2.11BSD is running, you'll have:
- Full Unix environment
- C compiler (`cc`)
- Text editors (`vi`, `ed`)
- Shell utilities
- Multi-user support
- FTP server capability
- Web server capability (with appropriate software)

## Running as a Service

### Systemd Service File

Create `/etc/systemd/system/pdp11emu.service`:

```ini
[Unit]
Description=PDP-11 Emulator
After=network.target

[Service]
Type=simple
User=pdp11
Group=pdp11
WorkingDirectory=/var/lib/pdp11
ExecStart=/usr/local/bin/pdp11emu --disk /var/lib/pdp11/disk.img --memory 4096 --port 2311
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### Setup Steps

```bash
# Create user and directory
sudo useradd -r -s /bin/false pdp11
sudo mkdir -p /var/lib/pdp11
sudo chown pdp11:pdp11 /var/lib/pdp11

# Copy disk image
sudo cp disk.img /var/lib/pdp11/
sudo chown pdp11:pdp11 /var/lib/pdp11/disk.img

# Enable and start service
sudo systemctl enable pdp11emu
sudo systemctl start pdp11emu

# Check status
sudo systemctl status pdp11emu

# View logs
sudo journalctl -u pdp11emu -f
```

## Architecture Details

### CPU Implementation

The emulator implements:
- 8 general-purpose registers (R0-R7)
- Program Counter (R7)
- Stack Pointer (R6)
- Processor Status Word with condition codes (N, Z, V, C)
- All standard PDP-11 addressing modes
- Double-operand instructions (MOV, CMP, BIT, BIC, BIS, ADD)
- Single-operand instructions (CLR, etc.)
- Branch instructions
- Memory-mapped I/O

### Memory Map

```
0000000 - Main memory (size configurable)
...
0177400 - Disk controller registers
0177560 - Serial console registers
```

### I/O Devices

#### Serial Console (DL11)
- RCSR (177560): Receiver Control/Status
- RBUF (177562): Receiver Buffer
- XCSR (177564): Transmitter Control/Status  
- XBUF (177566): Transmitter Buffer

#### Disk Controller (RK05)
- RKDS (177400): Drive Status
- RKER (177402): Error Register
- RKCS (177404): Control/Status
- RKWC (177406): Word Count
- RKBA (177410): Bus Address
- RKDA (177412): Disk Address

## Current Limitations

This is a functional base implementation. The following are not yet implemented:

- Memory Management Unit (MMU) - needed for 2.11BSD
- Split I/D space - needed for larger programs
- Additional instructions (some less common opcodes)
- Network interface (DEUNA/DELUA) - needed for TCP/IP
- Additional disk drives
- Tape controller
- Real-time clock
- Interrupts (partially implemented)

## Roadmap

### Phase 1 (Current)
- [x] Basic CPU core
- [x] Serial console
- [x] Disk controller
- [x] Telnet server
- [x] Bootstrap loader

### Phase 2 (Next)
- [ ] Complete instruction set
- [ ] MMU implementation
- [ ] Split I/D space
- [ ] Interrupt system
- [ ] Boot 2.11BSD

### Phase 3 (Future)
- [ ] Network interface
- [ ] Multiple disk drives
- [ ] Tape controller
- [ ] Real-time clock
- [ ] Performance optimizations

## Performance

Since this emulator runs at native speed without cycle-accurate timing:
- Instructions execute as fast as possible
- Typical modern CPU can execute millions of PDP-11 instructions per second
- Compiling C code will be nearly instantaneous
- The bottleneck will be I/O, not CPU speed

## Troubleshooting

### Emulator won't start
- Check if port is already in use: `netstat -ln | grep 2311`
- Try a different port: `--port 2312`

### Can't connect via telnet
- Check firewall: `sudo ufw allow 2311`
- Verify emulator is running: `ps aux | grep pdp11emu`

### Disk errors
- Ensure disk image file exists and has correct permissions
- Check disk image size matches RK05 geometry

## Development

### Adding Instructions

To add a new instruction:
1. Decode the opcode in `pdp11_step()` in `pdp11.c`
2. Implement the instruction behavior
3. Update condition codes appropriately
4. Test with sample code

### Adding I/O Devices

To add a new device:
1. Create header and implementation files (e.g., `device.h`, `device.c`)
2. Add register addresses and definitions
3. Implement register read/write functions
4. Hook into `io_read_word()` and `io_write_word()` in `main.c`
5. Add service function to main loop

## References

- PDP-11 Architecture Handbook
- 2.11BSD Documentation
- SIMH PDP-11 Simulator
- Unix v7 Documentation

## License

This is an educational/reference implementation. Use at your own risk.

## Contributing

This is a starting point for a full PDP-11 emulator. Areas that need work:
- Complete instruction set implementation
- MMU implementation
- Network interface
- More robust error handling
- Better documentation of instruction implementation

## Author

Built as a foundation for running 2.11BSD Unix with full C compilation and networking capabilities.
