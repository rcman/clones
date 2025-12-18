# TN5250 Terminal Emulator for Raspberry Pi Pico 2 W

A MicroPython-based IBM 5250 terminal emulator that connects to AS/400 (IBM i) systems via the TN5250 Telnet protocol.

## Features

- Full TN5250 protocol support for terminal emulation
- Classic green-screen display on ILI9341 LCD
- PS/2 keyboard input
- WiFi connectivity (Pico 2 W)
- Support for function keys F1-F12
- Field-based input with proper cursor navigation
- EBCDIC to ASCII translation

## Hardware Requirements

### Core Components

| Component | Model | Notes |
|-----------|-------|-------|
| Microcontroller | Raspberry Pi Pico 2 W | Must have W variant for WiFi |
| Display | ILI9341 320x240 SPI LCD | 2.8" or 3.2" recommended |
| Keyboard | PS/2 keyboard | Or USB with adapter |

### Optional Components

- USB-to-PS/2 adapter (for USB keyboards)
- 3D printed case
- Level shifter (if using 5V PS/2 keyboard)

## Wiring Diagram

```
Raspberry Pi Pico 2 W          ILI9341 Display
========================       ================
GP18 (SPI0 SCK)    ─────────── SCK
GP19 (SPI0 TX)     ─────────── MOSI (SDA)
GP17               ─────────── CS
GP20               ─────────── DC (RS)
GP21               ─────────── RESET
3V3                ─────────── VCC
GND                ─────────── GND
3V3                ─────────── LED (backlight)


Raspberry Pi Pico 2 W          PS/2 Keyboard
========================       ==============
GP14               ─────────── Clock (pin 5, purple)
GP15               ─────────── Data (pin 1, green)
3V3                ─────────── VCC (pin 4, red)
GND                ─────────── GND (pin 3, black)
```

### PS/2 Connector Pinout (looking at socket)
```
    6 o   o 5
   4 o     o 3
     o 2 1 o
     
1 = Data (green)
2 = Not used
3 = GND (black)
4 = VCC +5V (red) - use 3.3V from Pico
5 = Clock (purple)
6 = Not used
```

## Installation

### 1. Install MicroPython

Download MicroPython for Pico 2 W from:
https://micropython.org/download/RPI_PICO2_W/

Hold BOOTSEL button, connect USB, copy .uf2 file to the drive.

### 2. Upload Files

Using Thonny IDE or `mpremote`:

```bash
# Install mpremote if needed
pip install mpremote

# Upload all files
mpremote connect /dev/ttyACM0 fs cp -r tn5250_emulator/* :
```

Or manually copy each folder and file to the Pico.

### 3. Configure

Edit `config.py` with your settings:

```python
# WiFi Settings
WIFI_SSID = "YourNetworkName"
WIFI_PASSWORD = "YourPassword"

# AS/400 Connection
HOST = "192.168.1.100"  # Your AS/400 IP address
PORT = 23               # Telnet port (usually 23)
DEVICE_NAME = "PICO5250" # Virtual device name
```

### 4. Run

The emulator starts automatically on boot, or run manually:

```python
import main
main.main()
```

## Usage

### Key Mappings

| Key | Function |
|-----|----------|
| F1-F12 | Function keys (send to AS/400) |
| Enter | Submit/Enter |
| Tab | Next field |
| Arrow keys | Cursor movement |
| Home | First input field |
| Page Up | Roll Down (previous page) |
| Page Down | Roll Up (next page) |
| Backspace | Delete character |

### Common AS/400 Key Combinations

| Action | Keys |
|--------|------|
| Sign off | F3 (usually) |
| Command line | F10 (often) |
| Cancel | F12 (usually) |
| Help | F1 |

## Troubleshooting

### Display Issues

- **Blank screen**: Check SPI wiring, especially SCK and MOSI
- **Garbled display**: Verify CS and DC pins
- **No backlight**: Connect LED pin to 3V3

### Keyboard Issues

- **No input**: Check Clock and Data pins aren't swapped
- **Intermittent**: Try shorter cables, add pull-up resistors
- **Wrong characters**: Verify it's a genuine PS/2 keyboard (not USB)

### Connection Issues

- **WiFi fails**: Double-check SSID/password, ensure 2.4GHz network
- **Host unreachable**: Verify IP address, check firewall rules
- **Login fails**: Confirm TN5250 is enabled on AS/400

### AS/400 Configuration

On the AS/400, ensure:

1. TCP/IP is started: `STRTCP`
2. Telnet server is active: `STRTCPSVR *TELNET`
3. Port 23 is accessible through any firewalls

## Project Structure

```
tn5250_emulator/
├── main.py              # Main application
├── config.py            # Configuration settings
├── network.py           # WiFi connection helper
├── tn5250/
│   ├── __init__.py
│   ├── commands.py      # TN5250 protocol constants
│   ├── connection.py    # Network/Telnet handling
│   ├── parser.py        # Data stream parser
│   └── screen.py        # Screen buffer management
├── display/
│   ├── __init__.py
│   └── driver.py        # ILI9341 + terminal rendering
├── input/
│   ├── __init__.py
│   └── ps2keyboard.py   # PS/2 keyboard driver
└── README.md
```

## Limitations

- 24x80 display only (no 27x132 mode)
- Basic color support (green on black)
- No SSL/TLS support (plain Telnet only)
- Single session only
- No print support

## Future Enhancements

- [ ] 27x132 extended screen support
- [ ] Multiple color themes
- [ ] TLS encryption (port 992)
- [ ] Session recording/playback
- [ ] Macro support
- [ ] USB HID keyboard support
- [ ] Web-based configuration

## License

MIT License - feel free to use and modify.

## Resources

- [RFC 4777 - TN5250E](https://tools.ietf.org/html/rfc4777)
- [IBM 5250 Data Stream Reference](https://www.ibm.com/docs/en/i/7.4?topic=interface-5250-data-stream)
- [MicroPython Documentation](https://docs.micropython.org/)
- [Raspberry Pi Pico Documentation](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html)

## Acknowledgments

Inspired by classic terminal emulators and the enduring legacy of AS/400 systems in enterprise computing.
