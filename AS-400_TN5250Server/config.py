# Fake AS/400 TN5250 Server Configuration
# Raspberry Pi Pico 2 W

# WiFi Settings
WIFI_SSID = "YOUR_WIFI_SSID"
WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"

# Server Settings
SERVER_PORT = 23          # Standard Telnet port
MAX_CONNECTIONS = 1       # Pico can realistically handle 1-2 sessions
SYSTEM_NAME = "PICO400"   # System name shown on sign-on screen
SUBSYSTEM = "QINTER"      # Fake subsystem name

# Display Settings (optional local display)
USE_LOCAL_DISPLAY = False  # Set True if ILI9341 connected
PIN_SCK = 18
PIN_MOSI = 19
PIN_CS = 17
PIN_DC = 20
PIN_RST = 21

# Colors (RGB565 for local display)
COLOR_GREEN = 0x07E0
COLOR_BLACK = 0x0000

# Demo Users (username: password)
USERS = {
    "QSECOFR": "QSECOFR",
    "QUSER": "QUSER",
    "DEMO": "DEMO",
    "GUEST": "",           # No password required
}

# Menu System Configuration
DEFAULT_MENU = "MAIN"

# Idle timeout (seconds) - 0 to disable
IDLE_TIMEOUT = 300
