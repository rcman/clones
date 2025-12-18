# TN5250 Data Stream Parser
# Interprets 5250 commands and updates the screen buffer

from .commands import (
    ESC, CMD_WRITE_TO_DISPLAY, CMD_CLEAR_UNIT, CMD_ROLL,
    CMD_READ_INPUT_FIELDS, CMD_READ_MDT_FIELDS,
    SF, SBA, IC, RA, SOH,
    ATTR_NORMAL, ATTR_REVERSE, ATTR_HIGH_INTENSITY, ATTR_INVISIBLE,
    FFW_BYPASS, FFW_MDT,
    ebcdic_char, EBCDIC_TO_ASCII
)
from .screen import Screen, Field


class DataStreamParser:
    """
    Parses the 5250 data stream and updates the screen.
    
    The 5250 data stream consists of commands and orders that control
    what appears on the terminal display. This parser handles the most
    common commands used in typical AS/400 applications.
    """
    
    def __init__(self, screen):
        self.screen = screen
        self.debug = False  # Enable for verbose output
    
    def log(self, msg):
        """Debug logging."""
        if self.debug:
            print(f"[PARSER] {msg}")
    
    def parse(self, data):
        """
        Parse a 5250 data stream and update the screen.
        
        Args:
            data: bytes received from the host
        """
        if not data:
            return
        
        self.log(f"Parsing {len(data)} bytes")
        
        pos = 0
        while pos < len(data):
            byte = data[pos]
            
            # Check for escape sequence
            if byte == ESC:
                pos = self._handle_escape(data, pos)
            # Start of Header
            elif byte == SOH:
                pos = self._handle_soh(data, pos)
            # Other data - just display it
            else:
                # Regular display data at current position
                self._display_char(byte)
                pos += 1
    
    def _handle_escape(self, data, pos):
        """Handle ESC sequence (command follows)."""
        if pos + 1 >= len(data):
            return pos + 1
        
        cmd = data[pos + 1]
        self.log(f"ESC command: 0x{cmd:02X}")
        
        if cmd == CMD_WRITE_TO_DISPLAY:
            return self._handle_wtd(data, pos + 2)
        elif cmd == CMD_CLEAR_UNIT:
            self.screen.clear()
            return pos + 2
        elif cmd == CMD_ROLL:
            return self._handle_roll(data, pos + 2)
        else:
            self.log(f"Unknown ESC command: 0x{cmd:02X}")
            return pos + 2
    
    def _handle_wtd(self, data, pos):
        """
        Handle Write To Display command.
        
        WTD is the primary command for updating the screen. It contains
        a control character followed by orders and data.
        """
        if pos >= len(data):
            return pos
        
        # Control character (CC)
        cc = data[pos]
        pos += 1
        
        self.log(f"WTD Control: 0x{cc:02X}")
        
        # Check control character flags
        if cc & 0x40:  # Reset MDT
            for field in self.screen.fields:
                field.modified = False
        
        if cc & 0x20:  # Clear unit
            self.screen.clear()
        
        # Process orders and data
        while pos < len(data):
            byte = data[pos]
            
            # Set Buffer Address (most common order)
            if byte == SBA:
                pos = self._handle_sba(data, pos + 1)
            # Start Field
            elif byte == SF:
                pos = self._handle_sf(data, pos + 1)
            # Insert Cursor
            elif byte == IC:
                pos = self._handle_ic(data, pos + 1)
            # Repeat to Address
            elif byte == RA:
                pos = self._handle_ra(data, pos + 1)
            # Escape - new command
            elif byte == ESC:
                break
            # SOH - end of WTD data
            elif byte == SOH:
                break
            # Regular character data
            else:
                self._display_char(byte)
                pos += 1
        
        return pos
    
    def _handle_sba(self, data, pos):
        """
        Handle Set Buffer Address order.
        
        Sets the current buffer position for subsequent data.
        Address is a 2-byte value encoding row and column.
        """
        if pos + 1 >= len(data):
            return len(data)
        
        # Decode 5250 address format
        addr = (data[pos] << 8) | data[pos + 1]
        row, col = self._decode_address(addr)
        
        self.log(f"SBA: row={row}, col={col}")
        self.screen.set_cursor(row, col)
        
        return pos + 2
    
    def _handle_sf(self, data, pos):
        """
        Handle Start Field order.
        
        Defines an input or output field on the screen.
        """
        if pos + 1 >= len(data):
            return len(data)
        
        # Field Format Word (FFW)
        ffw1 = data[pos]
        ffw2 = data[pos + 1] if pos + 1 < len(data) else 0
        pos += 2
        
        # Field length - scan for next field or end of line
        # For simplicity, we'll use a default or scan ahead
        field_len = self._scan_field_length(data, pos)
        
        # Create field at current cursor position
        field = Field(
            row=self.screen.cursor_row,
            col=self.screen.cursor_col,
            length=field_len,
            ffw=ffw1,
            attr=ffw2
        )
        
        self.screen.add_field(field)
        self.log(f"SF: {field}")
        
        # Display attribute byte marks field start (usually not displayed)
        self.screen.advance_cursor()
        
        return pos
    
    def _handle_ic(self, data, pos):
        """
        Handle Insert Cursor order.
        
        Positions the cursor at the specified address.
        """
        if pos + 1 >= len(data):
            return len(data)
        
        addr = (data[pos] << 8) | data[pos + 1]
        row, col = self._decode_address(addr)
        
        self.log(f"IC: row={row}, col={col}")
        self.screen.set_cursor(row, col)
        
        return pos + 2
    
    def _handle_ra(self, data, pos):
        """
        Handle Repeat to Address order.
        
        Repeats a character from current position to specified address.
        """
        if pos + 2 >= len(data):
            return len(data)
        
        addr = (data[pos] << 8) | data[pos + 1]
        char = data[pos + 2]
        
        end_row, end_col = self._decode_address(addr)
        end_pos = end_row * self.screen.cols + end_col
        
        self.log(f"RA: to ({end_row},{end_col}), char=0x{char:02X}")
        
        # Fill from current position to end address
        cur_pos = self.screen.cursor_row * self.screen.cols + self.screen.cursor_col
        
        while cur_pos < end_pos:
            row = cur_pos // self.screen.cols
            col = cur_pos % self.screen.cols
            self._display_char_at(row, col, char)
            cur_pos += 1
        
        # Update cursor
        self.screen.set_cursor(end_row, end_col)
        
        return pos + 3
    
    def _handle_soh(self, data, pos):
        """
        Handle Start of Header.
        
        Contains control information about the screen format.
        """
        if pos + 1 >= len(data):
            return pos + 1
        
        length = data[pos + 1]
        self.log(f"SOH length: {length}")
        
        # Skip header data for now
        return pos + 2 + length
    
    def _handle_roll(self, data, pos):
        """
        Handle Roll (scroll) command.
        """
        if pos + 1 >= len(data):
            return len(data)
        
        direction = data[pos]
        lines = data[pos + 1] if pos + 1 < len(data) else 1
        
        self.log(f"ROLL: direction={direction}, lines={lines}")
        
        # TODO: Implement scrolling
        
        return pos + 2
    
    def _decode_address(self, addr):
        """
        Decode a 5250 buffer address to row and column.
        
        The address format depends on screen size:
        - 24x80: row = addr // 80, col = addr % 80
        - Address may also be encoded differently in some cases
        """
        # Simple linear address decode
        row = (addr >> 8) & 0xFF
        col = addr & 0xFF
        
        # Adjust for 1-based addressing if needed
        if row > 0:
            row -= 1
        if col > 0:
            col -= 1
        
        # Clamp to screen bounds
        row = max(0, min(row, self.screen.rows - 1))
        col = max(0, min(col, self.screen.cols - 1))
        
        return row, col
    
    def _display_char(self, ebcdic_byte):
        """Display a character at current cursor position and advance."""
        ascii_val = EBCDIC_TO_ASCII[ebcdic_byte]
        self.screen.set_char(
            self.screen.cursor_row,
            self.screen.cursor_col,
            ascii_val
        )
        self.screen.advance_cursor()
    
    def _display_char_at(self, row, col, ebcdic_byte):
        """Display a character at specific position."""
        ascii_val = EBCDIC_TO_ASCII[ebcdic_byte]
        self.screen.set_char(row, col, ascii_val)
    
    def _scan_field_length(self, data, pos):
        """
        Scan ahead to determine field length.
        
        Returns the number of characters until the next field
        or end of data.
        """
        length = 0
        while pos + length < len(data):
            byte = data[pos + length]
            if byte == SF or byte == ESC or byte == SOH:
                break
            length += 1
            if length > 132:  # Max reasonable field length
                break
        
        return max(1, length)
