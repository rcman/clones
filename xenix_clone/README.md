# Pico Xenix Emulator

A faithful recreation of the **Xenix System V** operating system experience for the Raspberry Pi Pico. This emulator brings the 1980s SCO/Microsoft Xenix terminal environment to life on modern hardware.

![Platform](https://img.shields.io/badge/platform-Raspberry%20Pi%20Pico-red)
![Python](https://img.shields.io/badge/python-3.x%20%7C%20MicroPython-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## What is Xenix?

Xenix was Microsoft's version of Unix, licensed from AT&T and developed in partnership with The Santa Cruz Operation (SCO). It was the most widely used Unix variant in the mid-to-late 1980s, running on microcomputers based on Intel 8086/80286 processors.

This emulator recreates the authentic look, feel, and behavior of **Xenix System V Release 2.3.2** from the 1980s.

## Features

### Authentic 1980s Experience

- **Period-accurate boot sequence** with SCO/Microsoft/AT&T copyright notices
- **Traditional Unix shell prompt** (`#` for root, `$` for users)
- **Authentic MOTD** with RESTRICTED RIGHTS LEGEND
- **Xenix-style command output** matching original formatting
- **Classic Unix commands** including banner, wall, write, and mesg

### Virtual Filesystem

- In-memory hierarchical filesystem with Unix-style metadata
- Standard Xenix directory structure (`/bin`, `/etc`, `/usr`, `/tmp`, `/home`, `/dev`)
- Persistent storage via JSON (saves to `xenix_state.json`)
- Full support for file operations (create, read, write, delete, copy, move)

### Text Editors

- **vi** - Simplified modal editor with basic commands
- **ed** - Classic line editor from original Unix

### System Commands

Full suite of authentic Xenix commands with period-accurate output formats.

## Installation

### For Raspberry Pi Pico (MicroPython)

1. Install MicroPython on your Raspberry Pi Pico:
   - Download the latest MicroPython firmware from [micropython.org](https://micropython.org/download/rp2-pico/)
   - Hold BOOTSEL button while connecting Pico to USB
   - Copy the `.uf2` file to the RPI-RP2 drive

2. Copy `pico_xenix.py` to your Pico:
   ```bash
   # Using mpremote (recommended)
   mpremote cp pico_xenix.py :main.py

   # Or using ampy
   ampy --port /dev/ttyACM0 put pico_xenix.py main.py
   ```

3. Connect to the Pico via serial terminal:
   ```bash
   screen /dev/ttyACM0 115200
   # or
   mpremote connect /dev/ttyACM0
   # or
   picocom /dev/ttyACM0 -b 115200
   ```

4. Reset the Pico - Xenix will boot automatically!

### For Desktop Python (Testing/Development)

1. Ensure Python 3.x is installed:
   ```bash
   python3 --version
   ```

2. Run the emulator:
   ```bash
   python3 pico_xenix.py
   ```

## Usage

### Boot Sequence

When you start the emulator, you'll see an authentic Xenix boot sequence:

```
The XENIX System


Copyright (c) 1984, 1985, 1986, 1987, 1988
The Santa Cruz Operation, Inc.
Microsoft Corporation
AT&T
All Rights Reserved


login: root

                     RESTRICTED RIGHTS LEGEND

Use, duplication, or disclosure is subject to restrictions as set forth
in subparagraph (c)(1)(ii) of the Rights in Technical Data and Computer
Software clause at DFARS 52.227-7013.

Microsoft Corporation
One Microsoft Way
Redmond, Washington  98052-6399

Last login: Sat Nov 15 12:48 on tty01

#
```

### Available Commands

#### File Operations
| Command | Description | Example |
|---------|-------------|---------|
| `ls [-l] [-a]` | List directory contents | `ls -l` |
| `cd <path>` | Change directory | `cd /usr/bin` |
| `pwd` | Print working directory | `pwd` |
| `mkdir <name>` | Create directory | `mkdir mydir` |
| `rmdir <name>` | Remove empty directory | `rmdir mydir` |
| `touch <file>` | Create empty file | `touch test.txt` |
| `rm <file>` | Remove file | `rm test.txt` |
| `cp <src> <dest>` | Copy file | `cp file1 file2` |
| `mv <src> <dest>` | Move/rename file | `mv old new` |
| `cat <file>` | Display file contents | `cat /etc/motd` |
| `echo <text>` | Print text (supports `> file`) | `echo "hello" > test.txt` |
| `find [pattern]` | Find files matching pattern | `find motd` |

#### Text Processing
| Command | Description | Example |
|---------|-------------|---------|
| `grep <pattern> <file>` | Search for pattern in file | `grep "error" logfile` |
| `vi <file>` | Visual editor | `vi myfile.txt` |
| `ed <file>` | Line editor | `ed myfile.txt` |

#### System Commands
| Command | Description | Example |
|---------|-------------|---------|
| `ps` | Show processes (Xenix format) | `ps` |
| `who` | Show logged-in users | `who` |
| `date` | Display current date/time | `date` |
| `uname [-a] [-s]` | System information | `uname -a` |
| `df` | Disk space (blocks/i-nodes) | `df` |
| `free` | Memory usage | `free` |
| `clear` | Clear screen | `clear` |
| `banner <text>` | ASCII art banner (max 10 chars) | `banner XENIX` |
| `help` | Show available commands | `help` |

#### Communication
| Command | Description | Example |
|---------|-------------|---------|
| `write <user>` | Send message to user | `write admin` |
| `wall [message]` | Broadcast message to all | `wall Maintenance in 5 min` |
| `mesg [y\|n]` | Control terminal write access | `mesg n` |

#### Exit
| Command | Description |
|---------|-------------|
| `exit` or `logout` | Save filesystem and exit |

### Using the vi Editor

The vi editor is a simplified modal editor with basic commands:

```bash
# vi myfile.txt

Commands:
  i         - Enter insert mode
  ESC       - Exit insert mode (back to command mode)
  :w        - Write (save) file
  :q        - Quit
  :wq       - Write and quit
  n         - Next line
  p         - Previous line
```

### Using the ed Line Editor

The ed editor is a classic line-oriented editor:

```bash
# ed myfile.txt

Commands:
  a         - Append text (type . on new line to finish)
  p         - Print buffer contents
  w         - Write file
  q         - Quit
```

### Keyboard Shortcuts

While at the shell prompt, you can use:

- **Backspace** or **Delete** - Delete previous character
- **Ctrl+C** - Cancel current line
- **Ctrl+D** - Logout (if line is empty)
- **Ctrl+U** - Kill (erase) entire line

## Examples

### Basic File Management

```bash
# List files
ls -l

# Create a directory structure
mkdir projects
cd projects
mkdir src docs
ls

# Create and edit a file
echo "Hello Xenix" > greeting.txt
cat greeting.txt

# Copy and organize
cp greeting.txt backup.txt
mv greeting.txt ../greeting_old.txt
```

### Using vi to Create a Document

```bash
# vi welcome.txt
# Press 'i' to enter insert mode
# Type your text
# Press ESC to exit insert mode
# Type ':wq' to save and quit
```

### System Information

```bash
# Check system details
uname -a
# Output: XENIX pico 2.3.2 i286 2 i8086

# View processes
ps
# Output:
#   PID  TTY TIME COMMAND
#     1  01  0:01 /etc/init
#    15  01  0:00 -sh

# Check who's logged in
who
# Output: root     tty01    Nov 15 12:48

# Check disk space
df
# Output:
# /            (/dev/root ):    12345 blocks     1234 i-nodes
# /tmp         (/dev/tmp  ):     5678 blocks      567 i-nodes
```

### Creating ASCII Art Banners

```bash
banner XENIX
# Output:
# ###  ###  ###  ###  ###
```

### Broadcasting Messages

```bash
wall System will reboot in 5 minutes
# Output:
# Broadcast message from root@pico (tty01)...
# System will reboot in 5 minutes
```

## File Persistence

The emulator automatically saves the filesystem state to `xenix_state.json` when you exit:

```bash
exit
# Output:
# Saving filesystem state...
# Filesystem saved.
#
# XENIX System V/286
# logout
```

On next boot, your files and directories are automatically restored!

## Architecture

The emulator consists of three main components:

### 1. FileSystem Class
- Implements hierarchical filesystem in memory
- Unix-style metadata (permissions, owner, timestamps)
- JSON serialization for persistence

### 2. SerialTerminal Class
- Handles terminal I/O with VT100 control codes
- Character-by-character input with local echo
- Control character processing (Ctrl+C, Ctrl+D, Ctrl+U, backspace)

### 3. XenixOS Class
- Main shell with command parser
- Authentic Xenix boot sequence
- Full command implementation

## Platform Compatibility

### Raspberry Pi Pico (MicroPython)
- Designed for UART/serial communication
- Uses `gc.mem_free()` for memory stats
- Optimized for limited resources

### Desktop Python 3.x
- Uses `sys.stdin`/`sys.stdout`
- Great for testing and development
- Full feature compatibility

## Hardware Requirements

### Raspberry Pi Pico
- Raspberry Pi Pico or Pico W
- MicroPython firmware
- USB connection for serial terminal

### Desktop
- Python 3.6 or higher
- Any operating system (Linux, macOS, Windows)

## Troubleshooting

### Pico doesn't boot into Xenix
- Make sure you renamed `pico_xenix.py` to `main.py` on the Pico
- Verify MicroPython is properly installed
- Try resetting the Pico (disconnect/reconnect USB)

### Keyboard input not working
- Ensure your terminal emulator is in raw mode
- For screen: Use `screen /dev/ttyACM0 115200`
- For picocom: Use `picocom /dev/ttyACM0 -b 115200`

### Filesystem not saving
- Ensure you use `exit` or `logout` command (don't just disconnect)
- On Pico, check available flash storage
- On desktop, verify write permissions in current directory

### Characters appear doubled
- Your terminal might have local echo enabled
- Try disabling local echo in your terminal settings

## Historical Note

This emulator recreates the experience of **Xenix System V Release 2.3.2**, which was popular in the mid-to-late 1980s. Xenix was significant because:

- It was the most widely used Unix variant of the 1980s
- It ran on affordable microcomputers (not just minicomputers)
- It introduced many users to Unix concepts
- SCO later evolved it into SCO Unix and UnixWare

The copyright messages, MOTD format, command output styles, and shell behavior are all based on authentic Xenix systems from this era.

## Contributing

Contributions are welcome! Some ideas for enhancements:

- Add more authentic Xenix commands (`awk`, `sed`, `tar`, etc.)
- Implement pipe support (`|`)
- Add job control (background processes with `&`)
- Implement more complete vi keybindings
- Add user management (multiple users)
- Implement file permissions enforcement

## License

MIT License - Feel free to use, modify, and distribute.

## Credits

Created to preserve and share the experience of 1980s Unix computing.

Special thanks to:
- The Santa Cruz Operation (SCO) for Xenix
- Microsoft Corporation for bringing Unix to microcomputers
- AT&T for the original Unix system
- The MicroPython community for the Raspberry Pi Pico port

## References

- [Xenix on Wikipedia](https://en.wikipedia.org/wiki/Xenix)
- [MicroPython Documentation](https://docs.micropython.org/)
- [Raspberry Pi Pico Documentation](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html)

---

**Enjoy your journey back to 1980s Unix computing!** 🖥️✨
