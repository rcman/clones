# ILI9341 Display Driver with Font Rendering
# Optimized for TN5250 terminal emulation on MicroPython

from machine import Pin, SPI
import time


# ILI9341 Commands
CMD_SWRESET = 0x01
CMD_SLPOUT = 0x11
CMD_DISPON = 0x29
CMD_CASET = 0x2A
CMD_PASET = 0x2B
CMD_RAMWR = 0x2C
CMD_MADCTL = 0x36
CMD_COLMOD = 0x3A

MADCTL_MY = 0x80
MADCTL_MX = 0x40
MADCTL_MV = 0x20
MADCTL_BGR = 0x08


class ILI9341:
    """ILI9341 320x240 display driver for MicroPython."""
    
    def __init__(self, spi, cs_pin, dc_pin, rst_pin, width=320, height=240):
        self.spi = spi
        self.cs = Pin(cs_pin, Pin.OUT, value=1)
        self.dc = Pin(dc_pin, Pin.OUT, value=0)
        self.rst = Pin(rst_pin, Pin.OUT, value=1)
        self.width = width
        self.height = height
        self._init_display()
    
    def _init_display(self):
        """Initialize the display controller."""
        self.rst.value(0)
        time.sleep_ms(50)
        self.rst.value(1)
        time.sleep_ms(150)
        
        self._cmd(CMD_SWRESET)
        time.sleep_ms(150)
        self._cmd(CMD_SLPOUT)
        time.sleep_ms(150)
        self._cmd(CMD_COLMOD, bytes([0x55]))  # 16-bit color
        self._cmd(CMD_MADCTL, bytes([MADCTL_MX | MADCTL_BGR]))
        self._cmd(CMD_DISPON)
        time.sleep_ms(100)
    
    def _cmd(self, cmd, data=None):
        """Send command and optional data."""
        self.dc.value(0)
        self.cs.value(0)
        self.spi.write(bytes([cmd]))
        self.cs.value(1)
        if data:
            self.dc.value(1)
            self.cs.value(0)
            self.spi.write(data)
            self.cs.value(1)
    
    def _set_window(self, x0, y0, x1, y1):
        """Set drawing window."""
        self._cmd(CMD_CASET, bytes([x0 >> 8, x0, x1 >> 8, x1]))
        self._cmd(CMD_PASET, bytes([y0 >> 8, y0, y1 >> 8, y1]))
        self._cmd(CMD_RAMWR)
    
    def fill(self, color):
        """Fill screen with color."""
        self._set_window(0, 0, self.width - 1, self.height - 1)
        hi, lo = color >> 8, color & 0xFF
        chunk = bytes([hi, lo] * 256)
        self.dc.value(1)
        self.cs.value(0)
        for _ in range(self.width * self.height // 256):
            self.spi.write(chunk)
        self.cs.value(1)
    
    def fill_rect(self, x, y, w, h, color):
        """Fill rectangle with color."""
        self._set_window(x, y, x + w - 1, y + h - 1)
        hi, lo = color >> 8, color & 0xFF
        data = bytes([hi, lo] * w)
        self.dc.value(1)
        self.cs.value(0)
        for _ in range(h):
            self.spi.write(data)
        self.cs.value(1)


# Compact 4x6 font (fits 80x40 chars on 320x240)
# Each character stored as 4 bytes (columns), 6 rows each
FONT_4X6 = {}

# Generate basic ASCII font data
_font_raw = {
    ' ': 0x000000, '!': 0x005F00, '"': 0x030003, '#': 0x147F14,
    '$': 0x246A12, '%': 0x130864, '&': 0x365950, "'": 0x000300,
    '(': 0x001C22, ')': 0x22001C, '*': 0x0A1C0A, '+': 0x081C08,
    ',': 0x503000, '-': 0x080808, '.': 0x300000, '/': 0x201008,
    '0': 0x3E413E, '1': 0x427F40, '2': 0x625946, '3': 0x224D31,
    '4': 0x181412, '5': 0x274539, '6': 0x3C4A30, '7': 0x017109,
    '8': 0x364936, '9': 0x06291E, ':': 0x003600, ';': 0x563600,
    '<': 0x081422, '=': 0x141414, '>': 0x221408, '?': 0x020906,
    '@': 0x324979, 
    'A': 0x7E117E, 'B': 0x7F4936, 'C': 0x3E4122, 'D': 0x7F221C,
    'E': 0x7F4941, 'F': 0x7F0901, 'G': 0x3E4132, 'H': 0x7F087F,
    'I': 0x417F41, 'J': 0x20413F, 'K': 0x7F1422, 'L': 0x7F4040,
    'M': 0x7F027F, 'N': 0x7F087F, 'O': 0x3E413E, 'P': 0x7F0906,
    'Q': 0x3E215E, 'R': 0x7F1946, 'S': 0x264931, 'T': 0x017F01,
    'U': 0x3F403F, 'V': 0x1F201F, 'W': 0x7F207F, 'X': 0x361436,
    'Y': 0x074807, 'Z': 0x615149,
    '[': 0x007F41, '\\': 0x020408, ']': 0x41007F, '^': 0x040201,
    '_': 0x404040, '`': 0x010200, 
    'a': 0x205478, 'b': 0x7F4438, 'c': 0x384428, 'd': 0x38447F,
    'e': 0x385418, 'f': 0x087E09, 'g': 0x08543C, 'h': 0x7F0878,
    'i': 0x447D40, 'j': 0x40443D, 'k': 0x7F1028, 'l': 0x417F40,
    'm': 0x7C187C, 'n': 0x7C0878, 'o': 0x384438, 'p': 0x7C1408,
    'q': 0x08147C, 'r': 0x7C0804, 's': 0x485420, 't': 0x043F44,
    'u': 0x3C407C, 'v': 0x1C201C, 'w': 0x3C403C, 'x': 0x442844,
    'y': 0x0C503C, 'z': 0x446454,
    '{': 0x083641, '|': 0x007700, '}': 0x413608, '~': 0x040804,
}

# Convert to byte arrays
for ch, val in _font_raw.items():
    FONT_4X6[ch] = bytes([(val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF, 0])


class TerminalDisplay:
    """Terminal display for 5250 emulation with character rendering."""
    
    # Colors (RGB565)
    GREEN = 0x07E0
    BLACK = 0x0000
    WHITE = 0xFFFF
    RED = 0xF800
    YELLOW = 0xFFE0
    CYAN = 0x07FF
    
    def __init__(self, display, rows=24, cols=80, fg=0x07E0, bg=0x0000):
        self.display = display
        self.rows = rows
        self.cols = cols
        self.fg = fg
        self.bg = bg
        
        # Calculate cell size for display
        # 320/80 = 4 pixels wide, 240/24 = 10 pixels tall
        self.cell_w = display.width // cols
        self.cell_h = display.height // rows
        
        # Screen buffer for dirty tracking
        self.buffer = [[' '] * cols for _ in range(rows)]
        self.attrs = [[0] * cols for _ in range(rows)]
        
        self.cursor_row = 0
        self.cursor_col = 0
        self.cursor_visible = True
    
    def clear(self):
        """Clear the display."""
        self.display.fill(self.bg)
        for r in range(self.rows):
            for c in range(self.cols):
                self.buffer[r][c] = ' '
                self.attrs[r][c] = 0
    
    def draw_char(self, row, col, char, inverse=False):
        """Draw a single character."""
        if row < 0 or row >= self.rows or col < 0 or col >= self.cols:
            return
        
        # Check if changed
        attr = 1 if inverse else 0
        if self.buffer[row][col] == char and self.attrs[row][col] == attr:
            return
        
        self.buffer[row][col] = char
        self.attrs[row][col] = attr
        
        x = col * self.cell_w
        y = row * self.cell_h
        
        fg = self.bg if inverse else self.fg
        bg = self.fg if inverse else self.bg
        
        # Clear cell
        self.display.fill_rect(x, y, self.cell_w, self.cell_h, bg)
        
        # Get font data
        font = FONT_4X6.get(char, FONT_4X6.get(' '))
        if not font:
            return
        
        # Draw character pixels
        self.display._set_window(x, y, x + self.cell_w - 1, y + self.cell_h - 1)
        
        pixels = []
        fg_hi, fg_lo = fg >> 8, fg & 0xFF
        bg_hi, bg_lo = bg >> 8, bg & 0xFF
        
        for py in range(self.cell_h):
            font_y = (py * 6) // self.cell_h  # Scale 6 rows to cell_h
            for px in range(self.cell_w):
                font_x = (px * 3) // self.cell_w  # Scale 3 cols to cell_w
                if font_x < 3 and font_y < 6:
                    bit = (font[font_x] >> font_y) & 1
                    if bit:
                        pixels.extend([fg_hi, fg_lo])
                    else:
                        pixels.extend([bg_hi, bg_lo])
                else:
                    pixels.extend([bg_hi, bg_lo])
        
        self.display.dc.value(1)
        self.display.cs.value(0)
        self.display.spi.write(bytes(pixels))
        self.display.cs.value(1)
    
    def draw_text(self, row, text, start_col=0, inverse=False):
        """Draw a string of text."""
        for i, char in enumerate(text):
            col = start_col + i
            if col >= self.cols:
                break
            self.draw_char(row, col, char, inverse)
    
    def draw_cursor(self, row, col, visible=True):
        """Draw or erase cursor at position."""
        if 0 <= row < self.rows and 0 <= col < self.cols:
            char = self.buffer[row][col]
            self.draw_char(row, col, char, inverse=visible)
    
    def render_screen(self, screen):
        """Render a Screen object to the display."""
        for row in range(min(self.rows, screen.rows)):
            for col in range(min(self.cols, screen.cols)):
                char = chr(screen.get_char(row, col))
                if char < ' ' or char > '~':
                    char = ' '
                
                # Check if position is in a protected field
                field = screen.get_field_at(row, col)
                inverse = field and not field.is_input
                
                self.draw_char(row, col, char, inverse)
        
        # Draw cursor
        if self.cursor_visible:
            self.draw_cursor(screen.cursor_row, screen.cursor_col, True)
    
    def status_line(self, text):
        """Draw status line at bottom of screen."""
        row = self.rows - 1
        # Clear line
        for col in range(self.cols):
            self.draw_char(row, col, ' ', inverse=True)
        # Draw text
        self.draw_text(row, text[:self.cols], 0, inverse=True)
