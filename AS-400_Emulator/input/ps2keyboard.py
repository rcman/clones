# PS/2 Keyboard Driver for MicroPython
# Handles PS/2 keyboard input on Raspberry Pi Pico

from machine import Pin
import time


# PS/2 Scan Code Set 2 to ASCII mapping
# Standard US keyboard layout
SCAN_TO_ASCII = {
    0x1C: 'a', 0x32: 'b', 0x21: 'c', 0x23: 'd', 0x24: 'e',
    0x2B: 'f', 0x34: 'g', 0x33: 'h', 0x43: 'i', 0x3B: 'j',
    0x42: 'k', 0x4B: 'l', 0x3A: 'm', 0x31: 'n', 0x44: 'o',
    0x4D: 'p', 0x15: 'q', 0x2D: 'r', 0x1B: 's', 0x2C: 't',
    0x3C: 'u', 0x2A: 'v', 0x1D: 'w', 0x22: 'x', 0x35: 'y',
    0x1A: 'z',
    0x45: '0', 0x16: '1', 0x1E: '2', 0x26: '3', 0x25: '4',
    0x2E: '5', 0x36: '6', 0x3D: '7', 0x3E: '8', 0x46: '9',
    0x29: ' ',   # Space
    0x5A: '\r',  # Enter
    0x66: '\b',  # Backspace
    0x0D: '\t',  # Tab
    0x76: '\x1b', # Escape
    0x49: '.', 0x41: ',', 0x4C: ';', 0x52: "'",
    0x54: '[', 0x5B: ']', 0x4E: '-', 0x55: '=',
    0x5D: '\\', 0x4A: '/', 0x0E: '`',
}

# Shifted characters
SCAN_TO_ASCII_SHIFT = {
    0x1C: 'A', 0x32: 'B', 0x21: 'C', 0x23: 'D', 0x24: 'E',
    0x2B: 'F', 0x34: 'G', 0x33: 'H', 0x43: 'I', 0x3B: 'J',
    0x42: 'K', 0x4B: 'L', 0x3A: 'M', 0x31: 'N', 0x44: 'O',
    0x4D: 'P', 0x15: 'Q', 0x2D: 'R', 0x1B: 'S', 0x2C: 'T',
    0x3C: 'U', 0x2A: 'V', 0x1D: 'W', 0x22: 'X', 0x35: 'Y',
    0x1A: 'Z',
    0x45: ')', 0x16: '!', 0x1E: '@', 0x26: '#', 0x25: '$',
    0x2E: '%', 0x36: '^', 0x3D: '&', 0x3E: '*', 0x46: '(',
    0x49: '>', 0x41: '<', 0x4C: ':', 0x52: '"',
    0x54: '{', 0x5B: '}', 0x4E: '_', 0x55: '+',
    0x5D: '|', 0x4A: '?', 0x0E: '~',
}

# Function keys (return special strings)
SCAN_TO_FKEY = {
    0x05: 'F1', 0x06: 'F2', 0x04: 'F3', 0x0C: 'F4',
    0x03: 'F5', 0x0B: 'F6', 0x83: 'F7', 0x0A: 'F8',
    0x01: 'F9', 0x09: 'F10', 0x78: 'F11', 0x07: 'F12',
}

# Extended keys (prefixed with E0)
EXTENDED_KEYS = {
    0x75: 'UP', 0x72: 'DOWN', 0x6B: 'LEFT', 0x74: 'RIGHT',
    0x6C: 'HOME', 0x69: 'END', 0x7D: 'PAGEUP', 0x7A: 'PAGEDOWN',
    0x70: 'INSERT', 0x71: 'DELETE',
}


class PS2Keyboard:
    """
    PS/2 Keyboard driver for MicroPython.
    
    Connects to a PS/2 keyboard via two GPIO pins (clock and data).
    Uses interrupt-driven reception for reliability.
    """
    
    def __init__(self, clock_pin, data_pin):
        """
        Initialize the PS/2 keyboard interface.
        
        Args:
            clock_pin: GPIO pin number for clock (e.g., 14)
            data_pin: GPIO pin number for data (e.g., 15)
        """
        self.clock = Pin(clock_pin, Pin.IN, Pin.PULL_UP)
        self.data = Pin(data_pin, Pin.IN, Pin.PULL_UP)
        
        # Reception state
        self._bit_count = 0
        self._scancode = 0
        self._parity = 0
        
        # Scancode buffer
        self._buffer = []
        
        # Key state
        self._shift = False
        self._ctrl = False
        self._alt = False
        self._extended = False
        self._release = False
        
        # Key buffer (processed characters/keys)
        self._key_buffer = []
        
        # Set up clock interrupt (falling edge)
        self.clock.irq(trigger=Pin.IRQ_FALLING, handler=self._clock_isr)
    
    def _clock_isr(self, pin):
        """
        Clock interrupt service routine.
        
        Called on each falling edge of the clock signal.
        PS/2 sends 11 bits: start(0), 8 data bits, parity, stop(1)
        """
        bit = self.data.value()
        
        if self._bit_count == 0:
            # Start bit (should be 0)
            if bit == 0:
                self._scancode = 0
                self._parity = 0
                self._bit_count = 1
        elif self._bit_count <= 8:
            # Data bits (LSB first)
            self._scancode |= (bit << (self._bit_count - 1))
            self._parity ^= bit
            self._bit_count += 1
        elif self._bit_count == 9:
            # Parity bit (odd parity)
            self._parity ^= bit
            self._bit_count += 1
        elif self._bit_count == 10:
            # Stop bit (should be 1)
            if bit == 1 and self._parity == 1:
                # Valid scancode received
                self._buffer.append(self._scancode)
            self._bit_count = 0
    
    def _process_scancode(self, code):
        """
        Process a scancode and update key state.
        
        Returns a key string or None.
        """
        # Extended key prefix
        if code == 0xE0:
            self._extended = True
            return None
        
        # Release prefix
        if code == 0xF0:
            self._release = True
            return None
        
        # Handle modifier keys
        if code == 0x12 or code == 0x59:  # Left/Right Shift
            self._shift = not self._release
            self._release = False
            self._extended = False
            return None
        
        if code == 0x14:  # Ctrl
            self._ctrl = not self._release
            self._release = False
            self._extended = False
            return None
        
        if code == 0x11:  # Alt
            self._alt = not self._release
            self._release = False
            self._extended = False
            return None
        
        # Skip release events for regular keys
        if self._release:
            self._release = False
            self._extended = False
            return None
        
        # Handle extended keys
        if self._extended:
            self._extended = False
            if code in EXTENDED_KEYS:
                return EXTENDED_KEYS[code]
            return None
        
        # Function keys
        if code in SCAN_TO_FKEY:
            return SCAN_TO_FKEY[code]
        
        # Regular keys
        if self._shift:
            if code in SCAN_TO_ASCII_SHIFT:
                return SCAN_TO_ASCII_SHIFT[code]
        
        if code in SCAN_TO_ASCII:
            char = SCAN_TO_ASCII[code]
            # Handle Ctrl combinations
            if self._ctrl and char.isalpha():
                return f"CTRL+{char.upper()}"
            return char
        
        return None
    
    def update(self):
        """
        Process any received scancodes.
        
        Call this regularly in your main loop.
        """
        while self._buffer:
            code = self._buffer.pop(0)
            key = self._process_scancode(code)
            if key:
                self._key_buffer.append(key)
    
    def available(self):
        """Check if any keys are available."""
        self.update()
        return len(self._key_buffer) > 0
    
    def read_key(self):
        """
        Read a key from the buffer.
        
        Returns a string (character, 'F1'-'F12', 'UP', 'DOWN', etc.)
        or None if no key is available.
        """
        self.update()
        if self._key_buffer:
            return self._key_buffer.pop(0)
        return None
    
    def read_char(self):
        """
        Read a single character from the buffer.
        
        Returns only printable characters or control codes.
        Returns None for function/arrow keys.
        """
        key = self.read_key()
        if key and len(key) == 1:
            return key
        return None
    
    @property
    def shift_pressed(self):
        """Check if Shift is currently pressed."""
        return self._shift
    
    @property
    def ctrl_pressed(self):
        """Check if Ctrl is currently pressed."""
        return self._ctrl
    
    @property
    def alt_pressed(self):
        """Check if Alt is currently pressed."""
        return self._alt
