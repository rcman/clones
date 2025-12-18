# WiFi Network Connection Helper for Pico W
# Handles WiFi connection and status

import network
import time


class WiFiConnection:
    """
    WiFi connection manager for Raspberry Pi Pico W.
    """
    
    def __init__(self, ssid, password):
        """
        Initialize WiFi connection manager.
        
        Args:
            ssid: WiFi network name
            password: WiFi password
        """
        self.ssid = ssid
        self.password = password
        self.wlan = network.WLAN(network.STA_IF)
        self._connected = False
    
    def connect(self, timeout=30):
        """
        Connect to WiFi network.
        
        Args:
            timeout: Maximum time to wait for connection (seconds)
        
        Returns:
            True if connected, False otherwise
        """
        print(f"Connecting to WiFi: {self.ssid}")
        
        # Activate interface
        self.wlan.active(True)
        
        # Connect
        self.wlan.connect(self.ssid, self.password)
        
        # Wait for connection
        start = time.time()
        while not self.wlan.isconnected():
            if time.time() - start > timeout:
                print("WiFi connection timeout!")
                return False
            
            status = self.wlan.status()
            if status == network.STAT_WRONG_PASSWORD:
                print("Wrong WiFi password!")
                return False
            elif status == network.STAT_NO_AP_FOUND:
                print("WiFi network not found!")
                return False
            elif status == network.STAT_CONNECT_FAIL:
                print("WiFi connection failed!")
                return False
            
            print(".", end="")
            time.sleep(0.5)
        
        self._connected = True
        print(f"\nConnected! IP: {self.ip_address}")
        return True
    
    def disconnect(self):
        """Disconnect from WiFi."""
        self.wlan.disconnect()
        self.wlan.active(False)
        self._connected = False
    
    @property
    def is_connected(self):
        """Check if currently connected."""
        return self.wlan.isconnected()
    
    @property
    def ip_address(self):
        """Get current IP address."""
        if self.is_connected:
            return self.wlan.ifconfig()[0]
        return None
    
    @property
    def signal_strength(self):
        """Get WiFi signal strength (RSSI)."""
        if self.is_connected:
            return self.wlan.status('rssi')
        return None
    
    def status_string(self):
        """Get human-readable status string."""
        if self.is_connected:
            return f"Connected: {self.ip_address} (RSSI: {self.signal_strength})"
        return "Not connected"
