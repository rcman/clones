# Screen Buffer Management for TN5250 Emulator
# Handles the 24x80 character buffer, fields, and cursor

from .commands import (
    ATTR_NORMAL, ATTR_REVERSE, ATTR_HIGH_INTENSITY, ATTR_INVISIBLE,
    FFW_BYPASS, FFW_MDT, ebcdic_char
)


class Field:
    """Represents an input/output field on the screen."""
    
    def __init__(self, row, col, length, ffw=0, fcw=0, attr=ATTR_NORMAL):
        self.row = row          # Starting row (0-based)
        self.col = col          # Starting column (0-based)
        self.length = length    # Field length in characters
        self.ffw = ffw          # Field Format Word
        self.fcw = fcw          # Field Control Word (extended)
        self.attr = attr        # Display attribute
        self.modified = False   # Modified Data Tag
    
    @property
    def is_protected(self):
        """Check if field is protected (bypass/output only)."""
        return bool(self.ffw & FFW_BYPASS)
    
    @property
    def is_input(self):
        """Check if field accepts input."""
        return not self.is_protected
    
    @property
    def end_col(self):
        """Calculate ending column of field."""
        return self.col + self.length - 1
    
    def contains(self, row, col):
        """Check if position is within this field."""
        if row != self.row:
            return False
        return self.col <= col <= self.end_col
    
    def __repr__(self):
        prot = "P" if self.is_protected else "I"
        return f"Field({self.row},{self.col},len={self.length},{prot})"


class Screen:
    """
    Manages the terminal screen buffer.
    
    The screen is a 24x80 (or 27x132) grid of characters with associated
    attributes. Fields define input and output areas.
    """
    
    def __init__(self, rows=24, cols=80):
        self.rows = rows
        self.cols = cols
        
        # Character buffer - stores displayable characters
        self.buffer = bytearray(rows * cols)
        
        # Attribute buffer - stores display attributes per character
        self.attrs = bytearray(rows * cols)
        
        # Cursor position
        self.cursor_row = 0
        self.cursor_col = 0
        
        # List of defined fields
        self.fields = []
        
        # Current field (where cursor is)
        self.current_field = None
        
        # Screen dirty flag (needs redraw)
        self.dirty = True
        
        # Dirty regions for partial update (list of (row, col, len) tuples)
        self.dirty_regions = []
        
        # Initialize with spaces
        self.clear()
    
    def clear(self):
        """Clear the entire screen to spaces."""
        for i in range(len(self.buffer)):
            self.buffer[i] = 0x20  # ASCII space
            self.attrs[i] = ATTR_NORMAL
        
        self.fields.clear()
        self.cursor_row = 0
        self.cursor_col = 0
        self.current_field = None
        self.dirty = True
        self.dirty_regions.clear()
    
    def _pos(self, row, col):
        """Convert row,col to buffer index."""
        return row * self.cols + col
    
    def set_char(self, row, col, char, attr=None):
        """Set a character at the specified position."""
        if 0 <= row < self.rows and 0 <= col < self.cols:
            pos = self._pos(row, col)
            
            # Handle both int and str input
            if isinstance(char, str):
                char = ord(char)
            
            self.buffer[pos] = char
            
            if attr is not None:
                self.attrs[pos] = attr
            
            self._mark_dirty(row, col, 1)
    
    def get_char(self, row, col):
        """Get character at position."""
        if 0 <= row < self.rows and 0 <= col < self.cols:
            return self.buffer[self._pos(row, col)]
        return 0x20
    
    def get_attr(self, row, col):
        """Get attribute at position."""
        if 0 <= row < self.rows and 0 <= col < self.cols:
            return self.attrs[self._pos(row, col)]
        return ATTR_NORMAL
    
    def set_cursor(self, row, col):
        """Move cursor to specified position."""
        old_row, old_col = self.cursor_row, self.cursor_col
        
        self.cursor_row = max(0, min(row, self.rows - 1))
        self.cursor_col = max(0, min(col, self.cols - 1))
        
        # Mark old and new positions dirty
        self._mark_dirty(old_row, old_col, 1)
        self._mark_dirty(self.cursor_row, self.cursor_col, 1)
        
        # Update current field
        self.current_field = self.get_field_at(self.cursor_row, self.cursor_col)
    
    def advance_cursor(self):
        """Move cursor forward one position, wrapping at line end."""
        self.cursor_col += 1
        if self.cursor_col >= self.cols:
            self.cursor_col = 0
            self.cursor_row += 1
            if self.cursor_row >= self.rows:
                self.cursor_row = 0
        
        self._mark_dirty(self.cursor_row, self.cursor_col, 1)
        self.current_field = self.get_field_at(self.cursor_row, self.cursor_col)
    
    def write_string(self, row, col, text, attr=ATTR_NORMAL):
        """Write a string starting at the given position."""
        for i, ch in enumerate(text):
            c = col + i
            if c >= self.cols:
                break
            self.set_char(row, c, ch, attr)
    
    def write_ebcdic(self, row, col, data, attr=ATTR_NORMAL):
        """Write EBCDIC data starting at the given position."""
        for i, b in enumerate(data):
            c = col + i
            if c >= self.cols:
                break
            # Convert EBCDIC to ASCII for display
            ascii_char = ebcdic_char(b)
            self.set_char(row, c, ascii_char, attr)
    
    def add_field(self, field):
        """Add a field definition to the screen."""
        self.fields.append(field)
        self._mark_dirty(field.row, field.col, field.length)
    
    def get_field_at(self, row, col):
        """Find the field containing the given position."""
        for field in self.fields:
            if field.contains(row, col):
                return field
        return None
    
    def get_next_input_field(self, from_row=None, from_col=None):
        """Find the next input (unprotected) field after the given position."""
        if from_row is None:
            from_row = self.cursor_row
        if from_col is None:
            from_col = self.cursor_col
        
        # Sort fields by position
        sorted_fields = sorted(self.fields, 
                               key=lambda f: (f.row, f.col))
        
        # Find input fields after current position
        for field in sorted_fields:
            if field.is_input:
                if (field.row > from_row or 
                    (field.row == from_row and field.col > from_col)):
                    return field
        
        # Wrap around - find first input field
        for field in sorted_fields:
            if field.is_input:
                return field
        
        return None
    
    def get_prev_input_field(self, from_row=None, from_col=None):
        """Find the previous input field before the given position."""
        if from_row is None:
            from_row = self.cursor_row
        if from_col is None:
            from_col = self.cursor_col
        
        # Sort fields by position (reverse)
        sorted_fields = sorted(self.fields, 
                               key=lambda f: (f.row, f.col), reverse=True)
        
        # Find input fields before current position
        for field in sorted_fields:
            if field.is_input:
                if (field.row < from_row or 
                    (field.row == from_row and field.col < from_col)):
                    return field
        
        # Wrap around - find last input field
        for field in sorted_fields:
            if field.is_input:
                return field
        
        return None
    
    def read_field_data(self, field):
        """Read the contents of a field as a string."""
        data = []
        for i in range(field.length):
            col = field.col + i
            if col < self.cols:
                data.append(chr(self.buffer[self._pos(field.row, col)]))
        return ''.join(data)
    
    def get_modified_fields(self):
        """Get all fields that have been modified."""
        return [f for f in self.fields if f.modified and f.is_input]
    
    def get_all_input_fields(self):
        """Get all input (unprotected) fields."""
        return [f for f in self.fields if f.is_input]
    
    def erase_unprotected(self):
        """Clear all unprotected (input) fields."""
        for field in self.fields:
            if field.is_input:
                for i in range(field.length):
                    col = field.col + i
                    if col < self.cols:
                        self.buffer[self._pos(field.row, col)] = 0x20
                field.modified = False
                self._mark_dirty(field.row, field.col, field.length)
    
    def _mark_dirty(self, row, col, length):
        """Mark a region as needing redraw."""
        self.dirty = True
        self.dirty_regions.append((row, col, length))
    
    def clear_dirty(self):
        """Clear dirty flags after redraw."""
        self.dirty = False
        self.dirty_regions.clear()
    
    def get_row_text(self, row):
        """Get a row as a string (for debugging)."""
        start = self._pos(row, 0)
        return ''.join(chr(b) if 32 <= b < 127 else ' ' 
                       for b in self.buffer[start:start + self.cols])
    
    def dump(self):
        """Dump screen contents for debugging."""
        print(f"Screen {self.rows}x{self.cols}, Cursor: ({self.cursor_row},{self.cursor_col})")
        print("+" + "-" * self.cols + "+")
        for row in range(self.rows):
            print("|" + self.get_row_text(row) + "|")
        print("+" + "-" * self.cols + "+")
        print(f"Fields: {len(self.fields)}")
        for f in self.fields:
            print(f"  {f}: '{self.read_field_data(f).strip()}'")
