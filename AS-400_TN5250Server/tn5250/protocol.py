# TN5250 Protocol Constants for Server
# Used to build 5250 data streams to send to clients

# =============================================================================
# Telnet Negotiation
# =============================================================================
IAC = 0xFF
DONT = 0xFE
DO = 0xFD
WONT = 0xFC
WILL = 0xFB
SB = 0xFA
SE = 0xF0

OPT_BINARY = 0x00
OPT_ECHO = 0x01
OPT_TERMINAL_TYPE = 0x18
OPT_EOR = 0x19
OPT_NEW_ENVIRON = 0x27

EOR = 0xEF  # End of Record

# =============================================================================
# 5250 Data Stream Commands
# =============================================================================
ESC = 0x04  # 5250 Escape (not ASCII ESC!)

# Commands sent TO the terminal
CMD_WRITE_TO_DISPLAY = 0x11
CMD_CLEAR_UNIT = 0x40
CMD_CLEAR_FORMAT_TABLE = 0x50
CMD_READ_INPUT_FIELDS = 0x42
CMD_READ_MDT_FIELDS = 0x52
CMD_WRITE_STRUCTURED_FIELD = 0xF3

# Write Control Character flags
WCC_RESET_MDT = 0x20        # Reset Modified Data Tags
WCC_RESET_KEYBOARD = 0x40   # Unlock keyboard
WCC_SOUND_ALARM = 0x04      # Beep

# =============================================================================
# Orders (commands within the data stream)
# =============================================================================
SBA = 0x11   # Set Buffer Address
SF = 0x1D    # Start Field
IC = 0x13    # Insert Cursor
RA = 0x02    # Repeat to Address
EA = 0x12    # Erase to Address
SOH = 0x01   # Start of Header
TD = 0x10    # Transparent Data

# =============================================================================
# Field Attributes
# =============================================================================
# Field Format Word byte 1
FFW_BYPASS = 0x20           # Protected/output field
FFW_DUP_ENABLE = 0x10
FFW_MDT = 0x08              # Modified Data Tag
FFW_ALPHA_SHIFT = 0x00
FFW_ALPHA_ONLY = 0x01
FFW_NUMERIC_SHIFT = 0x02
FFW_NUMERIC_ONLY = 0x03
FFW_SIGNED_NUMERIC = 0x06

# Display attributes (byte 2)
ATTR_NORMAL = 0x20
ATTR_REVERSE = 0x21
ATTR_HIGH_INTENSITY = 0x22
ATTR_INVISIBLE = 0x27       # For password fields
ATTR_UNDERSCORE = 0x24
ATTR_BLINK = 0x25
ATTR_COL_SEP = 0x26

# Color attributes (extended)
COLOR_GREEN = 0x20
COLOR_WHITE = 0x22
COLOR_RED = 0x28
COLOR_TURQUOISE = 0x30
COLOR_YELLOW = 0x32
COLOR_PINK = 0x38
COLOR_BLUE = 0x3A

# =============================================================================
# AID Codes (received FROM terminal)
# =============================================================================
AID_ENTER = 0xF1
AID_CLEAR = 0xBD
AID_HELP = 0xF3
AID_PRINT = 0xF6
AID_ROLLDOWN = 0xF4   # Page Up
AID_ROLLUP = 0xF5     # Page Down
AID_SYSRQ = 0x01

# Function keys
AID_F1 = 0x31
AID_F2 = 0x32
AID_F3 = 0x33
AID_F4 = 0x34
AID_F5 = 0x35
AID_F6 = 0x36
AID_F7 = 0x37
AID_F8 = 0x38
AID_F9 = 0x39
AID_F10 = 0x3A
AID_F11 = 0x3B
AID_F12 = 0x3C
AID_F13 = 0xB1
AID_F14 = 0xB2
AID_F15 = 0xB3
AID_F16 = 0xB4
AID_F17 = 0xB5
AID_F18 = 0xB6
AID_F19 = 0xB7
AID_F20 = 0xB8
AID_F21 = 0xB9
AID_F22 = 0xBA
AID_F23 = 0xBB
AID_F24 = 0xBC

AID_NAMES = {
    AID_ENTER: "ENTER",
    AID_CLEAR: "CLEAR",
    AID_HELP: "HELP",
    AID_F1: "F1", AID_F2: "F2", AID_F3: "F3", AID_F4: "F4",
    AID_F5: "F5", AID_F6: "F6", AID_F7: "F7", AID_F8: "F8",
    AID_F9: "F9", AID_F10: "F10", AID_F11: "F11", AID_F12: "F12",
    AID_F13: "F13", AID_F14: "F14", AID_F15: "F15", AID_F16: "F16",
    AID_F17: "F17", AID_F18: "F18", AID_F19: "F19", AID_F20: "F20",
    AID_F21: "F21", AID_F22: "F22", AID_F23: "F23", AID_F24: "F24",
    AID_ROLLUP: "PAGEDOWN", AID_ROLLDOWN: "PAGEUP",
    AID_SYSRQ: "SYSRQ", AID_PRINT: "PRINT",
}

# =============================================================================
# EBCDIC <-> ASCII Translation
# =============================================================================
ASCII_TO_EBCDIC = bytearray(256)
EBCDIC_TO_ASCII = bytearray(256)

# Initialize with defaults
for i in range(256):
    ASCII_TO_EBCDIC[i] = 0x40  # EBCDIC space
    EBCDIC_TO_ASCII[i] = 0x20  # ASCII space

# Character mappings (Code Page 37)
_a2e = {
    0x20: 0x40,  # Space
    0x21: 0x5A, 0x22: 0x7F, 0x23: 0x7B, 0x24: 0x5B, 0x25: 0x6C,
    0x26: 0x50, 0x27: 0x7D, 0x28: 0x4D, 0x29: 0x5D, 0x2A: 0x5C,
    0x2B: 0x4E, 0x2C: 0x6B, 0x2D: 0x60, 0x2E: 0x4B, 0x2F: 0x61,
    # Numbers
    0x30: 0xF0, 0x31: 0xF1, 0x32: 0xF2, 0x33: 0xF3, 0x34: 0xF4,
    0x35: 0xF5, 0x36: 0xF6, 0x37: 0xF7, 0x38: 0xF8, 0x39: 0xF9,
    0x3A: 0x7A, 0x3B: 0x5E, 0x3C: 0x4C, 0x3D: 0x7E, 0x3E: 0x6E,
    0x3F: 0x6F, 0x40: 0x7C,
    # Uppercase
    0x41: 0xC1, 0x42: 0xC2, 0x43: 0xC3, 0x44: 0xC4, 0x45: 0xC5,
    0x46: 0xC6, 0x47: 0xC7, 0x48: 0xC8, 0x49: 0xC9,
    0x4A: 0xD1, 0x4B: 0xD2, 0x4C: 0xD3, 0x4D: 0xD4, 0x4E: 0xD5,
    0x4F: 0xD6, 0x50: 0xD7, 0x51: 0xD8, 0x52: 0xD9,
    0x53: 0xE2, 0x54: 0xE3, 0x55: 0xE4, 0x56: 0xE5, 0x57: 0xE6,
    0x58: 0xE7, 0x59: 0xE8, 0x5A: 0xE9,
    0x5B: 0xBA, 0x5C: 0xE0, 0x5D: 0xBB, 0x5E: 0xB0, 0x5F: 0x6D,
    0x60: 0x79,
    # Lowercase
    0x61: 0x81, 0x62: 0x82, 0x63: 0x83, 0x64: 0x84, 0x65: 0x85,
    0x66: 0x86, 0x67: 0x87, 0x68: 0x88, 0x69: 0x89,
    0x6A: 0x91, 0x6B: 0x92, 0x6C: 0x93, 0x6D: 0x94, 0x6E: 0x95,
    0x6F: 0x96, 0x70: 0x97, 0x71: 0x98, 0x72: 0x99,
    0x73: 0xA2, 0x74: 0xA3, 0x75: 0xA4, 0x76: 0xA5, 0x77: 0xA6,
    0x78: 0xA7, 0x79: 0xA8, 0x7A: 0xA9,
    0x7B: 0xC0, 0x7C: 0x4F, 0x7D: 0xD0, 0x7E: 0xA1,
}

for ascii_val, ebcdic_val in _a2e.items():
    ASCII_TO_EBCDIC[ascii_val] = ebcdic_val
    EBCDIC_TO_ASCII[ebcdic_val] = ascii_val


def ascii_to_ebcdic_bytes(text):
    """Convert ASCII string to EBCDIC bytes."""
    if isinstance(text, str):
        text = text.encode('ascii', errors='replace')
    return bytes(ASCII_TO_EBCDIC[b] for b in text)


def ebcdic_to_ascii_bytes(data):
    """Convert EBCDIC bytes to ASCII bytes."""
    return bytes(EBCDIC_TO_ASCII[b] for b in data)


def ebcdic_to_ascii_str(data):
    """Convert EBCDIC bytes to ASCII string."""
    return ''.join(chr(EBCDIC_TO_ASCII[b]) for b in data)
