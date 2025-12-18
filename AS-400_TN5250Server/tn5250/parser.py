# 5250 Response Parser
# Parses data received from terminal clients

from .protocol import (
    IAC, DO, DONT, WILL, WONT, SB, SE, EOR,
    OPT_BINARY, OPT_EOR, OPT_TERMINAL_TYPE, OPT_NEW_ENVIRON,
    SBA, AID_NAMES, ebcdic_to_ascii_str
)


class ResponseParser:
    """
    Parses 5250 responses from terminal clients.
    
    When a user presses Enter or a function key, the terminal sends
    back the cursor position, AID code, and any modified field data.
    """
    
    def __init__(self, fields=None):
        """
        Initialize parser with field definitions.
        
        Args:
            fields: List of field dicts with row, col, length, name
        """
        self.fields = fields or []
        self.debug = False
    
    def set_fields(self, fields):
        """Update field definitions for parsing."""
        self.fields = fields
    
    def log(self, msg):
        if self.debug:
            print(f"[PARSER] {msg}")
    
    def parse(self, data):
        """
        Parse a response from the terminal.
        
        Returns dict with:
            - aid: AID code name (e.g., 'ENTER', 'F3')
            - aid_code: Raw AID byte
            - cursor_row: Cursor row position
            - cursor_col: Cursor column position  
            - fields: Dict of field_name -> value
        """
        if not data:
            return None
        
        # Strip Telnet EOR if present
        if len(data) >= 2 and data[-2] == IAC and data[-1] == EOR:
            data = data[:-2]
        
        if len(data) < 3:
            self.log(f"Response too short: {len(data)} bytes")
            return None
        
        # First 3 bytes: row, col, AID
        cursor_row = data[0]
        cursor_col = data[1]
        aid_code = data[2]
        
        aid_name = AID_NAMES.get(aid_code, f"UNKNOWN(0x{aid_code:02X})")
        
        self.log(f"AID: {aid_name}, Cursor: ({cursor_row}, {cursor_col})")
        
        result = {
            'aid': aid_name,
            'aid_code': aid_code,
            'cursor_row': cursor_row,
            'cursor_col': cursor_col,
            'fields': {}
        }
        
        # Parse field data (if any)
        if len(data) > 3:
            self._parse_field_data(data[3:], result)
        
        return result
    
    def _parse_field_data(self, data, result):
        """Parse field data from response."""
        pos = 0
        current_row = 0
        current_col = 0
        
        while pos < len(data):
            byte = data[pos]
            
            # Set Buffer Address - indicates field position
            if byte == SBA:
                if pos + 2 >= len(data):
                    break
                current_row = data[pos + 1]
                current_col = data[pos + 2]
                pos += 3
                self.log(f"SBA: ({current_row}, {current_col})")
            else:
                # Field data follows until next SBA or end
                field_data = bytearray()
                while pos < len(data) and data[pos] != SBA:
                    field_data.append(data[pos])
                    pos += 1
                
                if field_data:
                    # Convert EBCDIC to ASCII
                    text = ebcdic_to_ascii_str(field_data).rstrip()
                    
                    # Find matching field definition
                    field_name = self._find_field(current_row, current_col)
                    if field_name:
                        result['fields'][field_name] = text
                        self.log(f"Field '{field_name}': '{text}'")
                    else:
                        self.log(f"Unknown field at ({current_row},{current_col}): '{text}'")
        
        return result
    
    def _find_field(self, row, col):
        """Find field name by position."""
        for field in self.fields:
            if field['row'] == row and field['col'] == col:
                return field['name']
        
        # Try fuzzy match (column might be off by 1)
        for field in self.fields:
            if field['row'] == row and abs(field['col'] - col) <= 1:
                return field['name']
        
        return None


class TelnetNegotiator:
    """
    Handles Telnet protocol negotiation for TN5250.
    
    Manages the initial handshake where terminal and server
    agree on protocol options.
    """
    
    def __init__(self):
        self.binary_mode = False
        self.eor_mode = False
        self.terminal_type = None
        self.device_name = None
        self.debug = False
    
    def log(self, msg):
        if self.debug:
            print(f"[TELNET] {msg}")
    
    def get_initial_offers(self):
        """
        Get initial Telnet negotiation bytes to send.
        
        Server offers: WILL BINARY, WILL EOR, DO TERMINAL-TYPE
        """
        offers = bytes([
            IAC, WILL, OPT_BINARY,      # We will use binary mode
            IAC, WILL, OPT_EOR,         # We will use End of Record
            IAC, DO, OPT_TERMINAL_TYPE, # Please send terminal type
            IAC, DO, OPT_EOR,           # Please use EOR
            IAC, DO, OPT_BINARY,        # Please use binary
        ])
        return offers
    
    def process(self, data):
        """
        Process incoming Telnet data.
        
        Returns tuple: (telnet_responses, clean_data)
        - telnet_responses: bytes to send back for negotiation
        - clean_data: non-Telnet data (5250 protocol)
        """
        responses = bytearray()
        clean_data = bytearray()
        
        pos = 0
        while pos < len(data):
            if data[pos] == IAC:
                if pos + 1 >= len(data):
                    break
                
                cmd = data[pos + 1]
                
                # IAC IAC = escaped 0xFF
                if cmd == IAC:
                    clean_data.append(IAC)
                    pos += 2
                    continue
                
                # End of Record
                if cmd == EOR:
                    clean_data.append(IAC)
                    clean_data.append(EOR)
                    pos += 2
                    continue
                
                if pos + 2 >= len(data):
                    break
                
                opt = data[pos + 2]
                
                if cmd == DO:
                    resp = self._handle_do(opt)
                    if resp:
                        responses.extend(resp)
                    pos += 3
                
                elif cmd == DONT:
                    resp = self._handle_dont(opt)
                    if resp:
                        responses.extend(resp)
                    pos += 3
                
                elif cmd == WILL:
                    resp = self._handle_will(opt)
                    if resp:
                        responses.extend(resp)
                    pos += 3
                
                elif cmd == WONT:
                    resp = self._handle_wont(opt)
                    if resp:
                        responses.extend(resp)
                    pos += 3
                
                elif cmd == SB:
                    # Subnegotiation - find SE
                    end = self._find_se(data, pos + 2)
                    if end > 0:
                        resp = self._handle_subneg(data[pos + 2:end])
                        if resp:
                            responses.extend(resp)
                        pos = end + 2
                    else:
                        pos += 3
                else:
                    pos += 2
            else:
                clean_data.append(data[pos])
                pos += 1
        
        return bytes(responses), bytes(clean_data)
    
    def _find_se(self, data, start):
        """Find IAC SE sequence."""
        for i in range(start, len(data) - 1):
            if data[i] == IAC and data[i + 1] == SE:
                return i
        return -1
    
    def _handle_do(self, opt):
        """Handle DO request from client."""
        self.log(f"Received DO {opt}")
        
        if opt == OPT_BINARY:
            self.binary_mode = True
            return bytes([IAC, WILL, opt])
        elif opt == OPT_EOR:
            self.eor_mode = True
            return bytes([IAC, WILL, opt])
        else:
            return bytes([IAC, WONT, opt])
    
    def _handle_dont(self, opt):
        """Handle DONT request."""
        self.log(f"Received DONT {opt}")
        return bytes([IAC, WONT, opt])
    
    def _handle_will(self, opt):
        """Handle WILL offer from client."""
        self.log(f"Received WILL {opt}")
        
        if opt in (OPT_BINARY, OPT_EOR, OPT_TERMINAL_TYPE):
            if opt == OPT_TERMINAL_TYPE:
                # Request the terminal type
                return bytes([IAC, DO, opt, IAC, SB, opt, 1, IAC, SE])
            return bytes([IAC, DO, opt])
        else:
            return bytes([IAC, DONT, opt])
    
    def _handle_wont(self, opt):
        """Handle WONT from client."""
        self.log(f"Received WONT {opt}")
        return bytes([IAC, DONT, opt])
    
    def _handle_subneg(self, data):
        """Handle subnegotiation."""
        if len(data) < 2:
            return None
        
        opt = data[0]
        
        if opt == OPT_TERMINAL_TYPE:
            if data[1] == 0:  # IS
                self.terminal_type = bytes(data[2:]).decode('ascii', errors='ignore')
                self.log(f"Terminal type: {self.terminal_type}")
        
        elif opt == OPT_NEW_ENVIRON:
            # Parse environment variables
            self._parse_environ(data[1:])
        
        return None
    
    def _parse_environ(self, data):
        """Parse NEW-ENVIRON subnegotiation."""
        # Simplified parsing - look for DEVNAME
        try:
            text = bytes(data).decode('ascii', errors='ignore')
            if 'DEVNAME' in text:
                # Extract value after DEVNAME
                idx = text.find('DEVNAME')
                if idx >= 0:
                    self.device_name = text[idx + 8:idx + 18].strip('\x00\x01 ')
                    self.log(f"Device name: {self.device_name}")
        except:
            pass
