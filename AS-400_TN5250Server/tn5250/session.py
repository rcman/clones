# TN5250 Session Handler
# Manages individual client sessions with menu navigation

import time
from .screen_builder import ScreenTemplates
from .parser import ResponseParser, TelnetNegotiator
from .protocol import AID_F3, AID_F12, AID_ENTER, AID_CLEAR


class SessionState:
    """Possible session states."""
    NEGOTIATING = "negotiating"
    SIGNON = "signon"
    MAIN_MENU = "main_menu"
    SUBMENU = "submenu"
    MESSAGE = "message"
    COMMAND = "command"
    SIGNOFF = "signoff"


class Session:
    """
    Manages a single TN5250 client session.
    
    Handles the state machine for:
    - Sign-on screen and authentication
    - Menu navigation
    - Command processing
    - Message display
    """
    
    def __init__(self, client_socket, client_addr, config):
        self.socket = client_socket
        self.addr = client_addr
        self.config = config
        
        self.state = SessionState.NEGOTIATING
        self.user = None
        self.current_menu = None
        self.menu_stack = []
        
        self.negotiator = TelnetNegotiator()
        self.parser = ResponseParser()
        self.current_fields = []
        
        self.last_activity = time.time()
        self.debug = False
        
        # Session data
        self.messages = []
        self.command_history = []
    
    def log(self, msg):
        if self.debug:
            print(f"[SESSION {self.addr}] {msg}")
    
    def start(self):
        """
        Start the session - send initial negotiation and sign-on screen.
        """
        self.log("Session started")
        
        # Send Telnet negotiation
        self.socket.send(self.negotiator.get_initial_offers())
        
        # Small delay for negotiation
        time.sleep(0.2)
        
        # Send sign-on screen
        self._show_signon()
    
    def _send(self, data):
        """Send data to client."""
        try:
            self.socket.send(data)
            return True
        except Exception as e:
            self.log(f"Send error: {e}")
            return False
    
    def _show_signon(self):
        """Display the sign-on screen."""
        self.state = SessionState.SIGNON
        screen_data, fields = ScreenTemplates.signon_screen(
            self.config.SYSTEM_NAME,
            self.config.SUBSYSTEM
        )
        self.current_fields = fields
        self.parser.set_fields(fields)
        self._send(screen_data)
    
    def _show_main_menu(self):
        """Display the main menu."""
        self.state = SessionState.MAIN_MENU
        screen_data, fields = ScreenTemplates.main_menu(
            self.config.SYSTEM_NAME,
            self.user or "GUEST"
        )
        self.current_fields = fields
        self.parser.set_fields(fields)
        self._send(screen_data)
    
    def _show_message(self, title, message, msg_type="info"):
        """Display a message screen."""
        prev_state = self.state
        self.state = SessionState.MESSAGE
        screen_data, fields = ScreenTemplates.message_screen(title, message, msg_type)
        self.current_fields = fields
        self.parser.set_fields(fields)
        self._send(screen_data)
        # Store previous state to return to
        self._prev_state = prev_state
    
    def _show_about(self):
        """Display the about screen."""
        self.state = SessionState.SUBMENU
        screen_data, fields = ScreenTemplates.about_screen(self.config.SYSTEM_NAME)
        self.current_fields = fields
        self.parser.set_fields(fields)
        self._send(screen_data)
    
    def _show_command(self):
        """Display command entry screen."""
        self.state = SessionState.COMMAND
        screen_data, fields = ScreenTemplates.command_entry(self.config.SYSTEM_NAME)
        self.current_fields = fields
        self.parser.set_fields(fields)
        self._send(screen_data)
    
    def _show_error(self, msg, detail=""):
        """Display an error screen."""
        screen_data, fields = ScreenTemplates.error_screen(msg, detail)
        self.current_fields = fields
        self.parser.set_fields(fields)
        self._send(screen_data)
    
    def process_data(self, data):
        """
        Process incoming data from the client.
        
        Returns True to continue session, False to end it.
        """
        self.last_activity = time.time()
        
        # Process Telnet negotiation
        telnet_response, clean_data = self.negotiator.process(data)
        
        if telnet_response:
            self._send(telnet_response)
        
        if not clean_data:
            return True
        
        # Parse 5250 response
        response = self.parser.parse(clean_data)
        if not response:
            return True
        
        aid = response['aid']
        fields = response['fields']
        
        self.log(f"State: {self.state}, AID: {aid}, Fields: {fields}")
        
        # Handle based on current state
        if self.state == SessionState.SIGNON:
            return self._handle_signon(aid, fields)
        
        elif self.state == SessionState.MAIN_MENU:
            return self._handle_main_menu(aid, fields)
        
        elif self.state == SessionState.SUBMENU:
            return self._handle_submenu(aid, fields)
        
        elif self.state == SessionState.MESSAGE:
            return self._handle_message(aid, fields)
        
        elif self.state == SessionState.COMMAND:
            return self._handle_command(aid, fields)
        
        elif self.state == SessionState.SIGNOFF:
            return False
        
        return True
    
    def _handle_signon(self, aid, fields):
        """Handle sign-on screen input."""
        if aid == 'F3' or aid == 'F12':
            # Exit
            self.state = SessionState.SIGNOFF
            self._show_message("Sign Off", "Session ended. Goodbye!")
            return False
        
        if aid == 'ENTER':
            user = fields.get('user', '').strip().upper()
            password = fields.get('password', '').strip()
            
            # Validate credentials
            if not user:
                self._show_error("User ID is required")
                return True
            
            valid_users = getattr(self.config, 'USERS', {'GUEST': ''})
            
            if user not in valid_users:
                self._show_error(f"User {user} not found")
                return True
            
            expected_pw = valid_users[user]
            if expected_pw and password != expected_pw:
                self._show_error("Password incorrect")
                return True
            
            # Sign-on successful
            self.user = user
            self.log(f"User {user} signed on")
            
            # Check for specific program/menu
            program = fields.get('program', '').strip()
            menu = fields.get('menu', '').strip()
            
            if program:
                self._run_program(program)
            elif menu:
                self._show_menu(menu)
            else:
                self._show_main_menu()
        
        return True
    
    def _handle_main_menu(self, aid, fields):
        """Handle main menu input."""
        if aid == 'F3':
            # Sign off
            return self._do_signoff()
        
        if aid == 'F12':
            # Cancel - show sign off confirmation
            self._show_message(
                "Sign Off",
                ["Press Enter to sign off,", "or F12 to return to menu."]
            )
            self._confirm_signoff = True
            return True
        
        if aid == 'ENTER':
            command = fields.get('command', '').strip()
            return self._process_menu_command(command)
        
        return True
    
    def _process_menu_command(self, command):
        """Process a menu selection or command."""
        if not command:
            self._show_main_menu()
            return True
        
        cmd_upper = command.upper()
        
        # Numeric menu selections
        if command == '1':
            self._show_message("Work with Messages", 
                ["No messages.", "", "Press Enter to continue."])
        
        elif command == '2':
            self._show_message("Work with Files",
                ["File system not implemented.", "",
                 "This is a demo system running on",
                 "a Raspberry Pi Pico 2 W!"])
        
        elif command == '3':
            self._show_message("Output Queue",
                ["OUTQ: QPRINT", "Status: RLS", "Files: 0"])
        
        elif command == '4':
            self._show_job_status()
        
        elif command == '5':
            self._show_system_status()
        
        elif command == '6':
            self._show_active_jobs()
        
        elif command == '7':
            self._show_message("Spool Files",
                ["No spool files found."])
        
        elif command == '8':
            self._show_about()
            return True
        
        elif command == '90' or cmd_upper == 'SIGNOFF':
            return self._do_signoff()
        
        # Command-style input
        elif cmd_upper.startswith('GO '):
            menu = cmd_upper[3:].strip()
            self._show_message(f"Menu {menu}", 
                [f"Menu {menu} not found.", "",
                 "Available: MAIN"])
        
        elif cmd_upper == 'WRKSYSSTS' or cmd_upper == 'DSPSYSSTS':
            self._show_system_status()
        
        elif cmd_upper == 'WRKACTJOB':
            self._show_active_jobs()
        
        elif cmd_upper == 'DSPJOB':
            self._show_job_status()
        
        elif cmd_upper.startswith('?') or cmd_upper == 'HELP':
            self._show_help()
        
        elif cmd_upper == 'CALL QCMD':
            self._show_command()
            return True
        
        else:
            self._show_error(
                f"Command '{command}' not found",
                "Type a menu option number or valid command."
            )
        
        return True
    
    def _handle_submenu(self, aid, fields):
        """Handle submenu/detail screen input."""
        if aid in ('F3', 'F12', 'ENTER'):
            self._show_main_menu()
        return True
    
    def _handle_message(self, aid, fields):
        """Handle message screen input."""
        # Check for sign-off confirmation
        if hasattr(self, '_confirm_signoff') and self._confirm_signoff:
            if aid == 'ENTER':
                self._confirm_signoff = False
                return self._do_signoff()
            else:
                self._confirm_signoff = False
                self._show_main_menu()
                return True
        
        # Any key returns to previous state
        if hasattr(self, '_prev_state'):
            if self._prev_state == SessionState.MAIN_MENU:
                self._show_main_menu()
            else:
                self._show_main_menu()
        else:
            self._show_main_menu()
        
        return True
    
    def _handle_command(self, aid, fields):
        """Handle command entry screen."""
        if aid == 'F3' or aid == 'F12':
            self._show_main_menu()
            return True
        
        if aid == 'ENTER':
            command = fields.get('command', '').strip()
            if command:
                self.command_history.append(command)
                return self._process_menu_command(command)
            self._show_command()
        
        return True
    
    def _do_signoff(self):
        """Sign off the user."""
        self.log(f"User {self.user} signing off")
        self._show_message("Sign Off", 
            [f"User {self.user} signed off.",
             "",
             f"System: {self.config.SYSTEM_NAME}",
             "",
             "Session ended."])
        self.state = SessionState.SIGNOFF
        return True  # Let message display, then close
    
    def _show_system_status(self):
        """Display system status."""
        import gc
        gc.collect()
        free_mem = gc.mem_free() if hasattr(gc, 'mem_free') else 0
        
        self._show_message("System Status", [
            f"System: {self.config.SYSTEM_NAME}",
            "",
            f"CPU Util . . . . . :     5.2 %",
            f"Active Jobs  . . . :        1",
            f"Memory Free  . . . :   {free_mem:,} bytes",
            "",
            "System Status: ACTIVE",
            "IPL Date/Time: 2025-01-01 00:00:00",
        ])
    
    def _show_active_jobs(self):
        """Display active jobs."""
        self._show_message("Active Jobs", [
            "Opt  Job        User        Type    Status",
            "     QINTER     QSYS        SBS     ACTIVE",
            f"     PICO5250   {self.user or 'QUSER':10s}  INT     ACTIVE",
            "",
            "                                 Bottom",
            "",
            "Press Enter to continue.",
        ])
    
    def _show_job_status(self):
        """Display job status."""
        self._show_message("Display Job Status", [
            f"Job: PICO5250    User: {self.user or 'QUSER'}",
            "",
            "Status . . . . . : ACTIVE",
            "Type . . . . . . : INTERACTIVE", 
            "Subsystem  . . . : QINTER",
            "Pool . . . . . . : 1",
            "CPU Time . . . . : 00:00:00.001",
        ])
    
    def _show_help(self):
        """Display help information."""
        self._show_message("Help", [
            "Available Commands:",
            "",
            "  1-8, 90  Menu options",
            "  SIGNOFF  End session",
            "  WRKSYSSTS  System status",
            "  WRKACTJOB  Active jobs",
            "  DSPJOB     Job status",
            "  GO menu    Go to menu",
            "  CALL QCMD  Command entry",
        ])
    
    def _run_program(self, program):
        """Run a program (simulated)."""
        self._show_message(f"Program {program.upper()}",
            [f"Program {program.upper()} not found in library list.",
             "",
             "Library list:",
             "  QSYS",
             "  QGPL",
             "  QTEMP"])
    
    def _show_menu(self, menu):
        """Show a specific menu."""
        menu = menu.upper()
        if menu == 'MAIN':
            self._show_main_menu()
        else:
            self._show_message(f"Menu {menu}",
                [f"Menu {menu} not found.",
                 "",
                 "Press Enter to continue to MAIN menu."])
    
    def is_timed_out(self, timeout):
        """Check if session has timed out."""
        if timeout <= 0:
            return False
        return (time.time() - self.last_activity) > timeout
    
    def close(self):
        """Close the session."""
        self.log("Session closing")
        try:
            self.socket.close()
        except:
            pass
