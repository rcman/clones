# XENIX Server for Linux

A C++ implementation of a XENIX-style telnet server that runs as a Linux systemd service.

## Features

- Full XENIX terminal emulation
- Multi-user TCP/telnet access
- Persistent filesystem (saved to `/var/lib/xenix/xenix_state.json`)
- Runs as a systemd service
- Support for multiple simultaneous connections
- Command history and line editing
- Authentic XENIX commands and behavior

## Requirements

- Linux system with systemd
- g++ compiler with C++17 support
- Make
- Root/sudo access for installation

## Installation

### 1. Compile the server

```bash
make
```

### 2. Install as a system service

```bash
make install
```

This will:
- Create a dedicated `xenix` user
- Install the binary to `/usr/local/bin/`
- Install the systemd service file
- Create the state directory at `/var/lib/xenix/`

### 3. Start the service

```bash
sudo systemctl start xenix
```

### 4. Enable on boot (optional)

```bash
sudo systemctl enable xenix
```

## Usage

### Connecting to the server

From the local machine:
```bash
telnet localhost 2323
```

Or using netcat:
```bash
nc localhost 2323
```

From a remote machine:
```bash
telnet <server-ip> 2323
```

### Managing the service

**Check status:**
```bash
sudo systemctl status xenix
```

**View logs:**
```bash
sudo journalctl -u xenix -f
```

**Stop the service:**
```bash
sudo systemctl stop xenix
```

**Restart the service:**
```bash
sudo systemctl restart xenix
```

### Quick commands via Makefile

```bash
make start    # Start the service
make stop     # Stop the service
make restart  # Restart the service
make status   # Check status
make logs     # View logs
```

## Available XENIX Commands

### File Management
- `ls [-l]` - List files
- `cd <path>` - Change directory
- `pwd` - Print working directory
- `mkdir <dir>` - Create directory
- `rmdir <dir>` - Remove empty directory
- `touch <file>` - Create empty file
- `cat <file>` - Display file contents
- `rm [-rf] <file>` - Remove files
- `cp <src> <dest>` - Copy file
- `mv <src> <dest>` - Move/rename file
- `chmod <mode> <file>` - Change permissions

### Text Processing
- `echo <text>` - Print text
- `find <path> -name <pattern>` - Find files

### System Information
- `ps` - List processes
- `who` - Show logged in users
- `whoami` - Show current user
- `date` - Show date and time
- `uname [-a]` - Show system information

### Other
- `clear` - Clear screen
- `help` - Show help
- `exit` or `logout` - Exit session

## Configuration

### Change the port

Edit `xenix_server.cpp` and change the `TCP_PORT` constant:
```cpp
const int TCP_PORT = 2323;  // Change this value
```

Then recompile and reinstall:
```bash
make clean
make
make install
sudo systemctl restart xenix
```

### Change the state file location

Edit `xenix_server.cpp` and change the `SAVE_FILE` constant:
```cpp
const std::string SAVE_FILE = "/var/lib/xenix/xenix_state.json";
```

Make sure the xenix user has write access to the directory.

## Security Notes

1. **Default Port**: The server runs on port 2323 by default (non-privileged port). To use port 23 (standard telnet), either:
   - Run as root (not recommended)
   - Use port forwarding: `sudo iptables -t nat -A PREROUTING -p tcp --dport 23 -j REDIRECT --to-port 2323`
   - Grant CAP_NET_BIND_SERVICE capability: `sudo setcap 'cap_net_bind_service=+ep' /usr/local/bin/xenix_server`

2. **Firewall**: You may need to open the port in your firewall:
   ```bash
   sudo ufw allow 2323/tcp
   ```

3. **No Authentication**: This is a simulation - there's no real authentication. Don't expose it to untrusted networks!

4. **No Encryption**: Telnet is unencrypted. For production use, consider wrapping with stunnel or similar.

## Troubleshooting

### Service won't start

Check the logs:
```bash
sudo journalctl -u xenix -n 50
```

Common issues:
- Port already in use: Check with `sudo netstat -tlnp | grep 2323`
- Permission issues: Verify `/var/lib/xenix` exists and is owned by `xenix:xenix`

### Can't connect

1. Verify service is running: `sudo systemctl status xenix`
2. Check if port is listening: `sudo netstat -tlnp | grep 2323`
3. Test local connection: `telnet localhost 2323`
4. Check firewall rules

### Lost connection immediately

This usually means the server crashed. Check logs:
```bash
sudo journalctl -u xenix -n 100
```

## Uninstallation

```bash
make uninstall
```

This will:
- Stop and disable the service
- Remove the binary and service files
- Leave the xenix user and data directory intact (remove manually if needed)

To completely remove everything:
```bash
make uninstall
sudo userdel xenix
sudo rm -rf /var/lib/xenix
```

## Development

### Building without installing

```bash
make
./xenix_server
```

The server will run in the foreground. Press Ctrl+C to stop.

### Testing

Connect with telnet in another terminal:
```bash
telnet localhost 2323
```

### Debugging

Run with gdb:
```bash
make
gdb ./xenix_server
```

## License

This is a recreation/simulation of historical XENIX for educational purposes.

## Credits

Based on the MicroPython XENIX implementation, ported to C++ for Linux systems.
