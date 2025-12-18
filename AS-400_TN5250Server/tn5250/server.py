# TN5250 Server
# Accepts incoming terminal connections and manages sessions

import socket
import select
import time

from .session import Session


class TN5250Server:
    """
    TN5250 Telnet Server for Fake AS/400.
    
    Listens for incoming connections and spawns sessions
    to handle each connected terminal.
    """
    
    def __init__(self, config):
        self.config = config
        self.port = getattr(config, 'SERVER_PORT', 23)
        self.max_connections = getattr(config, 'MAX_CONNECTIONS', 1)
        self.idle_timeout = getattr(config, 'IDLE_TIMEOUT', 300)
        
        self.server_socket = None
        self.sessions = []
        self.running = False
        self.debug = False
    
    def log(self, msg):
        print(f"[SERVER] {msg}")
    
    def start(self):
        """
        Start the TN5250 server.
        
        Creates the listening socket and begins accepting connections.
        """
        self.log(f"Starting TN5250 server on port {self.port}")
        
        # Create server socket
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            self.server_socket.bind(('0.0.0.0', self.port))
            self.server_socket.listen(5)
            self.server_socket.setblocking(False)
            
            self.running = True
            self.log(f"Server listening on port {self.port}")
            self.log(f"Max connections: {self.max_connections}")
            self.log(f"System name: {self.config.SYSTEM_NAME}")
            
            return True
            
        except Exception as e:
            self.log(f"Failed to start server: {e}")
            return False
    
    def stop(self):
        """Stop the server and close all sessions."""
        self.log("Stopping server...")
        self.running = False
        
        # Close all sessions
        for session in self.sessions:
            session.close()
        self.sessions.clear()
        
        # Close server socket
        if self.server_socket:
            try:
                self.server_socket.close()
            except:
                pass
            self.server_socket = None
        
        self.log("Server stopped")
    
    def run_once(self):
        """
        Run one iteration of the server loop.
        
        Call this repeatedly from your main loop.
        Returns True while server is running.
        """
        if not self.running:
            return False
        
        # Check for new connections
        self._accept_connections()
        
        # Process existing sessions
        self._process_sessions()
        
        # Clean up timed-out sessions
        self._cleanup_sessions()
        
        return True
    
    def run_forever(self):
        """
        Run the server until stopped.
        
        Blocks and handles all connections.
        """
        while self.running:
            try:
                self.run_once()
                time.sleep(0.01)  # Small delay to prevent CPU hogging
            except KeyboardInterrupt:
                self.log("Interrupted")
                break
            except Exception as e:
                self.log(f"Error: {e}")
        
        self.stop()
    
    def _accept_connections(self):
        """Check for and accept new connections."""
        try:
            # Non-blocking accept
            readable, _, _ = select.select([self.server_socket], [], [], 0)
            
            if self.server_socket in readable:
                client_socket, client_addr = self.server_socket.accept()
                
                self.log(f"Connection from {client_addr}")
                
                # Check connection limit
                if len(self.sessions) >= self.max_connections:
                    self.log(f"Connection limit reached, rejecting {client_addr}")
                    client_socket.close()
                    return
                
                # Create new session
                client_socket.setblocking(False)
                session = Session(client_socket, client_addr, self.config)
                session.debug = self.debug
                self.sessions.append(session)
                
                # Start the session (sends sign-on screen)
                session.start()
                
        except Exception as e:
            if "EAGAIN" not in str(e) and "EWOULDBLOCK" not in str(e):
                self.log(f"Accept error: {e}")
    
    def _process_sessions(self):
        """Process data from all active sessions."""
        for session in list(self.sessions):
            try:
                # Check for incoming data
                readable, _, _ = select.select([session.socket], [], [], 0)
                
                if session.socket in readable:
                    data = session.socket.recv(4096)
                    
                    if data:
                        # Process the data
                        if not session.process_data(data):
                            # Session ended
                            self._end_session(session)
                    else:
                        # Connection closed
                        self.log(f"Connection closed by {session.addr}")
                        self._end_session(session)
                        
            except Exception as e:
                err_str = str(e)
                if "EAGAIN" not in err_str and "EWOULDBLOCK" not in err_str:
                    self.log(f"Session error for {session.addr}: {e}")
                    self._end_session(session)
    
    def _cleanup_sessions(self):
        """Remove timed-out sessions."""
        if self.idle_timeout <= 0:
            return
        
        for session in list(self.sessions):
            if session.is_timed_out(self.idle_timeout):
                self.log(f"Session {session.addr} timed out")
                self._end_session(session)
    
    def _end_session(self, session):
        """End and remove a session."""
        session.close()
        if session in self.sessions:
            self.sessions.remove(session)
        self.log(f"Session ended for {session.addr}")
    
    def get_status(self):
        """Get server status information."""
        return {
            'running': self.running,
            'port': self.port,
            'active_sessions': len(self.sessions),
            'max_sessions': self.max_connections,
            'sessions': [
                {
                    'addr': s.addr,
                    'user': s.user,
                    'state': s.state
                }
                for s in self.sessions
            ]
        }
