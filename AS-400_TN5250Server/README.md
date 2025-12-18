# Fake AS/400 TN5250 Server

A MicroPython-based TN5250 server that emulates the look and feel of an IBM AS/400 (iSeries) green-screen interface. Connect with any standard 5250 terminal emulator!

## What This Does

This turns your Raspberry Pi Pico 2 W into a fake AS/400 that:

- Accepts incoming TN5250 terminal connections
- Displays an authentic-looking sign-on screen  
- Authenticates users (configurable user list)
- Provides navigable menus
- Responds to function keys (F1-F24)
- Handles basic commands
- Supports multiple concurrent sessions (limited by Pico memory)

**It does NOT actually run OS/400 or AS/400 programs** — it's a convincing simulation for demos, training, nostalgia, or as a retro-styled menu system.

## Hardware Requirements

| Component | Required? | Notes |
|-----------|-----------|-------|
| Raspberry Pi Pico 2 W | Yes | Must have W variant for WiFi |
| USB cable | Yes | For power and initial programming |
| Display | No | Optional ILI9341 for local status |

That's it! No keyboard or display needed since users connect remotely.

## Quick Start

### 1. Install MicroPython

Download MicroPython for Pico 2 W:
https://micropython.org/download/RPI_PICO2_W/

Hold BOOTSEL, connect USB, copy the .uf2 file.

### 2. Upload Files

Using Thonny or mpremote:

```bash
# Install mpremote
pip install mpremote

# Upload everything
mpremote connect /dev/ttyACM0 fs cp -r as400_emulator/* :
```

### 3. Configure

Edit `config.py`:

```python
WIFI_SSID = "YourNetwork"
WIFI_PASSWORD = "YourPassword"
SYSTEM_NAME = "MYISERIES"  # Appears on sign-on screen

# Add your users
USERS = {
    "QSECOFR": "SECRET",
    "ADMIN": "ADMIN",
    "DEMO": "",  # No password
}
```

### 4. Connect Power

The server starts automatically. Watch the USB serial console for the IP address:

```
========================================
  FAKE AS/400 TN5250 SERVER
========================================

Connecting to WiFi: MyNetwork...
Connected! IP: 192.168.1.42

Server listening on port 23

Connect using:
    tn5250 192.168.1.42
```

### 5. Connect a Terminal

Use any TN5250 terminal emulator:

**Linux:**
```bash
# Install tn5250
sudo apt install tn5250

# Connect
tn5250 192.168.1.42
```

**Windows/Mac:**
- [tn5250j](https://tn5250j.github.io/) (free, cross-platform)
- IBM i Access Client Solutions

**Any platform:**
```bash
# Basic telnet works too (less features)
telnet 192.168.1.42 23
```

## Screens & Navigation

### Sign-On Screen

```
                          Sign On

System . . . . . :   PICO400
Subsystem  . . . :   QINTER
Display  . . . . :   QPADEV0001

    +----------------------[ Sign On ]----------------------+
    |                                                        |
    |  User  . . . . . . . . . . . . __________             |
    |  Password  . . . . . . . . . . __________             |
    |  Program/procedure . . . . . . __________             |
    |  Menu  . . . . . . . . . . . . __________             |
    |  Current library . . . . . . . __________             |
    +--------------------------------------------------------+

(C) COPYRIGHT PICO SYSTEMS 2025

F3=Exit   F12=Cancel
```

### Main Menu

After signing on, you see:

```
                 PICO400 - MAIN MENU                User: DEMO

Select one of the following:

     1. Work with messages
     2. Work with files
     3. Work with output queue
     4. Display job status
     5. System status
     6. Display active jobs
     7. Work with spool files
     8. About this system
     90. Sign off

Selection or command
===> ______________________________________________________________________

F3=Exit   F4=Prompt   F9=Retrieve   F12=Cancel
```

### Available Options

| Option | Description |
|--------|-------------|
| 1 | Work with messages (shows no messages) |
| 2 | Work with files (not implemented message) |
| 3 | Output queue status |
| 4 | Display job status |
| 5 | System status (shows real Pico memory!) |
| 6 | Display active jobs |
| 7 | Spool files |
| 8 | About this system |
| 90 | Sign off |

### Commands

Type commands at the `===>` prompt:

| Command | Action |
|---------|--------|
| `SIGNOFF` | End session |
| `WRKSYSSTS` | System status |
| `WRKACTJOB` | Active jobs |
| `DSPJOB` | Job status |
| `GO MAIN` | Go to menu |
| `CALL QCMD` | Command entry screen |
| `?` or `HELP` | Show help |

### Function Keys

| Key | Action |
|-----|--------|
| F1 | Help (where applicable) |
| F3 | Exit/Sign off |
| F4 | Prompt |
| F12 | Cancel/Go back |
| Enter | Submit |

## Project Structure

```
as400_emulator/
├── main.py              # Entry point
├── config.py            # Configuration
├── network.py           # WiFi helper
└── tn5250/
    ├── __init__.py      # Package init
    ├── protocol.py      # TN5250/EBCDIC constants
    ├── screen_builder.py # Screen construction
    ├── parser.py        # Response parsing
    ├── session.py       # Session state machine
    └── server.py        # TCP server
```

## Customization

### Adding Screens

Create new screens in `screen_builder.py`:

```python
@staticmethod
def my_custom_screen():
    s = ScreenBuilder()
    s.clear()
    s.write_to_display()
    
    s.center_text(1, "MY CUSTOM SCREEN", highlight=True)
    s.text(5, 10, "Hello from my screen!")
    
    s.text(20, 2, "F3=Exit")
    s.cursor(5, 35)
    
    return s.build(), s.get_fields()
```

### Adding Menu Options

Edit `session.py` `_process_menu_command()`:

```python
elif command == '99':
    # My custom option
    self._show_message("Custom", ["You selected option 99!"])
```

### Adding Commands

In the same function:

```python
elif cmd_upper == 'MYCOMMAND':
    self._show_message("My Command", ["Command executed!"])
```

## Limitations

- Single-threaded (handles one thing at a time)
- 1-2 concurrent sessions max (memory limited)
- No actual program execution
- No database or file system
- No print support
- 24x80 display only

## Troubleshooting

### "Connection refused"

- Is the Pico powered on?
- Check WiFi connection
- Verify IP address
- Ensure port 23 is not blocked

### "Garbled display"

- Use a proper TN5250 client, not just telnet
- Check terminal type setting (IBM-3179-2 works well)

### "Can't sign on"

- Check username in config.py (case-sensitive!)
- Check password
- Try user `GUEST` with no password

### Memory issues

- Reduce `MAX_CONNECTIONS` to 1
- Close unused sessions
- The Pico only has 520KB RAM

## Use Cases

1. **Retro Demo** — Show people what AS/400 looked like
2. **Training** — Practice 5250 navigation without a real system
3. **Escape Room** — Create a hacking/terminal puzzle
4. **IoT Menu** — Build a green-screen interface for your IoT project
5. **Nostalgia** — Relive the glory days of midrange computing

## Technical Notes

### TN5250 Protocol

The server implements enough of RFC 4777 to work with standard clients:
- Telnet negotiation (BINARY, EOR, TERMINAL-TYPE)
- 5250 data stream generation (WTD, SBA, SF, IC, RA)
- AID response parsing (Enter, function keys)
- EBCDIC/ASCII translation

### Why MicroPython?

- Runs on cheap hardware ($6 Pico)
- Easy to modify
- No OS overhead
- Fun to hack on

## License

MIT License — do whatever you want with it!

## Acknowledgments

- IBM, for creating the AS/400 and 5250 protocol
- The MicroPython team
- Everyone who still remembers green screens fondly

---

*"The AS/400 never dies, it just gets emulated on increasingly smaller hardware."*
