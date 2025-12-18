"""
Fake AS/400 TN5250 Server for Raspberry Pi Pico 2 W

This creates a TN5250 server that emulates the look and feel
of an IBM AS/400 (iSeries) system. Connect with any TN5250
terminal emulator to interact with the fake system.

Hardware:
- Raspberry Pi Pico 2 W (for WiFi)
- Optional: ILI9341 display for local status

Usage:
1. Edit config.py with your WiFi credentials
2. Upload all files to Pico
3. Connect power - server starts automatically
4. Connect using a TN5250 client (tn5250j, IBM i Access, etc.)
"""

import time
import sys

# Import configuration
import config

# Import our modules
from network import WiFiConnection
from tn5250 import TN5250Server


def print_banner():
    """Print startup banner."""
    print()
    print("=" * 60)
    print("  FAKE AS/400 TN5250 SERVER")
    print("  Raspberry Pi Pico 2 W Edition")
    print("=" * 60)
    print()


def print_connection_info(ip):
    """Print connection instructions."""
    print()
    print("-" * 60)
    print("  SERVER READY")
    print("-" * 60)
    print()
    print(f"  System Name : {config.SYSTEM_NAME}")
    print(f"  IP Address  : {ip}")
    print(f"  Port        : {config.SERVER_PORT}")
    print()
    print("  Connect using a TN5250 terminal emulator:")
    print()
    print(f"    tn5250 {ip}")
    print(f"    - or -")
    print(f"    telnet {ip} {config.SERVER_PORT}")
    print()
    print("  Demo users:")
    for user, pw in config.USERS.items():
        if pw:
            print(f"    {user} / {pw}")
        else:
            print(f"    {user} (no password)")
    print()
    print("-" * 60)
    print()


def main():
    """Main entry point."""
    print_banner()
    
    # Connect to WiFi
    print("Step 1: Connecting to WiFi...")
    wifi = WiFiConnection(config.WIFI_SSID, config.WIFI_PASSWORD)
    
    if not wifi.connect(timeout=30):
        print("\nFailed to connect to WiFi. Check config.py settings.")
        print("Restarting in 10 seconds...")
        time.sleep(10)
        import machine
        machine.reset()
    
    ip = wifi.ip_address
    print(f"WiFi connected: {ip}")
    
    # Start TN5250 server
    print("\nStep 2: Starting TN5250 server...")
    server = TN5250Server(config)
    server.debug = False  # Set True for verbose logging
    
    if not server.start():
        print("\nFailed to start server. Port may be in use.")
        print("Restarting in 10 seconds...")
        time.sleep(10)
        import machine
        machine.reset()
    
    # Print connection instructions
    print_connection_info(ip)
    
    # Main loop
    print("Waiting for connections... (Ctrl+C to stop)")
    print()
    
    last_status = 0
    
    try:
        while True:
            # Process server
            server.run_once()
            
            # Periodic status update
            now = time.time()
            if now - last_status > 60:
                status = server.get_status()
                if status['active_sessions'] > 0:
                    print(f"[STATUS] Active sessions: {status['active_sessions']}")
                    for s in status['sessions']:
                        print(f"         - {s['addr']}: {s['user'] or '(signing on)'} [{s['state']}]")
                last_status = now
            
            # Small delay
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        print("\nShutting down...")
    
    finally:
        server.stop()
        wifi.disconnect()
        print("Goodbye!")


# Auto-start when imported as main module
if __name__ == "__main__":
    main()
