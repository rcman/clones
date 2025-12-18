# TN5250 Command Constants and Definitions
# Reference: RFC 4777 (Telnet Environment Option) and IBM 5250 Data Stream documentation

# =============================================================================
# Telnet Negotiation Codes
# =============================================================================
IAC = 0xFF   # Interpret As Command
DONT = 0xFE
DO = 0xFD
WONT = 0xFC
WILL = 0xFB
SB = 0xFA    # Subnegotiation Begin
SE = 0xF0    # Subnegotiation End

# Telnet Options
OPT_BINARY = 0x00
OPT_ECHO = 0x01
OPT_TERMINAL_TYPE = 0x18
OPT_EOR = 0x19           # End of Record
OPT_NAWS = 0x1F          # Window Size
OPT_NEW_ENVIRON = 0x27   # New Environment

EOR = 0xEF  # End of Record marker

# =============================================================================
# 5250 Escape Codes and Commands
# =============================================================================
ESC = 0x04  # 5250 Escape code (not standard ASCII ESC!)

# Write Control Characters (WCC)
CMD_CLEAR_UNIT = 0x40
CMD_CLEAR_FORMAT_TABLE = 0x50
CMD_CLEAR_UNIT_ALTERNATE = 0x20

# 5250 Data Stream Commands (following ESC)
CMD_WRITE_TO_DISPLAY = 0x11      # WTD - Write To Display
CMD_READ_INPUT_FIELDS = 0x42    # Read all input fields
CMD_READ_MDT_FIELDS = 0x52      # Read Modified Data Tag fields
CMD_READ_SCREEN = 0x62          # Read entire screen
CMD_READ_IMMEDIATE = 0x72       # Read immediate (AID only)
CMD_SAVE_SCREEN = 0x02          # Save screen
CMD_RESTORE_SCREEN = 0x12       # Restore screen
CMD_WRITE_STRUCTURED_FIELD = 0xF3  # WSF
CMD_ROLL = 0x23                 # Roll screen up/down

# Start Field Orders
SF = 0x1D           # Start Field
SFE = 0x29          # Start Field Extended

# Other Orders
IC = 0x13           # Insert Cursor
RA = 0x02           # Repeat to Address
SBA = 0x11          # Set Buffer Address
MC = 0x15           # Move Cursor
TD = 0x10           # Transparent Data
SOH = 0x01          # Start of Header
EUA = 0x12          # Erase Unprotected to Address

# =============================================================================
# Field Attribute Definitions
# =============================================================================
# Field Format Word (FFW) flags - first byte
FFW_BYPASS = 0x20         # Bypass field (protected)
FFW_DUP_ENABLE = 0x10     # Dup key enabled
FFW_MDT = 0x08            # Modified Data Tag
FFW_ALPHA_SHIFT = 0x00    # Alpha shift
FFW_ALPHA_ONLY = 0x01     # Alpha only
FFW_NUMERIC_SHIFT = 0x02  # Numeric shift  
FFW_NUMERIC_ONLY = 0x03   # Numeric only
FFW_DIGITS_ONLY = 0x05    # Digits only
FFW_SIGNED_NUMERIC = 0x06 # Signed numeric
FFW_INHIBIT_ENTRY = 0x07  # Field entry inhibited

# Field Control Word (FCW) flags - extended attributes
FCW_MANDATORY_ENTRY = 0x01
FCW_MANDATORY_FILL = 0x02
FCW_RIGHT_ADJUST_BLANK = 0x05
FCW_RIGHT_ADJUST_ZERO = 0x06
FCW_MONOCASE = 0x20

# Display Attributes (Second FFW byte)
ATTR_NORMAL = 0x20
ATTR_REVERSE = 0x21
ATTR_HIGH_INTENSITY = 0x22
ATTR_INVISIBLE = 0x27
ATTR_UNDERSCORE = 0x24
ATTR_BLINK = 0x25
ATTR_COL_SEPARATOR = 0x26

# Attribute colors (in extended attributes)
ATTR_COLOR_GREEN = 0x20
ATTR_COLOR_WHITE = 0x22
ATTR_COLOR_RED = 0x28
ATTR_COLOR_TURQUOISE = 0x30
ATTR_COLOR_YELLOW = 0x32
ATTR_COLOR_PINK = 0x38
ATTR_COLOR_BLUE = 0x3A

# =============================================================================
# AID Codes (Attention Identifier) - sent when user presses keys
# =============================================================================
AID_NO_AID = 0x60
AID_ENTER = 0xF1
AID_CLEAR = 0xBD
AID_HELP = 0xF3
AID_PRINT = 0xF6
AID_ROLLDOWN = 0xF4   # Page Up
AID_ROLLUP = 0xF5     # Page Down
AID_RECORD_BACKSPACE = 0xF8
AID_AUTO_ENTER = 0x3F
AID_SYSRQ = 0x01      # System Request

# Function Keys
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

# AID code lookup table for keyboard mapping
AID_KEYS = {
    'F1': AID_F1, 'F2': AID_F2, 'F3': AID_F3, 'F4': AID_F4,
    'F5': AID_F5, 'F6': AID_F6, 'F7': AID_F7, 'F8': AID_F8,
    'F9': AID_F9, 'F10': AID_F10, 'F11': AID_F11, 'F12': AID_F12,
    'ENTER': AID_ENTER, 'CLEAR': AID_CLEAR, 'HELP': AID_HELP,
    'PAGEUP': AID_ROLLDOWN, 'PAGEDOWN': AID_ROLLUP,
    'SYSRQ': AID_SYSRQ, 'PRINT': AID_PRINT,
}

# =============================================================================
# EBCDIC to ASCII Translation Table
# =============================================================================
# IBM AS/400 uses EBCDIC encoding - we need to translate to/from ASCII
EBCDIC_TO_ASCII = bytearray(256)
ASCII_TO_EBCDIC = bytearray(256)

# Initialize with placeholder
for i in range(256):
    EBCDIC_TO_ASCII[i] = 0x20  # Space for unknown
    ASCII_TO_EBCDIC[i] = 0x40  # EBCDIC space for unknown

# Common character mappings (EBCDIC code page 37)
_e2a_map = {
    0x40: 0x20,  # Space
    0x4B: 0x2E,  # .
    0x4C: 0x3C,  # <
    0x4D: 0x28,  # (
    0x4E: 0x2B,  # +
    0x4F: 0x7C,  # |
    0x50: 0x26,  # &
    0x5A: 0x21,  # !
    0x5B: 0x24,  # $
    0x5C: 0x2A,  # *
    0x5D: 0x29,  # )
    0x5E: 0x3B,  # ;
    0x60: 0x2D,  # -
    0x61: 0x2F,  # /
    0x6B: 0x2C,  # ,
    0x6C: 0x25,  # %
    0x6D: 0x5F,  # _
    0x6E: 0x3E,  # >
    0x6F: 0x3F,  # ?
    0x7A: 0x3A,  # :
    0x7B: 0x23,  # #
    0x7C: 0x40,  # @
    0x7D: 0x27,  # '
    0x7E: 0x3D,  # =
    0x7F: 0x22,  # "
    # Lowercase letters
    0x81: 0x61, 0x82: 0x62, 0x83: 0x63, 0x84: 0x64, 0x85: 0x65,
    0x86: 0x66, 0x87: 0x67, 0x88: 0x68, 0x89: 0x69,
    0x91: 0x6A, 0x92: 0x6B, 0x93: 0x6C, 0x94: 0x6D, 0x95: 0x6E,
    0x96: 0x6F, 0x97: 0x70, 0x98: 0x71, 0x99: 0x72,
    0xA2: 0x73, 0xA3: 0x74, 0xA4: 0x75, 0xA5: 0x76, 0xA6: 0x77,
    0xA7: 0x78, 0xA8: 0x79, 0xA9: 0x7A,
    # Uppercase letters
    0xC1: 0x41, 0xC2: 0x42, 0xC3: 0x43, 0xC4: 0x44, 0xC5: 0x45,
    0xC6: 0x46, 0xC7: 0x47, 0xC8: 0x48, 0xC9: 0x49,
    0xD1: 0x4A, 0xD2: 0x4B, 0xD3: 0x4C, 0xD4: 0x4D, 0xD5: 0x4E,
    0xD6: 0x4F, 0xD7: 0x50, 0xD8: 0x51, 0xD9: 0x52,
    0xE2: 0x53, 0xE3: 0x54, 0xE4: 0x55, 0xE5: 0x56, 0xE6: 0x57,
    0xE7: 0x58, 0xE8: 0x59, 0xE9: 0x5A,
    # Numbers
    0xF0: 0x30, 0xF1: 0x31, 0xF2: 0x32, 0xF3: 0x33, 0xF4: 0x34,
    0xF5: 0x35, 0xF6: 0x36, 0xF7: 0x37, 0xF8: 0x38, 0xF9: 0x39,
}

# Build translation tables
for ebcdic, ascii_val in _e2a_map.items():
    EBCDIC_TO_ASCII[ebcdic] = ascii_val
    ASCII_TO_EBCDIC[ascii_val] = ebcdic


def ebcdic_to_ascii(data: bytes) -> bytes:
    """Convert EBCDIC bytes to ASCII."""
    return bytes(EBCDIC_TO_ASCII[b] for b in data)


def ascii_to_ebcdic(data: bytes) -> bytes:
    """Convert ASCII bytes to EBCDIC."""
    return bytes(ASCII_TO_EBCDIC[b] for b in data)


def ebcdic_char(b: int) -> str:
    """Convert single EBCDIC byte to ASCII character."""
    return chr(EBCDIC_TO_ASCII[b])


def ascii_char_to_ebcdic(c: str) -> int:
    """Convert single ASCII character to EBCDIC byte."""
    return ASCII_TO_EBCDIC[ord(c)]
