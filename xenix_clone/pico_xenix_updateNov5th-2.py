import sys
import time
import json
import os
from machine import UART, Pin
import _thread

# UART Configuration
UART_ID = 0          # UART0 (GPIO 0/1) or UART1 (GPIO 4/5)
DEFAULT_BAUD = 115200
TX_PIN = 0           # GPIO 0 for UART0 TX
RX_PIN = 1           # GPIO 1 for UART0 RX

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
        self._create_initial_structure()
        self.lock = _thread.allocate_lock()
    
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
    
    def get_path(self, path_stack):
        return '/'.join(path_stack).replace('//', '/')
    
    def list_files(self, node):
        with self.lock:
            return list(node.children.values())
    
    def change_directory(self, current, path_stack, path):
        with self.lock:
            if path == '..':
                if len(path_stack) > 1:
                    path_stack.pop()
                    current = self._navigate_to_path(path_stack)
            elif path == '/':
                path_stack[:] = ['/']
                current = self.root
            elif path.startswith('/'):
                parts = [p for p in path.split('/') if p]
                new_path = ['/']
                node = self.root
                
                for part in parts:
                    if part in node.children and node.children[part].is_directory:
                        node = node.children[part]
                        new_path.append(part)
                    else:
                        return current, "cd: {}: No such directory".format(path)
                
                path_stack[:] = new_path
                current = node
            else:
                if path in current.children and current.children[path].is_directory:
                    path_stack.append(path)
                    current = current.children[path]
                else:
                    return current, "cd: {}: No such directory".format(path)
            return current, None
    
    def _navigate_to_path(self, path):
        node = self.root
        for i in range(1, len(path)):
            if path[i] in node.children:
                node = node.children[path[i]]
            else:
                return self.root
        return node
    
    def create_directory(self, current, name):
        with self.lock:
            if name not in current.children:
                current.children[name] = FileNode(name, True)
            else:
                return "mkdir: cannot create directory '{}': File exists".format(name)
            return None
    
    def remove_directory(self, current, name):
        with self.lock:
            if name in current.children:
                node = current.children[name]
                if node.is_directory and len(node.children) == 0:
                    del current.children[name]
                else:
                    return "rmdir: failed to remove '{}'".format(name)
            else:
                return "rmdir: failed to remove '{}': No such directory".format(name)
            return None
    
    def create_file(self, current, name):
        with self.lock:
            if name not in current.children:
                current.children[name] = FileNode(name, False)
            return None
    
    def write_file(self, current, name, content):
        with self.lock:
            if name in current.children:
                file_node = current.children[name]
            else:
                file_node = FileNode(name, False)
                current.children[name] = file_node
            
            if not file_node.is_directory:
                file_node.content = content
                file_node.size = len(content)
                file_node.modified = file_node._get_timestamp()
            return None
    
    def read_file(self, current, name):
        with self.lock:
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
            
            if name in current.children:
                node = current.children[name]
                if not node.is_directory:
                    return node.content, None
            return None, "cat: {}: No such file".format(name)
    
    def remove_file(self, current, name):
        with self.lock:
            if name in current.children:
                del current.children[name]
            else:
                return "rm: cannot remove '{}': No such file".format(name)
            return None
    
    def copy_file(self, current, src, dest):
        with self.lock:
            if src in current.children and not current.children[src].is_directory:
                source = current.children[src]
                copy = FileNode(dest, False)
                copy.content = source.content
                copy.size = source.size
                current.children[dest] = copy
            else:
                return "cp: cannot copy '{}'".format(src)
            return None
    
    def move_file(self, current, src, dest):
        with self.lock:
            if src in current.children:
                source = current.children[src]
                del current.children[src]
                source.name = dest
                current.children[dest] = source
            else:
                return "mv: cannot move '{}'".format(src)
            return None
    
    def find(self, current, path_stack, pattern):
        with self.lock:
            results = []
            for name in current.children.keys():
                if pattern == '*' or pattern in name:
                    results.append(self.get_path(path_stack) + '/' + name)
            return results
    
    def save(self, filename):
        with self.lock:
            try:
                with open(filename, 'w') as f:
                    json.dump(self.root.to_dict(), f)
                return None
            except Exception as e:
                return "Error saving filesystem: {}".format(str(e))
    
    def load(self, filename):
        with self.lock:
            try:
                with open(filename, 'r') as f:
                    data = json.load(f)
                self.root = FileNode.from_dict(data)
                return None
            except:
                return None  # File doesn't exist, use defaults


class SerialTerminal:
    """
    Terminal supporting both USB serial (sys.stdin/stdout) and UART pins.
    """
    
    BAUD_RATES = [300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200]
    
    def __init__(self, name, use_uart=False, uart_id=0, baud_rate=115200, tx_pin=0, rx_pin=1):
        self.name = name
        self.use_uart = use_uart
        self.uart = None
        self.baud_rate = baud_rate
        self.running = True
        
        if use_uart:
            try:
                self.uart = UART(uart_id, baudrate=baud_rate, 
                               tx=Pin(tx_pin), rx=Pin(rx_pin),
                               bits=8, parity=None, stop=1)
                self.uart.init(baudrate=baud_rate, bits=8, parity=None, stop=1)
                print(f"{name}: UART{uart_id} initialized: {baud_rate} baud, TX=GPIO{tx_pin}, RX=GPIO{rx_pin}")
            except Exception as e:
                print(f"{name}: UART init failed: {e}")
                self.use_uart = False
    
    def set_baud_rate(self, baud_rate):
        if baud_rate not in self.BAUD_RATES:
            return f"Unsupported baud rate. Supported: {', '.join(map(str, self.BAUD_RATES))}"
        
        if self.uart:
            try:
                self.uart.init(baudrate=baud_rate, bits=8, parity=None, stop=1)
                self.baud_rate = baud_rate
                return f"Baud rate changed to {baud_rate}"
            except Exception as e:
                return f"Error changing baud rate: {e}"
        else:
            return "UART not initialized"
    
    def write(self, text):
        text = text.replace('\n', '\r\n')
        
        if self.use_uart and self.uart:
            self.uart.write(text.encode())
        else:
            sys.stdout.write(text)
    
    def writeln(self, text=""):
        self.write(text + '\n')
    
    def read_line(self):
        line = ""
        while self.running:
            if self.use_uart and self.uart:
                if self.uart.any():
                    char = self.uart.read(1).decode('utf-8', 'ignore')
                else:
                    time.sleep_ms(10)
                    continue
            else:
                char = sys.stdin.read(1)
            
            if char in ('\r', '\n'):
                if line:
                    self.writeln()
                    return line
            elif char in ('\x7f', '\x08'):
                if line:
                    line = line[:-1]
                    self.write('\b \b')
            elif char == '\x03':
                self.writeln('^C')
                return ""
            elif char == '\x04':
                if not line:
                    self.writeln("logout")
                    return "exit"
            elif char == '\x15':
                for _ in range(len(line)):
                    self.write('\b \b')
                line = ""
            elif ord(char) >= 32 and ord(char) < 127:
                line += char
                self.write(char)
        return None
    
    def clear_screen(self):
        self.write('\x1b[2J')
        self.write('\x1b[H')


class XenixSession:
    """Individual user session"""
    def __init__(self, fs, terminal, hostname="pico"):
        self.fs = fs
        self.terminal = terminal
        self.hostname = hostname
        self.current_user = "root"
        self.current = self.fs.root
        self.path_stack = ["/"]
        self.running = True
    
    def boot(self):
        self.terminal.clear_screen()
        self.terminal.writeln("Xenix System V/386 for Raspberry Pi Pico")
        self.terminal.writeln("Copyright (c) 1984-1991 Microsoft Corporation")
        self.terminal.writeln(f"Terminal: {self.terminal.name}")
        self.terminal.writeln("")
        
        # Show MOTD
        content, _ = self.fs.read_file(self.current, '/etc/motd')
        if content:
            self.terminal.writeln(content)
        
        self.terminal.writeln("")
        self.terminal.write("login: ")
        username = self.terminal.read_line()
        
        if username:
            self.current_user = username
            self.terminal.writeln(f"Welcome to Xenix, {username}!")
            self.terminal.writeln("")
        
        self.run_shell()
    
    def run_shell(self):
        while self.running and self.terminal.running:
            prompt = "{}@{}:{} # ".format(self.current_user, self.hostname, 
                                         self.fs.get_path(self.path_stack))
            self.terminal.write(prompt)
            
            command_line = self.terminal.read_line()
            
            if command_line is None:
                break
            
            if command_line:
                self.execute_command(command_line)
    
    def execute_command(self, command_line):
        parts = command_line.strip().split()
        if not parts:
            return
        
        command = parts[0]
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
            'uname': self.cmd_uname,
            'df': self.cmd_df,
            'free': self.cmd_free,
            'vi': self.cmd_vi,
            'ed': self.cmd_ed,
            'help': self.cmd_help,
            'exit': self.cmd_exit,
            'logout': self.cmd_exit,
            'baud': self.cmd_baud,
            'stty': self.cmd_stty,
        }
        
        if command in commands:
            commands[command](args)
        else:
            self.terminal.writeln("{}: command not found".format(command))
    
    def cmd_ls(self, args):
        files = self.fs.list_files(self.current)
        
        if '-l' in args:
            for f in files:
                self.terminal.writeln("{} 1 {} {} {:6d} {} {}".format(
                    f.permissions, f.owner, f.group, f.size, f.modified, f.name))
        else:
            names = [f.name for f in files]
            if names:
                self.terminal.writeln('  '.join(names))
    
    def cmd_cd(self, args):
        path = args[0] if args else '/home/root'
        self.current, err = self.fs.change_directory(self.current, self.path_stack, path)
        if err:
            self.terminal.writeln(err)
    
    def cmd_pwd(self, args):
        self.terminal.writeln(self.fs.get_path(self.path_stack))
    
    def cmd_mkdir(self, args):
        if not args:
            self.terminal.writeln("mkdir: missing operand")
            return
        err = self.fs.create_directory(self.current, args[0])
        if err:
            self.terminal.writeln(err)
    
    def cmd_rmdir(self, args):
        if not args:
            self.terminal.writeln("rmdir: missing operand")
            return
        err = self.fs.remove_directory(self.current, args[0])
        if err:
            self.terminal.writeln(err)
    
    def cmd_cat(self, args):
        if not args:
            self.terminal.writeln("cat: missing operand")
            return
        content, err = self.fs.read_file(self.current, args[0])
        if err:
            self.terminal.writeln(err)
        elif content:
            self.terminal.writeln(content)
    
    def cmd_echo(self, args):
        if not args:
            self.terminal.writeln("")
            return
        self.terminal.writeln(' '.join(args))
    
    def cmd_touch(self, args):
        if not args:
            self.terminal.writeln("touch: missing operand")
            return
        self.fs.create_file(self.current, args[0])
    
    def cmd_rm(self, args):
        if not args:
            self.terminal.writeln("rm: missing operand")
            return
        err = self.fs.remove_file(self.current, args[0])
        if err:
            self.terminal.writeln(err)
    
    def cmd_cp(self, args):
        if len(args) < 2:
            self.terminal.writeln("cp: missing operand")
            return
        err = self.fs.copy_file(self.current, args[0], args[1])
        if err:
            self.terminal.writeln(err)
    
    def cmd_mv(self, args):
        if len(args) < 2:
            self.terminal.writeln("mv: missing operand")
            return
        err = self.fs.move_file(self.current, args[0], args[1])
        if err:
            self.terminal.writeln(err)
    
    def cmd_find(self, args):
        pattern = args[0] if args else '*'
        results = self.fs.find(self.current, self.path_stack, pattern)
        for path in results:
            self.terminal.writeln(path)
    
    def cmd_grep(self, args):
        if len(args) < 2:
            self.terminal.writeln("grep: missing operand")
            return
        pattern = args[0]
        filename = args[1]
        content, err = self.fs.read_file(self.current, filename)
        if err:
            self.terminal.writeln(err)
        elif content:
            for line in content.split('\n'):
                if pattern in line:
                    self.terminal.writeln(line)
    
    def cmd_ps(self, args):
        self.terminal.writeln("  PID TTY      TIME CMD")
        self.terminal.writeln("    1 tty0     0:00 init")
        self.terminal.writeln("   42 {}  0:01 xenix".format(self.terminal.name))
    
    def cmd_who(self, args):
        self.terminal.writeln("{}     {}".format(self.current_user, self.terminal.name))
    
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
    
    def cmd_baud(self, args):
        if not args:
            self.terminal.writeln(f"Current baud rate: {self.terminal.baud_rate}")
            self.terminal.writeln(f"Supported rates: {', '.join(map(str, SerialTerminal.BAUD_RATES))}")
            return
        
        try:
            baud = int(args[0])
            result = self.terminal.set_baud_rate(baud)
            self.terminal.writeln(result)
        except ValueError:
            self.terminal.writeln("Invalid baud rate")
    
    def cmd_stty(self, args):
        if self.terminal.use_uart:
            self.terminal.writeln(f"speed {self.terminal.baud_rate} baud")
            self.terminal.writeln("line = 0")
            self.terminal.writeln("intr = ^C; quit = ^\\; erase = ^?; kill = ^U; eof = ^D")
        else:
            self.terminal.writeln("speed 115200 baud (USB serial)")
    
    def cmd_vi(self, args):
        if not args:
            self.terminal.writeln("vi: missing filename")
            return
        
        filename = args[0]
        content, _ = self.fs.read_file(self.current, filename)
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
                    self.fs.write_file(self.current, filename, '\n'.join(lines))
                    self.terminal.writeln('"{}" {} lines written'.format(filename, len(lines)))
                elif cmd in ('q', ':q'):
                    editing = False
                elif cmd in ('wq', ':wq'):
                    self.fs.write_file(self.current, filename, '\n'.join(lines))
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
        content, _ = self.fs.read_file(self.current, filename)
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
                self.fs.write_file(self.current, filename, buffer)
                self.terminal.writeln("{} bytes written".format(len(buffer)))
            elif cmd == 'q':
                editing = False
            else:
                self.terminal.writeln("?")
    
    def cmd_help(self, args):
        self.terminal.writeln("Available commands:")
        self.terminal.writeln("File: ls, cd, pwd, mkdir, rmdir, cat, echo, touch, rm, cp, mv, find")
        self.terminal.writeln("Text: grep, vi, ed")
        self.terminal.writeln("System: ps, who, date, clear, uname, df, free, baud, stty")
        self.terminal.writeln("Other: help, exit, logout")
    
    def cmd_exit(self, args):
        self.terminal.writeln("logout")
        self.running = False


def uart_session_handler(fs, uart_id, baud_rate, tx_pin, rx_pin):
    """Thread handler for UART terminal session"""
    terminal = SerialTerminal(f"ttyS{uart_id}", True, uart_id, baud_rate, tx_pin, rx_pin)
    session = XenixSession(fs, terminal)
    session.boot()


def main():
    """
    Multi-user Xenix OS with simultaneous USB and UART terminals.
    
    USB console always available on sys.stdin/stdout
    UART terminal on GPIO pins (configurable below)
    """
    
    # Shared filesystem
    fs = FileSystem()
    save_file = "xenix_state.json"
    fs.load(save_file)
    
    # Start UART session in separate thread
    ENABLE_UART = True  # Set to False to disable UART terminal
    
    if ENABLE_UART:
        try:
            _thread.start_new_thread(uart_session_handler, 
                                   (fs, 0, 115200, 0, 1))
            print("UART terminal started on GPIO 0/1")
        except Exception as e:
            print(f"Failed to start UART terminal: {e}")
    
    # Run USB console session in main thread
    usb_terminal = SerialTerminal("ttyUSB0", False)
    usb_session = XenixSession(fs, usb_terminal)
    
    try:
        usb_session.boot()
    finally:
        # Save filesystem on exit
        print("Saving filesystem state...")
        err = fs.save(save_file)
        if err:
            print(err)
        else:
            print("Filesystem saved.")


if __name__ == '__main__':
    main()
