# TN5250 Emulator Configuration
# Adjust these settings for your environment

# WiFi Settings
WIFI_SSID = "YOUR_WIFI_SSID"
WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"

# AS/400 Connection Settings
HOST = "your-as400-hostname-or-ip"
PORT = 23  # Standard Telnet port, use 992 for TLS (not supported in basic version)
DEVICE_NAME = "PICO5250"  # Virtual device name

# Display Settings (ILI9341 320x240)
DISPLAY_WIDTH = 320
DISPLAY_HEIGHT = 240
DISPLAY_ROTATION = 0  # 0, 90, 180, or 270

# SPI Pins for Display
PIN_SCK = 18
PIN_MOSI = 19
PIN_CS = 17
PIN_DC = 20
PIN_RST = 21

# PS/2 Keyboard Pins
PIN_KB_CLOCK = 14
PIN_KB_DATA = 15

# Terminal Settings
TERM_ROWS = 24
TERM_COLS = 80

# Colors (RGB565 format)
COLOR_GREEN = 0x07E0      # Classic green screen
COLOR_BLACK = 0x0000
COLOR_WHITE = 0xFFFF
COLOR_RED = 0xF800
COLOR_BLUE = 0x001F
COLOR_YELLOW = 0xFFE0

# Use green screen theme by default
FG_COLOR = COLOR_GREEN
BG_COLOR = COLOR_BLACK
