import sys
import time
import json
import os
from machine import UART, Pin
import select

class FileNode:
    def __init__(self, name, is_directory=False):
        self.name = name
        self.is_directory = is_directory
        self.content = ""
        self.permissions = "drwxr-xr-x" if is_directory else "-rw-r--r--"
        self.owner = "root"
        self.group = "root"
        self.size = 0
        self.modified = self._get_timestamp()
        self.children = {} if is_directory else None
    
    def _get_timestamp(self):
        try:
            t = time.localtime()
            months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
            return "{} {:2d} {:02d}:{:02d}".format(
                months[t[1]-1], t[2], t[3], t[4])
        except:
            return "Jan 01 00:00"
    
    def to_dict(self):
        d = {
            'name': self.name,
            'is_directory': self.is_directory,
            'content': self.content,
            'permissions': self.permissions,
            'owner': self.owner,
            'group': self.group,
            'size': self.size,
            'modified': self.modified
        }
        if self.is_directory:
            d['children'] = {k: v.to_dict() for k, v in self.children.items()}
        return d
    
    @staticmethod
    def from_dict(d):
        node = FileNode(d['name'], d['is_directory'])
        node.content = d.get('content', '')
        node.permissions = d.get('permissions', 'drwxr-xr-x' if d['is_directory'] else '-rw-r--r--')
        node.owner = d.get('owner', 'root')
        node.group = d.get('group', 'root')
        node.size = d.get('size', 0)
        node.modified = d.get('modified', 'Jan 01 00:00')
        if d['is_directory']:
            node.children = {k: FileNode.from_dict(v) for k, v in d.get('children', {}).items()}
        return node


class FileSystem:
    def __init__(self):
        self.root = FileNode("/", True)
        self.current = self.root
        self.path_stack = ["/"]
        self._create_initial_structure()
    
    def _create_initial_structure(self):
        self.root.children['bin'] = FileNode('bin', True)
        self.root.children['etc'] = FileNode('etc', True)
        self.root.children['usr'] = FileNode('usr', True)
        self.root.children['tmp'] = FileNode('tmp', True)
        self.root.children['home'] = FileNode('home', True)
        self.root.children['mnt'] = FileNode('mnt', True)
        
        home = self.root.children['home']
        home.children['root'] = FileNode('root', True)
        
        # Create default MOTD
        etc = self.root.children['etc']
        motd = FileNode('motd', False)
        motd.content = "Welcome to Xenix System V on Raspberry Pi Pico!\n\nMicroPython " + sys.version + "\n"
        motd.size = len(motd.content)
        etc.children['motd'] = motd
    
    def get_current_path(self):
        return '/'.join(self.path_stack).replace('//', '/')
    
    def list_files(self):
        return list(self.current.children.values())
    
    def change_directory(self, path):
        if path == '..':
            if len(self.path_stack) > 1:
                self.path_stack.pop()
                self.current = self._navigate_to_path(self.path_stack)
        elif path == '/':
            self.path_stack = ['/']
            self.current = self.root
        elif path.startswith('/'):
            parts = [p for p in path.split('/') if p]
            new_path = ['/']
            node = self.root
            
            for part in parts:
                if part in node.children and node.children[part].is_directory:
                    node = node.children[part]
                    new_path.append(part)
                else:
                    return "cd: {}: No such directory".format(path)
            
            self.path_stack = new_path
            self.current = node
        else:
            if path in self.current.children and self.current.children[path].is_directory:
                self.path_stack.append(path)
                self.current = self.current.children[path]
            else:
                return "cd: {}: No such directory".format(path)
        return None
    
    def _navigate_to_path(self, path):
        node = self.root
        for i in range(1, len(path)):
            if path[i] in node.children:
                node = node.children[path[i]]
            else:
                return self.root
        return node
    
    def create_directory(self, name):
        if name not in self.current.children:
            self.current.children[name] = FileNode(name, True)
        else:
            return "mkdir: cannot create directory '{}': File exists".format(name)
        return None
    
    def remove_directory(self, name):
        if name in self.current.children:
            node = self.current.children[name]
            if node.is_directory and len(node.children) == 0:
                del self.current.children[name]
            else:
                return "rmdir: failed to remove '{}'".format(name)
        else:
            return "rmdir: failed to remove '{}': No such directory".format(name)
        return None
    
    def create_file(self, name):
        if name not in self.current.children:
            self.current.children[name] = FileNode(name, False)
        return None
    
    def write_file(self, name, content):
        if name in self.current.children:
            file_node = self.current.children[name]
        else:
            file_node = FileNode(name, False)
            self.current.children[name] = file_node
        
        if not file_node.is_directory:
            file_node.content = content
            file_node.size = len(content)
            file_node.modified = file_node._get_timestamp()
        return None
    
    def read_file(self, name):
        if name.startswith('/'):
            parts = [p for p in name.split('/') if p]
            node = self.root
            
            for part in parts:
                if part in node.children:
                    node = node.children[part]
                else:
                    return None, "cat: {}: No such file".format(name)
            
            if not node.is_directory:
                return node.content, None
            return None, "cat: {}: Is a directory".format(name)
        
        if name in self.current.children:
            node = self.current.children[name]
            if not node.is_directory:
                return node.content, None
        return None, "cat: {}: No such file".format(name)
    
    def remove_file(self, name):
        if name in self.current.children:
            del self.current.children[name]
        else:
            return "rm: cannot remove '{}': No such file".format(name)
        return None
    
    def copy_file(self, src, dest):
        if src in self.current.children and not self.current.children[src].is_directory:
            source = self.current.children[src]
            copy = FileNode(dest, False)
            copy.content = source.content
            copy.size = source.size
            self.current.children[dest] = copy
        else:
            return "cp: cannot copy '{}'".format(src)
        return None
    
    def move_file(self, src, dest):
        if src in self.current.children:
            source = self.current.children[src]
            del self.current.children[src]
            source.name = dest
            self.current.children[dest] = source
        else:
            return "mv: cannot move '{}'".format(src)
        return None
    
    def find(self, pattern):
        results = []
        for name in self.current.children.keys():
            if pattern == '*' or pattern in name:
                results.append(self.get_current_path() + '/' + name)
        return results
    
    def save(self, filename):
        try:
            with open(filename, 'w') as f:
                json.dump(self.root.to_dict(), f)
            return None
        except Exception as e:
            return "Error saving filesystem: {}".format(str(e))
    
    def load(self, filename):
        try:
            with open(filename, 'r') as f:
                data = json.load(f)
            self.root = FileNode.from_dict(data)
            self.current = self.root
            self.path_stack = ['/']
            return None
        except:
            return None  # File doesn't exist, use defaults


class SerialTerminal:
    def __init__(self, uart_id=0, baudrate=115200, tx_pin=0, rx_pin=1):
        """
        Initialize UART on GPIO pins
        Default: UART0, TX=GP0, RX=GP1, 115200 baud
        Alternative: UART1, TX=GP4, RX=GP5 (uart_id=1, tx_pin=4, rx_pin=5)
        """
        self.uart = UART(uart_id, baudrate=baudrate, tx=Pin(tx_pin), rx=Pin(rx_pin))
        self.uart.init(baudrate=baudrate, bits=8, parity=None, stop=1)
        
        # Enable flow control if using RTS/CTS pins
        # For UART0: RTS=GP2, CTS=GP3
        # For UART1: RTS=GP6, CTS=GP7
        # Uncomment below to enable hardware flow control:
        # self.rts = Pin(2 if uart_id == 0 else 6, Pin.OUT)
        # self.cts = Pin(3 if uart_id == 0 else 7, Pin.IN)
        
        self.running = True
        self.poll = select.poll()
        self.poll.register(self.uart, select.POLLIN)
        
        # Send VT100 initialization
        self.write('\x1b[2J')  # Clear screen
        self.write('\x1b[H')   # Home cursor
        self.write('\x1b[?7h') # Enable line wrap
        self.write('\x1b[?25h') # Show cursor
        
        # Flush any existing data
        time.sleep_ms(50)
        while self.uart.any():
            self.uart.read(1)
    
    def write(self, text):
        """Write text to UART with CRLF conversion"""
        if not text:
            return
        # Convert \n to \r\n for proper terminal display
        text = text.replace('\n', '\r\n')
        
        # Write in chunks to avoid buffer overflow
        chunk_size = 32
        for i in range(0, len(text), chunk_size):
            chunk = text[i:i+chunk_size]
            self.uart.write(chunk)
            # Small delay for terminal to process
            if len(chunk) == chunk_size and i + chunk_size < len(text):
                time.sleep_ms(2)
    
    def writeln(self, text=""):
        """Write line with newline"""
        self.write(text + '\n')
    
    def read_line(self):
        """Read a line of input from UART with echo"""
        line = ""
        
        while True:
            # Check if data available with timeout
            events = self.poll.poll(10)
            
            if events:
                char_byte = self.uart.read(1)
                if not char_byte:
                    continue
                
                char = char_byte.decode('utf-8', 'ignore')
                
                if char in ('\r', '\n'):
                    if line:
                        self.writeln()
                        return line
                elif char in ('\x7f', '\x08'):  # Backspace or DEL
                    if line:
                        line = line[:-1]
                        self.write('\b \b')
                elif char == '\x03':  # Ctrl+C
                    self.writeln('^C')
                    return ""
                elif char == '\x04':  # Ctrl+D (EOF)
                    if not line:
                        self.writeln("logout")
                        return "exit"
                elif char == '\x15':  # Ctrl+U (kill line)
                    for _ in range(len(line)):
                        self.write('\b \b')
                    line = ""
                elif char == '\x0c':  # Ctrl+L (refresh)
                    self.clear_screen()
                    return "clear"
                elif ord(char) >= 32 and ord(char) < 127:  # Printable ASCII
                    line += char
                    self.write(char)  # Echo character
    
    def clear_screen(self):
        """Send VT100 clear screen sequence"""
        self.write('\x1b[2J')  # Clear screen
        self.write('\x1b[H')   # Home cursor


class XenixOS:
    def __init__(self, uart_id=0, baudrate=115200, tx_pin=0, rx_pin=1):
        """
        Initialize Xenix OS with UART configuration
        
        UART0 (default): TX=GP0, RX=GP1
        UART1: TX=GP4, RX=GP5 (use uart_id=1, tx_pin=4, rx_pin=5)
        
        Optional RTS/CTS for flow control:
        UART0: RTS=GP2, CTS=GP3
        UART1: RTS=GP6, CTS=GP7
        """
        self.fs = FileSystem()
        self.current_user = "root"
        self.hostname = "pico"
        self.terminal = SerialTerminal(uart_id, baudrate, tx_pin, rx_pin)
        self.running = True
        self.save_file = "xenix_state.json"
        
        # Try to load saved state
        self.fs.load(self.save_file)
    
    def boot(self):
        self.terminal.clear_screen()
        self.terminal.writeln("Xenix System V/386 for Raspberry Pi Pico")
        self.terminal.writeln("Copyright (c) 1984-1989 Microsoft Corporation")
        self.terminal.writeln("All rights reserved.")
        self.terminal.writeln()
        
        # Display MOTD
        content, err = self.fs.read_file('/etc/motd')
        if content:
            self.terminal.writeln(content)
        
        self.terminal.writeln()
        self.terminal.writeln("Login: " + self.current_user)
        self.terminal.writeln("Welcome to Xenix")
        self.terminal.writeln()
        
        while self.running:
            prompt = "{}@{}:{}$ ".format(self.current_user, self.hostname, self.fs.get_current_path())
            self.terminal.write(prompt)
            cmd_line = self.terminal.read_line()
            
            if cmd_line:
                self.execute_command(cmd_line)
    
    def execute_command(self, cmd_line):
        parts = cmd_line.strip().split()
        if not parts:
            return
        
        cmd = parts[0]
        args = parts[1:]
        
        commands = {
            'ls': self.cmd_ls,
            'cd': self.cmd_cd,
            'pwd': self.cmd_pwd,
            'mkdir': self.cmd_mkdir,
            'rmdir': self.cmd_rmdir,
            'cat': self.cmd_cat,
            'echo': self.cmd_echo,
            'touch': self.cmd_touch,
            'rm': self.cmd_rm,
            'cp': self.cmd_cp,
            'mv': self.cmd_mv,
            'find': self.cmd_find,
            'grep': self.cmd_grep,
            'ps': self.cmd_ps,
            'who': self.cmd_who,
            'date': self.cmd_date,
            'clear': self.cmd_clear,
            'help': self.cmd_help,
            'exit': self.cmd_exit,
            'uname': self.cmd_uname,
            'df': self.cmd_df,
            'free': self.cmd_free,
            'vi': self.cmd_vi,
            'ed': self.cmd_ed,
            'gpio': self.cmd_gpio,
        }
        
        if cmd in commands:
            commands[cmd](args)
        else:
            self.terminal.writeln("{}: command not found".format(cmd))
    
    def cmd_ls(self, args):
        files = self.fs.list_files()
        long_format = '-l' in args
        
        if long_format:
            for f in files:
                self.terminal.writeln("{} {} {} {:8d} {} {}".format(
                    f.permissions, f.owner, f.group, f.size, f.modified, f.name))
        else:
            line = "  ".join([f.name for f in files])
            if line:
                self.terminal.writeln(line)
    
    def cmd_cd(self, args):
        if not args:
            err = self.fs.change_directory('/home/' + self.current_user)
        else:
            err = self.fs.change_directory(args[0])
        if err:
            self.terminal.writeln(err)
    
    def cmd_pwd(self, args):
        self.terminal.writeln(self.fs.get_current_path())
    
    def cmd_mkdir(self, args):
        if not args:
            self.terminal.writeln("mkdir: missing operand")
            return
        err = self.fs.create_directory(args[0])
        if err:
            self.terminal.writeln(err)
    
    def cmd_rmdir(self, args):
        if not args:
            self.terminal.writeln("rmdir: missing operand")
            return
        err = self.fs.remove_directory(args[0])
        if err:
            self.terminal.writeln(err)
    
    def cmd_cat(self, args):
        if not args:
            self.terminal.writeln("cat: missing operand")
            return
        content, err = self.fs.read_file(args[0])
        if err:
            self.terminal.writeln(err)
        elif content:
            self.terminal.writeln(content)
    
    def cmd_echo(self, args):
        if not args:
            self.terminal.writeln()
            return
        
        # Check for redirect
        if '>' in args:
            idx = args.index('>')
            text = ' '.join(args[:idx])
            if idx + 1 < len(args):
                filename = args[idx + 1]
                self.fs.write_file(filename, text)
            return
        
        self.terminal.writeln(' '.join(args))
    
    def cmd_touch(self, args):
        if not args:
            self.terminal.writeln("touch: missing operand")
            return
        self.fs.create_file(args[0])
    
    def cmd_rm(self, args):
        if not args:
            self.terminal.writeln("rm: missing operand")
            return
        err = self.fs.remove_file(args[0])
        if err:
            self.terminal.writeln(err)
    
    def cmd_cp(self, args):
        if len(args) < 2:
            self.terminal.writeln("cp: missing operand")
            return
        err = self.fs.copy_file(args[0], args[1])
        if err:
            self.terminal.writeln(err)
    
    def cmd_mv(self, args):
        if len(args) < 2:
            self.terminal.writeln("mv: missing operand")
            return
        err = self.fs.move_file(args[0], args[1])
        if err:
            self.terminal.writeln(err)
    
    def cmd_find(self, args):
        pattern = args[0] if args else '*'
        results = self.fs.find(pattern)
        for path in results:
            self.terminal.writeln(path)
    
    def cmd_grep(self, args):
        if len(args) < 2:
            self.terminal.writeln("grep: missing operand")
            return
        pattern = args[0]
        filename = args[1]
        content, err = self.fs.read_file(filename)
        if err:
            self.terminal.writeln(err)
        elif content:
            for line in content.split('\n'):
                if pattern in line:
                    self.terminal.writeln(line)
    
    def cmd_ps(self, args):
        self.terminal.writeln("  PID TTY      TIME CMD")
        self.terminal.writeln("    1 ttyUSB0  0:00 init")
        self.terminal.writeln("   42 ttyUSB0  0:01 xenix")
    
    def cmd_who(self, args):
        self.terminal.writeln("{}     ttyUSB0".format(self.current_user))
    
    def cmd_date(self, args):
        try:
            t = time.localtime()
            days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
            months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
            self.terminal.writeln("{} {} {:2d} {:02d}:{:02d}:{:02d} UTC {}".format(
                days[t[6]], months[t[1]-1], t[2], t[3], t[4], t[5], t[0]))
        except:
            self.terminal.writeln("Date unavailable")
    
    def cmd_clear(self, args):
        self.terminal.clear_screen()
    
    def cmd_uname(self, args):
        self.terminal.writeln("Xenix pico 3.0 arm")
    
    def cmd_df(self, args):
        self.terminal.writeln("Filesystem     1K-blocks  Used Available Use% Mounted on")
        self.terminal.writeln("/dev/flash0       256000 12800    243200   5% /")
    
    def cmd_free(self, args):
        import gc
        gc.collect()
        free = gc.mem_free()
        total = gc.mem_alloc() + free
        used = gc.mem_alloc()
        self.terminal.writeln("       total     used     free")
        self.terminal.writeln("Mem: {:7d} {:8d} {:8d}".format(total, used, free))
    
    def cmd_vi(self, args):
        if not args:
            self.terminal.writeln("vi: missing filename")
            return
        
        filename = args[0]
        content, _ = self.fs.read_file(filename)
        if content is None:
            content = ""
        
        lines = content.split('\n') if content else ['']
        
        self.terminal.writeln('"{}" {} lines'.format(filename, len(lines)))
        self.terminal.writeln("Simple editor - Commands: :w (save), :q (quit), :wq (save & quit), i (insert), ESC (exit insert)")
        
        editing = True
        insert_mode = False
        current_line = 0
        
        while editing:
            if not insert_mode:
                self.terminal.writeln("\n--- Line {} of {} ---".format(current_line + 1, len(lines)))
                if current_line < len(lines):
                    self.terminal.writeln(lines[current_line])
                self.terminal.write(":")
            
            input_text = self.terminal.read_line()
            
            if insert_mode:
                if input_text in ('ESC', 'esc'):
                    insert_mode = False
                    self.terminal.writeln("-- COMMAND MODE --")
                else:
                    if current_line >= len(lines):
                        lines.append(input_text)
                    else:
                        lines[current_line] = input_text
                    current_line += 1
                    if current_line >= len(lines):
                        lines.append('')
            else:
                cmd = input_text.strip()
                if cmd == 'i':
                    insert_mode = True
                    self.terminal.writeln("-- INSERT MODE -- (type ESC to exit)")
                elif cmd in ('w', ':w'):
                    self.fs.write_file(filename, '\n'.join(lines))
                    self.terminal.writeln('"{}" {} lines written'.format(filename, len(lines)))
                elif cmd in ('q', ':q'):
                    editing = False
                elif cmd in ('wq', ':wq'):
                    self.fs.write_file(filename, '\n'.join(lines))
                    self.terminal.writeln('"{}" {} lines written'.format(filename, len(lines)))
                    editing = False
                elif cmd == 'n':
                    if current_line < len(lines) - 1:
                        current_line += 1
                elif cmd == 'p':
                    if current_line > 0:
                        current_line -= 1
                else:
                    self.terminal.writeln("Unknown command")
    
    def cmd_ed(self, args):
        if not args:
            self.terminal.writeln("ed: missing filename")
            return
        
        filename = args[0]
        content, _ = self.fs.read_file(filename)
        if content is None:
            content = ""
        
        self.terminal.writeln("ED line editor. Commands: a (append), p (print), w (write), q (quit)")
        
        buffer = content
        editing = True
        
        while editing:
            self.terminal.write("*")
            cmd = self.terminal.read_line().strip()
            
            if cmd == 'a':
                self.terminal.writeln("Enter text (type . to finish):")
                while True:
                    line = self.terminal.read_line()
                    if line == '.':
                        break
                    buffer += line + '\n'
            elif cmd == 'p':
                self.terminal.writeln(buffer)
            elif cmd == 'w':
                self.fs.write_file(filename, buffer)
                self.terminal.writeln("{} bytes written".format(len(buffer)))
            elif cmd == 'q':
                editing = False
            else:
                self.terminal.writeln("?")
    
    def cmd_help(self, args):
        self.terminal.writeln("Available commands:")
        self.terminal.writeln("File: ls, cd, pwd, mkdir, rmdir, cat, echo, touch, rm, cp, mv, find")
        self.terminal.writeln("Text: grep, vi, ed")
        self.terminal.writeln("System: ps, who, date, clear, uname, df, free, gpio")
        self.terminal.writeln("Other: help, exit")
    
    def cmd_gpio(self, args):
        """Show GPIO pin configuration"""
        self.terminal.writeln("GPIO Pin Configuration:")
        self.terminal.writeln("UART0: TX=GP0, RX=GP1 (default)")
        self.terminal.writeln("       RTS=GP2, CTS=GP3 (optional flow control)")
        self.terminal.writeln("UART1: TX=GP4, RX=GP5")
        self.terminal.writeln("       RTS=GP6, CTS=GP7 (optional flow control)")
        self.terminal.writeln("")
        self.terminal.writeln("Current UART: UART" + str(self.terminal.uart))
    
    def cmd_exit(self, args):
        self.terminal.writeln("Saving filesystem state...")
        err = self.fs.save(self.save_file)
        if err:
            self.terminal.writeln(err)
        else:
            self.terminal.writeln("Filesystem saved.")
        self.terminal.writeln("logout")
        self.running = False


# Main entry point
def main():
    """
    Main entry point - Configure your UART pins here
    
    Example configurations:
    1. UART0 (default): os = XenixOS(uart_id=0, baudrate=115200, tx_pin=0, rx_pin=1)
    2. UART1: os = XenixOS(uart_id=1, baudrate=115200, tx_pin=4, rx_pin=5)
    3. Custom baud: os = XenixOS(uart_id=0, baudrate=9600, tx_pin=0, rx_pin=1)
    """
    
    # Default configuration - UART0 on GP0 (TX) and GP1 (RX) at 115200 baud
    os = XenixOS(uart_id=0, baudrate=115200, tx_pin=0, rx_pin=1)
    
    # Alternative: Use UART1 on GP4 (TX) and GP5 (RX)
    # os = XenixOS(uart_id=1, baudrate=115200, tx_pin=4, rx_pin=5)
    
    os.boot()


if __name__ == '__main__':
    main()
