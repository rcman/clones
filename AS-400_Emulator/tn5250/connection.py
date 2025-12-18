# TN5250 Network Connection Handler
# Manages Telnet connection and TN5250 protocol negotiation

import socket
import time

from .commands import (
    IAC, DO, DONT, WILL, WONT, SB, SE,
    OPT_BINARY, OPT_ECHO, OPT_TERMINAL_TYPE, OPT_EOR,
    OPT_NEW_ENVIRON, OPT_NAWS, EOR,
    AID_ENTER, AID_CLEAR, AID_KEYS,
    ascii_to_ebcdic
)
from .screen import Screen


class TN5250Connection:
    """
    Handles the TN5250 Telnet connection.
    
    TN5250 is the Telnet-based protocol for 5250 terminal emulation.
    It uses standard Telnet negotiation plus 5250-specific options.
    """
    
    def __init__(self, host, port=23, device_name="MICROPICO"):
        self.host = host
        self.port = port
        self.device_name = device_name
        
        self.socket = None
        self.connected = False
        self.negotiated = False
        
        # Receive buffer
        self.recv_buffer = bytearray()
        
        # Debug mode
        self.debug = False
        
        # Terminal type string for negotiation
        self.terminal_type = b"IBM-3179-2"  # Common 5250 terminal
    
    def log(self, msg):
        """Debug logging."""
        if self.debug:
            print(f"[CONN] {msg}")
    
    def connect(self):
        """
        Establish connection to the AS/400.
        
        Returns True on success, False on failure.
        """
        try:
            self.log(f"Connecting to {self.host}:{self.port}...")
            
            # Create socket
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(30)
            
            # Connect
            self.socket.connect((self.host, self.port))
            
            # Set to non-blocking for receive polling
            self.socket.setblocking(False)
            
            self.connected = True
            self.log("Connected!")
            
            # Perform Telnet negotiation
            self._negotiate()
            
            return True
            
        except Exception as e:
            self.log(f"Connection failed: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Close the connection."""
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
        self.socket = None
        self.connected = False
        self.negotiated = False
        self.log("Disconnected")
    
    def _negotiate(self):
        """
        Perform Telnet option negotiation.
        
        The AS/400 will send various DO/WILL requests.
        We respond appropriately to establish 5250 mode.
        """
        self.log("Starting Telnet negotiation...")
        
        # Give server time to send initial negotiation
        time.sleep(0.5)
        
        # Process any incoming negotiation requests
        max_rounds = 10
        for _ in range(max_rounds):
            data = self._recv_raw()
            if not data:
                time.sleep(0.1)
                continue
            
            self.log(f"Received {len(data)} bytes during negotiation")
            self._process_telnet_commands(data)
        
        self.negotiated = True
        self.log("Negotiation complete")
    
    def _process_telnet_commands(self, data):
        """Process Telnet IAC commands in received data."""
        pos = 0
        while pos < len(data):
            if data[pos] == IAC:
                if pos + 1 >= len(data):
                    break
                
                cmd = data[pos + 1]
                
                # Handle IAC IAC (escaped 0xFF)
                if cmd == IAC:
                    self.recv_buffer.append(IAC)
                    pos += 2
                    continue
                
                if pos + 2 >= len(data):
                    break
                
                opt = data[pos + 2]
                
                if cmd == DO:
                    self._handle_do(opt)
                    pos += 3
                elif cmd == DONT:
                    self._handle_dont(opt)
                    pos += 3
                elif cmd == WILL:
                    self._handle_will(opt)
                    pos += 3
                elif cmd == WONT:
                    self._handle_wont(opt)
                    pos += 3
                elif cmd == SB:
                    # Subnegotiation - find SE
                    end = self._find_se(data, pos + 2)
                    if end > 0:
                        self._handle_subneg(data[pos + 2:end])
                        pos = end + 2  # Skip IAC SE
                    else:
                        pos += 3
                else:
                    pos += 2
            else:
                # Regular data - add to buffer
                self.recv_buffer.append(data[pos])
                pos += 1
    
    def _find_se(self, data, start):
        """Find IAC SE sequence marking end of subnegotiation."""
        for i in range(start, len(data) - 1):
            if data[i] == IAC and data[i + 1] == SE:
                return i
        return -1
    
    def _handle_do(self, opt):
        """Respond to DO request."""
        self.log(f"DO option {opt}")
        
        if opt == OPT_TERMINAL_TYPE:
            # We will send terminal type
            self._send_telnet(WILL, opt)
        elif opt == OPT_EOR:
            # We support End of Record
            self._send_telnet(WILL, opt)
        elif opt == OPT_BINARY:
            # Binary mode
            self._send_telnet(WILL, opt)
        elif opt == OPT_NEW_ENVIRON:
            # Environment variables
            self._send_telnet(WILL, opt)
        else:
            # Refuse unknown options
            self._send_telnet(WONT, opt)
    
    def _handle_dont(self, opt):
        """Respond to DONT request."""
        self.log(f"DONT option {opt}")
        self._send_telnet(WONT, opt)
    
    def _handle_will(self, opt):
        """Respond to WILL request."""
        self.log(f"WILL option {opt}")
        
        if opt == OPT_ECHO:
            # Server will echo - we accept
            self._send_telnet(DO, opt)
        elif opt == OPT_EOR:
            self._send_telnet(DO, opt)
        elif opt == OPT_BINARY:
            self._send_telnet(DO, opt)
        else:
            self._send_telnet(DONT, opt)
    
    def _handle_wont(self, opt):
        """Respond to WONT request."""
        self.log(f"WONT option {opt}")
        self._send_telnet(DONT, opt)
    
    def _handle_subneg(self, data):
        """Handle subnegotiation data."""
        if len(data) < 1:
            return
        
        opt = data[0]
        self.log(f"Subnegotiation for option {opt}")
        
        if opt == OPT_TERMINAL_TYPE:
            # Server wants terminal type - send it
            if len(data) > 1 and data[1] == 1:  # SEND
                self._send_terminal_type()
        
        elif opt == OPT_NEW_ENVIRON:
            # Send environment variables (device name)
            self._send_environment()
    
    def _send_telnet(self, cmd, opt):
        """Send a Telnet command."""
        self.log(f"Sending: IAC {cmd} {opt}")
        self._send_raw(bytes([IAC, cmd, opt]))
    
    def _send_terminal_type(self):
        """Send terminal type subnegotiation."""
        self.log(f"Sending terminal type: {self.terminal_type}")
        msg = bytes([IAC, SB, OPT_TERMINAL_TYPE, 0]) + self.terminal_type + bytes([IAC, SE])
        self._send_raw(msg)
    
    def _send_environment(self):
        """Send environment variables (device name)."""
        # NEW-ENVIRON: VAR DEVNAME VALUE <name>
        self.log(f"Sending device name: {self.device_name}")
        var_devname = b"DEVNAME"
        msg = (bytes([IAC, SB, OPT_NEW_ENVIRON, 0]) +  # IS
               bytes([0]) + var_devname +              # VAR
               bytes([1]) + self.device_name.encode() +  # VALUE
               bytes([IAC, SE]))
        self._send_raw(msg)
    
    def _send_raw(self, data):
        """Send raw bytes to socket."""
        if self.socket and self.connected:
            try:
                self.socket.send(data)
            except Exception as e:
                self.log(f"Send error: {e}")
                self.connected = False
    
    def _recv_raw(self):
        """Receive raw bytes from socket (non-blocking)."""
        if not self.socket or not self.connected:
            return None
        
        try:
            data = self.socket.recv(4096)
            if data:
                return data
            else:
                # Empty data means connection closed
                self.connected = False
                return None
        except OSError as e:
            # EAGAIN/EWOULDBLOCK means no data available
            if e.args[0] in (11, 35, 10035):  # EAGAIN on different platforms
                return None
            self.log(f"Receive error: {e}")
            self.connected = False
            return None
    
    def receive(self):
        """
        Receive and process data from the host.
        
        Returns processed 5250 data stream (with Telnet stripped),
        or None if no data available.
        """
        # Get raw data
        raw = self._recv_raw()
        if raw:
            self._process_telnet_commands(raw)
        
        # Return buffered data
        if self.recv_buffer:
            data = bytes(self.recv_buffer)
            self.recv_buffer.clear()
            return data
        
        return None
    
    def send_aid(self, aid_code, screen, send_fields=True):
        """
        Send an AID (Attention Identifier) to the host.
        
        This is how we send function key presses and field data
        back to the AS/400.
        
        Args:
            aid_code: The AID byte (e.g., AID_ENTER, AID_F3)
            screen: Screen object to read field data from
            send_fields: Whether to include modified field data
        """
        self.log(f"Sending AID: 0x{aid_code:02X}")
        
        # Build response data
        response = bytearray()
        
        # Row and column of cursor (1-based)
        response.append(screen.cursor_row + 1)
        response.append(screen.cursor_col + 1)
        
        # AID byte
        response.append(aid_code)
        
        # Include modified field data for most AIDs
        if send_fields and aid_code not in (AID_CLEAR,):
            fields = screen.get_modified_fields()
            
            for field in fields:
                # Set Buffer Address for field
                response.append(0x11)  # SBA
                response.append(field.row + 1)
                response.append(field.col + 1)
                
                # Field data (convert to EBCDIC)
                field_data = screen.read_field_data(field)
                response.extend(ascii_to_ebcdic(field_data.encode()))
        
        # Wrap in EOR
        self._send_raw(bytes(response) + bytes([IAC, EOR]))
    
    def send_key(self, key_name, screen):
        """
        Send a key press to the host.
        
        Args:
            key_name: Key name (e.g., "ENTER", "F3", "F12")
            screen: Screen object
        """
        aid_code = AID_KEYS.get(key_name.upper())
        if aid_code:
            self.send_aid(aid_code, screen)
        else:
            self.log(f"Unknown key: {key_name}")
