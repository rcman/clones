# WiFi Network Connection Helper for Pico W

import network
import time


class WiFiConnection:
    """WiFi connection manager for Raspberry Pi Pico W."""
    
    def __init__(self, ssid, password):
        self.ssid = ssid
        self.password = password
        self.wlan = network.WLAN(network.STA_IF)
    
    def connect(self, timeout=30):
        """Connect to WiFi network."""
        print(f"Connecting to WiFi: {self.ssid}")
        
        self.wlan.active(True)
        self.wlan.connect(self.ssid, self.password)
        
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
        
        print(f"\nConnected! IP: {self.ip_address}")
        return True
    
    def disconnect(self):
        """Disconnect from WiFi."""
        self.wlan.disconnect()
        self.wlan.active(False)
    
    @property
    def is_connected(self):
        return self.wlan.isconnected()
    
    @property
    def ip_address(self):
        if self.is_connected:
            return self.wlan.ifconfig()[0]
        return None
