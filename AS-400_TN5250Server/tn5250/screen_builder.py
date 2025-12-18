# 5250 Screen Builder
# Constructs 5250 data streams to send to terminal clients

from .protocol import (
    ESC, CMD_WRITE_TO_DISPLAY, CMD_CLEAR_UNIT,
    SBA, SF, IC, RA, SOH,
    WCC_RESET_MDT, WCC_RESET_KEYBOARD,
    FFW_BYPASS, ATTR_NORMAL, ATTR_HIGH_INTENSITY, ATTR_REVERSE,
    ATTR_UNDERSCORE, ATTR_INVISIBLE,
    ascii_to_ebcdic_bytes, IAC, EOR
)


class ScreenBuilder:
    """
    Builds 5250 data streams for sending screens to terminals.
    
    This creates the byte sequences that tell a 5250 terminal
    what to display, where to place fields, etc.
    """
    
    def __init__(self, rows=24, cols=80):
        self.rows = rows
        self.cols = cols
        self.data = bytearray()
        self.fields = []  # Track field positions for response parsing
    
    def clear(self):
        """Start fresh with a clear screen command."""
        self.data = bytearray()
        self.fields = []
        # Clear unit and format table
        self.data.append(ESC)
        self.data.append(CMD_CLEAR_UNIT)
        return self
    
    def write_to_display(self, wcc=None):
        """
        Start a Write To Display command.
        
        Args:
            wcc: Write Control Character (default: reset MDT + unlock keyboard)
        """
        if wcc is None:
            wcc = WCC_RESET_MDT | WCC_RESET_KEYBOARD
        
        self.data.append(ESC)
        self.data.append(CMD_WRITE_TO_DISPLAY)
        self.data.append(wcc)
        return self
    
    def set_buffer_address(self, row, col):
        """
        Set the buffer address (cursor position for next data).
        
        Args:
            row: Row number (1-based)
            col: Column number (1-based)
        """
        self.data.append(SBA)
        self.data.append(row)
        self.data.append(col)
        return self
    
    def sba(self, row, col):
        """Alias for set_buffer_address."""
        return self.set_buffer_address(row, col)
    
    def write_text(self, text, row=None, col=None):
        """
        Write text at current or specified position.
        
        Args:
            text: ASCII text to display
            row: Optional row (1-based)
            col: Optional column (1-based)
        """
        if row is not None and col is not None:
            self.set_buffer_address(row, col)
        
        # Convert to EBCDIC and add to stream
        self.data.extend(ascii_to_ebcdic_bytes(text))
        return self
    
    def text(self, row, col, text):
        """Convenience method: write text at position."""
        return self.write_text(text, row, col)
    
    def start_field(self, row, col, length, 
                    protected=True, 
                    attr=ATTR_NORMAL,
                    hidden=False,
                    highlight=False):
        """
        Define a field on the screen.
        
        Args:
            row: Row number (1-based)
            col: Column number (1-based) 
            length: Field length in characters
            protected: True for output-only, False for input
            attr: Display attribute
            hidden: True for password fields
            highlight: True for high intensity
        """
        self.set_buffer_address(row, col)
        
        # Start Field order
        self.data.append(SF)
        
        # Field Format Word byte 1
        ffw1 = 0x00
        if protected:
            ffw1 |= FFW_BYPASS
        
        # Field Format Word byte 2 (display attribute)
        if hidden:
            ffw2 = ATTR_INVISIBLE
        elif highlight:
            ffw2 = ATTR_HIGH_INTENSITY
        else:
            ffw2 = attr
        
        self.data.append(ffw1)
        self.data.append(ffw2)
        
        # Track field for response parsing
        if not protected:
            self.fields.append({
                'row': row,
                'col': col + 1,  # Data starts after attribute byte
                'length': length,
                'name': f"field_{len(self.fields)}"
            })
        
        return self
    
    def input_field(self, row, col, length, hidden=False, name=None):
        """
        Create an input field.
        
        Args:
            row: Row (1-based)
            col: Column (1-based)
            length: Field length
            hidden: True for password fields
            name: Optional field name for identification
        """
        self.start_field(row, col, length, protected=False, hidden=hidden)
        
        # Update field name if provided
        if name and self.fields:
            self.fields[-1]['name'] = name
        
        # Fill field with underscores to show extent (optional visual)
        # Actually, just leave blank - the terminal handles this
        
        return self
    
    def output_field(self, row, col, text, highlight=False, reverse=False):
        """
        Create a protected output field with text.
        
        Args:
            row: Row (1-based)
            col: Column (1-based)
            text: Text to display
            highlight: High intensity
            reverse: Reverse video
        """
        attr = ATTR_NORMAL
        if highlight:
            attr = ATTR_HIGH_INTENSITY
        if reverse:
            attr = ATTR_REVERSE
        
        self.start_field(row, col, len(text), protected=True, attr=attr)
        self.data.extend(ascii_to_ebcdic_bytes(text))
        return self
    
    def insert_cursor(self, row, col):
        """
        Position the cursor.
        
        Args:
            row: Row (1-based)
            col: Column (1-based)
        """
        self.data.append(IC)
        self.data.append(row)
        self.data.append(col)
        return self
    
    def cursor(self, row, col):
        """Alias for insert_cursor."""
        return self.insert_cursor(row, col)
    
    def repeat_to_address(self, row, col, char=' '):
        """
        Repeat a character from current position to specified address.
        Useful for drawing lines or clearing areas.
        """
        self.data.append(RA)
        self.data.append(row)
        self.data.append(col)
        self.data.extend(ascii_to_ebcdic_bytes(char))
        return self
    
    def horizontal_line(self, row, col_start, col_end, char='-'):
        """Draw a horizontal line."""
        self.set_buffer_address(row, col_start)
        line = char * (col_end - col_start + 1)
        self.data.extend(ascii_to_ebcdic_bytes(line))
        return self
    
    def center_text(self, row, text, highlight=False):
        """
        Write centered text on a row.
        
        Args:
            row: Row number (1-based)
            text: Text to center
            highlight: Use high intensity
        """
        col = (self.cols - len(text)) // 2 + 1
        if col < 1:
            col = 1
        
        if highlight:
            self.output_field(row, col, text, highlight=True)
        else:
            self.text(row, col, text)
        return self
    
    def right_text(self, row, text, margin=1):
        """Write right-aligned text."""
        col = self.cols - len(text) - margin + 1
        self.text(row, col, text)
        return self
    
    def box(self, top, left, bottom, right, title=None):
        """
        Draw a box with optional title.
        
        Uses simple ASCII characters for compatibility.
        """
        # Top line
        self.text(top, left, '+' + '-' * (right - left - 1) + '+')
        
        # Side lines
        for row in range(top + 1, bottom):
            self.text(row, left, '|')
            self.text(row, right, '|')
        
        # Bottom line
        self.text(bottom, left, '+' + '-' * (right - left - 1) + '+')
        
        # Title
        if title:
            title_text = f"[ {title} ]"
            title_col = left + (right - left - len(title_text)) // 2
            self.text(top, title_col, title_text)
        
        return self
    
    def build(self):
        """
        Finalize and return the complete data stream.
        
        Wraps the data with IAC EOR for Telnet transmission.
        """
        result = bytes(self.data) + bytes([IAC, EOR])
        return result
    
    def get_fields(self):
        """Get list of input field definitions."""
        return self.fields.copy()


class ScreenTemplates:
    """Pre-built screen templates for common AS/400-style displays."""
    
    @staticmethod
    def signon_screen(system_name="PICO400", subsystem="QINTER"):
        """
        Build the classic AS/400 sign-on screen.
        """
        s = ScreenBuilder()
        s.clear()
        s.write_to_display()
        
        # Header
        s.center_text(1, f"Sign On", highlight=True)
        s.text(2, 2, f"System . . . . . :   {system_name}")
        s.text(3, 2, f"Subsystem  . . . :   {subsystem}")
        s.text(4, 2, f"Display  . . . . :   QPADEV0001")
        
        # Sign-on box
        s.box(6, 5, 16, 75, "Sign On")
        
        # Fields
        s.text(8, 10, "User  . . . . . . . . . . . .")
        s.input_field(8, 42, 10, name="user")
        
        s.text(9, 10, "Password  . . . . . . . . . .")
        s.input_field(9, 42, 10, hidden=True, name="password")
        
        s.text(10, 10, "Program/procedure . . . . . .")
        s.input_field(10, 42, 10, name="program")
        
        s.text(11, 10, "Menu  . . . . . . . . . . . .")
        s.input_field(11, 42, 10, name="menu")
        
        s.text(12, 10, "Current library . . . . . . .")
        s.input_field(12, 42, 10, name="curlib")
        
        # Footer
        s.text(18, 2, "(C) COPYRIGHT PICO SYSTEMS 2025")
        
        # Function key legend
        s.text(22, 2, "F3=Exit   F12=Cancel")
        
        # Position cursor at username
        s.cursor(8, 43)
        
        return s.build(), s.get_fields()
    
    @staticmethod
    def main_menu(system_name="PICO400", user="GUEST"):
        """Build a main menu screen."""
        s = ScreenBuilder()
        s.clear()
        s.write_to_display()
        
        # Header
        s.center_text(1, f"{system_name} - MAIN MENU", highlight=True)
        s.right_text(2, f"User: {user}")
        
        s.text(3, 2, "Select one of the following:")
        
        # Menu options
        options = [
            ("1", "Work with messages"),
            ("2", "Work with files"),
            ("3", "Work with output queue"),
            ("4", "Display job status"),
            ("5", "System status"),
            ("6", "Display active jobs"),
            ("7", "Work with spool files"),
            ("8", "About this system"),
            ("90", "Sign off"),
        ]
        
        row = 5
        for opt, desc in options:
            s.text(row, 7, f"{opt}.")
            s.text(row, 12, desc)
            row += 1
        
        # Command line
        s.text(19, 2, "Selection or command")
        s.text(20, 2, "===>")
        s.input_field(20, 7, 70, name="command")
        
        # Function keys
        s.text(22, 2, "F3=Exit   F4=Prompt   F9=Retrieve   F12=Cancel")
        s.text(23, 2, "F13=Info assistant   F23=Set initial menu")
        
        s.cursor(20, 8)
        
        return s.build(), s.get_fields()
    
    @staticmethod
    def message_screen(title, message, msg_type="info"):
        """
        Build a message display screen.
        
        Args:
            title: Message title
            message: Message text (can be multi-line list)
            msg_type: 'info', 'warning', or 'error'
        """
        s = ScreenBuilder()
        s.clear()
        s.write_to_display()
        
        # Title
        highlight = msg_type != "info"
        s.center_text(1, title, highlight=highlight)
        
        # Message box
        s.box(4, 3, 18, 77)
        
        # Message content
        if isinstance(message, str):
            message = message.split('\n')
        
        row = 6
        for line in message[:10]:  # Max 10 lines
            s.text(row, 6, line[:68])
            row += 1
        
        # Press Enter prompt
        s.text(20, 2, "Press Enter to continue.")
        
        s.cursor(20, 27)
        
        return s.build(), s.get_fields()
    
    @staticmethod  
    def about_screen(system_name="PICO400"):
        """Build an about/system info screen."""
        s = ScreenBuilder()
        s.clear()
        s.write_to_display()
        
        s.center_text(1, "About This System", highlight=True)
        
        s.box(3, 10, 18, 70, "System Information")
        
        info = [
            f"System Name . . . . : {system_name}",
            "",
            "Hardware:",
            "  Model . . . . . . : Raspberry Pi Pico 2 W",
            "  Processor . . . . : RP2350 Dual ARM Cortex-M33",
            "  Memory  . . . . . : 520 KB SRAM",
            "",
            "Software:",
            "  OS  . . . . . . . : MicroPython",
            "  Emulator  . . . . : Fake AS/400 TN5250 Server",
            "  Version . . . . . : 1.0.0",
            "",
            "This is a TN5250 server that emulates the look",
            "and feel of an IBM AS/400 (iSeries) system.",
        ]
        
        row = 4
        for line in info:
            s.text(row, 12, line)
            row += 1
        
        s.text(20, 2, "F3=Exit   F12=Cancel")
        s.cursor(20, 50)
        
        return s.build(), s.get_fields()
    
    @staticmethod
    def command_entry(system_name="PICO400"):
        """Build a command entry screen."""
        s = ScreenBuilder()
        s.clear()
        s.write_to_display()
        
        s.center_text(1, f"{system_name} Command Entry", highlight=True)
        
        s.text(4, 2, "Type command, press Enter.")
        s.text(6, 2, "===>")
        s.input_field(6, 7, 70, name="command")
        
        s.text(20, 2, "F3=Exit   F4=Prompt   F9=Retrieve   F12=Cancel")
        
        s.cursor(6, 8)
        
        return s.build(), s.get_fields()
    
    @staticmethod
    def error_screen(error_msg, detail=""):
        """Build an error display screen."""
        s = ScreenBuilder()
        s.clear()
        s.write_to_display()
        
        s.center_text(1, "* * *  E R R O R  * * *", highlight=True)
        
        s.box(5, 5, 15, 75)
        
        s.text(7, 10, error_msg[:60])
        if detail:
            s.text(9, 10, detail[:60])
        
        s.text(12, 10, "Press F3 to exit or Enter to continue.")
        
        s.text(20, 2, "F3=Exit   F12=Cancel")
        s.cursor(12, 50)
        
        return s.build(), s.get_fields()
