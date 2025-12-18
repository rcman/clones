"""
TN5250 Terminal Emulator for Raspberry Pi Pico 2 W
Main application entry point

This emulator connects to an IBM AS/400 (iSeries) system via TN5250
protocol and provides a classic green-screen terminal interface.

Hardware Requirements:
- Raspberry Pi Pico 2 W (with WiFi)
- ILI9341 320x240 SPI display
- PS/2 keyboard (optional, can use USB serial for testing)

Wiring:
  Display (ILI9341):
    GP18 -> SCK
    GP19 -> MOSI  
    GP17 -> CS
    GP20 -> DC
    GP21 -> RESET
    3V3  -> VCC
    GND  -> GND
    
  PS/2 Keyboard:
    GP14 -> Clock
    GP15 -> Data
    3V3  -> VCC
    GND  -> GND
"""

from machine import Pin, SPI
import time
import sys

# Import configuration
import config

# Import our modules
from network import WiFiConnection
from display import ILI9341, TerminalDisplay
from input import PS2Keyboard
from tn5250 import Screen, TN5250Connection, DataStreamParser, AID_KEYS


class TN5250Emulator:
    """
    Main TN5250 terminal emulator application.
    """
    
    def __init__(self):
        self.wifi = None
        self.display = None
        self.terminal = None
        self.keyboard = None
        self.connection = None
        self.screen = None
        self.parser = None
        
        self.running = False
        self.debug = False
    
    def init_hardware(self):
        """Initialize all hardware components."""
        print("Initializing hardware...")
        
        # Initialize SPI for display
        spi = SPI(0,
                  baudrate=40000000,
                  polarity=0,
                  phase=0,
                  sck=Pin(config.PIN_SCK),
                  mosi=Pin(config.PIN_MOSI))
        
        # Initialize display
        print("  Display...")
        self.display = ILI9341(
            spi,
            cs_pin=config.PIN_CS,
            dc_pin=config.PIN_DC,
            rst_pin=config.PIN_RST
        )
        
        # Initialize terminal renderer
        self.terminal = TerminalDisplay(
            self.display,
            rows=config.TERM_ROWS,
            cols=config.TERM_COLS,
            fg=config.FG_COLOR,
            bg=config.BG_COLOR
        )
        
        # Clear display and show boot message
        self.terminal.clear()
        self.terminal.draw_text(0, "TN5250 Emulator for Pico 2 W")
        self.terminal.draw_text(1, "Initializing...")
        
        # Initialize keyboard
        print("  Keyboard...")
        try:
            self.keyboard = PS2Keyboard(
                config.PIN_KB_CLOCK,
                config.PIN_KB_DATA
            )
            self.terminal.draw_text(2, "Keyboard: PS/2 ready")
        except Exception as e:
            print(f"  Keyboard init failed: {e}")
            self.terminal.draw_text(2, "Keyboard: Not detected")
            self.keyboard = None
        
        # Initialize screen buffer
        self.screen = Screen(config.TERM_ROWS, config.TERM_COLS)
        
        # Initialize parser
        self.parser = DataStreamParser(self.screen)
        self.parser.debug = self.debug
        
        print("Hardware initialized!")
    
    def connect_wifi(self):
        """Connect to WiFi network."""
        self.terminal.draw_text(4, f"WiFi: Connecting to {config.WIFI_SSID}...")
        
        self.wifi = WiFiConnection(config.WIFI_SSID, config.WIFI_PASSWORD)
        
        if self.wifi.connect(timeout=30):
            self.terminal.draw_text(4, f"WiFi: {self.wifi.ip_address}")
            return True
        else:
            self.terminal.draw_text(4, "WiFi: Connection failed!")
            return False
    
    def connect_host(self):
        """Connect to AS/400 host."""
        self.terminal.draw_text(6, f"Host: Connecting to {config.HOST}...")
        
        self.connection = TN5250Connection(
            config.HOST,
            config.PORT,
            config.DEVICE_NAME
        )
        self.connection.debug = self.debug
        
        if self.connection.connect():
            self.terminal.draw_text(6, f"Host: Connected to {config.HOST}")
            return True
        else:
            self.terminal.draw_text(6, "Host: Connection failed!")
            return False
    
    def handle_keyboard(self):
        """Process keyboard input."""
        if not self.keyboard:
            return
        
        key = self.keyboard.read_key()
        if not key:
            return
        
        # Function keys -> send AID
        if key in AID_KEYS:
            print(f"Sending {key}")
            self.connection.send_key(key, self.screen)
            return
        
        # Navigation keys
        if key == 'UP':
            self.screen.set_cursor(
                max(0, self.screen.cursor_row - 1),
                self.screen.cursor_col
            )
        elif key == 'DOWN':
            self.screen.set_cursor(
                min(self.screen.rows - 1, self.screen.cursor_row + 1),
                self.screen.cursor_col
            )
        elif key == 'LEFT':
            self.screen.set_cursor(
                self.screen.cursor_row,
                max(0, self.screen.cursor_col - 1)
            )
        elif key == 'RIGHT':
            self.screen.set_cursor(
                self.screen.cursor_row,
                min(self.screen.cols - 1, self.screen.cursor_col + 1)
            )
        elif key == 'HOME':
            # Go to first input field
            field = self.screen.get_next_input_field(0, 0)
            if field:
                self.screen.set_cursor(field.row, field.col)
        elif key == 'TAB' or key == '\t':
            # Next field
            field = self.screen.get_next_input_field()
            if field:
                self.screen.set_cursor(field.row, field.col)
        
        # Backspace
        elif key == '\b':
            if self.screen.cursor_col > 0:
                self.screen.set_cursor(
                    self.screen.cursor_row,
                    self.screen.cursor_col - 1
                )
                self.screen.set_char(
                    self.screen.cursor_row,
                    self.screen.cursor_col,
                    ' '
                )
                # Mark field as modified
                field = self.screen.current_field
                if field and field.is_input:
                    field.modified = True
        
        # Enter key
        elif key == '\r':
            self.connection.send_key('ENTER', self.screen)
        
        # Printable characters
        elif len(key) == 1 and ord(key) >= 32:
            field = self.screen.current_field
            if field and field.is_input:
                self.screen.set_char(
                    self.screen.cursor_row,
                    self.screen.cursor_col,
                    key
                )
                field.modified = True
                self.screen.advance_cursor()
    
    def handle_serial_input(self):
        """Handle input from USB serial (for testing without keyboard)."""
        # Check for serial input
        if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
            char = sys.stdin.read(1)
            if char:
                # Similar handling as keyboard
                if char == '\r' or char == '\n':
                    self.connection.send_key('ENTER', self.screen)
                elif char == '\x1b':  # Escape sequence
                    # Could handle arrow keys, function keys here
                    pass
                elif ord(char) >= 32:
                    field = self.screen.current_field
                    if field and field.is_input:
                        self.screen.set_char(
                            self.screen.cursor_row,
                            self.screen.cursor_col,
                            char
                        )
                        field.modified = True
                        self.screen.advance_cursor()
    
    def process_host_data(self):
        """Receive and process data from host."""
        if not self.connection or not self.connection.connected:
            return
        
        data = self.connection.receive()
        if data:
            print(f"Received {len(data)} bytes from host")
            self.parser.parse(data)
            self.terminal.render_screen(self.screen)
    
    def update_status(self):
        """Update status line."""
        status = f"R:{self.screen.cursor_row+1:02d} C:{self.screen.cursor_col+1:02d}"
        
        if self.connection and self.connection.connected:
            status += " | Connected"
        else:
            status += " | Disconnected"
        
        if self.keyboard and self.keyboard.shift_pressed:
            status += " SHIFT"
        
        self.terminal.status_line(status)
    
    def run(self):
        """Main application loop."""
        print("\n=== TN5250 Emulator Starting ===\n")
        
        # Initialize hardware
        self.init_hardware()
        time.sleep(1)
        
        # Connect to WiFi
        if not self.connect_wifi():
            self.terminal.draw_text(10, "Press RESET to retry")
            return
        
        time.sleep(0.5)
        
        # Connect to host
        if not self.connect_host():
            self.terminal.draw_text(10, "Press RESET to retry")
            return
        
        # Clear screen for terminal
        time.sleep(1)
        self.terminal.clear()
        
        # Main loop
        self.running = True
        last_status = 0
        
        print("Entering main loop...")
        
        while self.running:
            try:
                # Process host data
                self.process_host_data()
                
                # Handle keyboard input
                self.handle_keyboard()
                
                # Update status line periodically
                now = time.ticks_ms()
                if time.ticks_diff(now, last_status) > 500:
                    self.update_status()
                    last_status = now
                
                # Small delay to prevent CPU hogging
                time.sleep_ms(10)
                
                # Check connection
                if not self.connection.connected:
                    self.terminal.draw_text(12, "Connection lost!")
                    self.terminal.draw_text(13, "Press RESET to reconnect")
                    break
                
            except KeyboardInterrupt:
                print("\nStopping...")
                self.running = False
            except Exception as e:
                print(f"Error: {e}")
                self.terminal.draw_text(12, f"Error: {e}")
        
        # Cleanup
        if self.connection:
            self.connection.disconnect()
        if self.wifi:
            self.wifi.disconnect()
        
        print("Emulator stopped.")


# Entry point
def main():
    """Application entry point."""
    emulator = TN5250Emulator()
    emulator.run()


if __name__ == "__main__":
    main()
