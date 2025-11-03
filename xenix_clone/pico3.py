import sys
import time
import json
import os
from machine import UART, Pin
import select
import _thread
import hashlib

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


class UserManager:
    def __init__(self, fs):
        self.fs = fs
        self.users = {}
        self._load_passwd()
    
    def _hash_password(self, password):
        """Simple hash for password storage"""
        return hashlib.sha256(password.encode()).hexdigest()[:16]
    
    def _load_passwd(self):
        """Load users from /etc/passwd"""
        content, err = self.fs.read_file('/etc/passwd')
        if not err and content:
            for line in content.strip().split('\n'):
                if line:
                    parts = line.split(':')
                    if len(parts) >= 3:
                        username = parts[0]
                        password = parts[1]
                        homedir = parts[2]
                        self.users[username] = {
                            'password': password,
                            'homedir': homedir
                        }
        
        # Ensure root exists
        if 'root' not in self.users:
            self.users['root'] = {
                'password': '',  # No password for root
                'homedir': '/home/root'
            }
            self._save_passwd()
    
    def _save_passwd(self):
        """Save users to /etc/passwd"""
        lines = []
        for username, info in self.users.items():
            lines.append("{}:{}:{}".format(username, info['password'], info['homedir']))
        self.fs.write_file('/etc/passwd', '\n'.join(lines))
    
    def add_user(self, username, password=''):
        """Add a new user"""
        if username in self.users:
            return "useradd: user '{}' already exists".format(username)
        
        homedir = '/home/' + username
        
        # Create home directory
        old_current = self.fs.current
        old_path = list(self.fs.path_stack)
        
        self.fs.change_directory('/home')
        err = self.fs.create_directory(username)
        if err:
            self.fs.current = old_current
            self.fs.path_stack = old_path
            return err
        
        # Change to user's home directory
        self.fs.change_directory(username)
        
        # Create .bashrc
        bashrc_content = "# .bashrc for {}\n# Add your aliases and startup commands here\necho 'Welcome, {}!'\n".format(username, username)
        self.fs.write_file('.bashrc', bashrc_content)
        
        # Create empty .bash_history
        self.fs.write_file('.bash_history', '')
        
        # Restore original directory
        self.fs.current = old_current
        self.fs.path_stack = old_path
        
        # Add user to passwd
        hashed_pw = self._hash_password(password) if password else ''
        self.users[username] = {
            'password': hashed_pw,
            'homedir': homedir
        }
        self._save_passwd()
        
        return None
    
    def remove_user(self, username):
        """Remove a user"""
        if username not in self.users:
            return "rmuser: user '{}' does not exist".format(username)
        
        if username == 'root':
            return "rmuser: cannot remove root user"
        
        # Remove from users dict
        del self.users[username]
        self._save_passwd()
        
        return None
    
    def authenticate(self, username, password):
        """Authenticate a user"""
        if username not in self.users:
            return False
        
        user_info = self.users[username]
        stored_hash = user_info['password']
        
        # Root with no password
        if username == 'root' and stored_hash == '':
            return True
        
        # Check password
        if stored_hash == self._hash_password(password):
            return True
        
        return False
    
    def get_homedir(self, username):
        """Get user's home directory"""
        if username in self.users:
            return self.users[username]['homedir']
        return '/home/' + username
    
    def user_exists(self, username):
        """Check if user exists"""
        return username in self.users


class UserSession:
    def __init__(self, username, fs):
        self.username = username
        self.fs = fs
        self.history = []
        self._load_history()
    
    def _load_history(self):
        """Load command history from .bash_history"""
        homedir = self.fs.root
        path_parts = [p for p in ('/home/' + self.username).split('/') if p]
        
        for part in path_parts:
            if part in homedir.children:
                homedir = homedir.children[part]
            else:
                return
        
        if '.bash_history' in homedir.children:
            content = homedir.children['.bash_history'].content
            if content:
                self.history = content.strip().split('\n')
    
    def _save_history(self):
        """Save command history to .bash_history"""
        old_current = self.fs.current
        old_path = list(self.fs.path_stack)
        
        self.fs.change_directory('/home/' + self.username)
        history_text = '\n'.join(self.history[-100:])  # Keep last 100 commands
        self.fs.write_file('.bash_history', history_text)
        
        self.fs.current = old_current
        self.fs.path_stack = old_path
    
    def add_to_history(self, command):
        """Add command to history"""
        if command.strip():
            self.history.append(command)
            self._save_history()
    
    def get_history(self):
        """Get command history"""
        return self.history
    
    def execute_bashrc(self, terminal):
        """Execute .bashrc for user"""
        old_current = self.fs.current
        old_path = list(self.fs.path_stack)
        
        content, err = self.fs.read_file('/home/{}/.bashrc'.format(self.username))
        if content:
            for line in content.split('\n'):
                line = line.strip()
                if line and not line.startswith('#'):
                    # Parse simple commands
                    if line.startswith('echo '):
                        text = line[5:].strip('\'"')
                        terminal.writeln(text)
                    elif line.startswith('alias '):
                        # Store aliases if needed
                        pass
        
        self.fs.current = old_current
        self.fs.path_stack = old_path


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
        
        # Create root's .bashrc
        root_home = home.children['root']
        bashrc = FileNode('.bashrc', False)
        bashrc.content = "# .bashrc for root\necho 'Root privileges active'\n"
        bashrc.size = len(bashrc.content)
        root_home.children['.bashrc'] = bashrc
        
        # Create root's .bash_history
        history = FileNode('.bash_history', False)
        history.content = ""
        root_home.children['.bash_history'] = history
        
        # Create default MOTD
        etc = self.root.children['etc']
        motd = FileNode('motd', False)
        motd.content = "Welcome to Xenix System V on Raspberry Pi Pico!\n\nMicroPython " + sys.version + "\n"
        motd.size = len(motd.content)
        etc.children['motd'] = motd
        
        # Create passwd file
        passwd = FileNode('passwd', False)
        passwd.content = "root::/home/root"
        passwd.size = len(passwd.content)
        etc.children['passwd'] = passwd
    
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
            return None


class SerialTerminal:
    def __init__(self, uart_id=0, baudrate=115200, tx_pin=0, rx_pin=1):
        self.uart = UART(uart_id, baudrate=baudrate, tx=Pin(tx_pin), rx=Pin(rx_pin))
        self.uart.init(baudrate=baudrate, bits=8, parity=None, stop=1)
        self.uart_id = uart_id
        
        self.running = True
        self.poll = select.poll()
        self.poll.register(self.uart, select.POLLIN)
        
        # Send VT100 initialization
        self.write('\x1b[2J')
        self.write('\x1b[H')
        self.write('\x1b[?7h')
        self.write('\x1b[?25h')
        
        time.sleep_ms(50)
        while self.uart.any():
            self.uart.read(1)
    
    def write(self, text):
        if not text:
            return
        text = text.replace('\n', '\r\n')
        
        chunk_size = 32
        for i in range(0, len(text), chunk_size):
            chunk = text[i:i+chunk_size]
            self.uart.write(chunk)
            if len(chunk) == chunk_size and i + chunk_size < len(text):
                time.sleep_ms(2)
    
    def writeln(self, text=""):
        self.write(text + '\n')
    
    def read_line(self, echo=True):
        line = ""
        
        while True:
            events = self.poll.poll(10)
            
            if events:
                char_byte = self.uart.read(1)
                if not char_byte:
                    continue
                
                char = char_byte.decode('utf-8', 'ignore')
                
                if char in ('\r', '\n'):
                    if line or not echo:
                        self.writeln()
                        return line
                elif char in ('\x7f', '\x08'):
                    if line:
                        line = line[:-1]
                        if echo:
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
                        if echo:
                            self.write('\b \b')
                    line = ""
                elif char == '\x0c':
                    self.clear_screen()
                    return "clear"
                elif ord(char) >= 32 and ord(char) < 127:
                    line += char
                    if echo:
                        self.write(char)
    
    def read_password(self):
        """Read password without echo"""
        return self.read_line(echo=False)
    
    def clear_screen(self):
        self.write('\x1b[2J')
        self.write('\x1b[H')


class XenixOS:
    def __init__(self, uart_id=0, baudrate=115200, tx_pin=0, rx_pin=1):
        self.fs = FileSystem()
        self.user_manager = UserManager(self.fs)
        self.current_user = None
        self.session = None
        self.hostname = "pico"
        self.terminal = SerialTerminal(uart_id, baudrate, tx_pin, rx_pin)
        self.running = True
        self.save_file = "xenix_state.json"
        
        # Try to load saved state
        self.fs.load(self.save_file)
        self.user_manager._load_passwd()
    
    def login(self):
        """Handle user login"""
        attempts = 0
        max_attempts = 3
        
        while attempts < max_attempts:
            self.terminal.write("login: ")
            username = self.terminal.read_line()
            
            if not username:
                attempts += 1
                continue
            
            if not self.user_manager.user_exists(username):
                self.terminal.writeln("Login incorrect")
                attempts += 1
                time.sleep(1)
                continue
            
            self.terminal.write("Password: ")
            password = self.terminal.read_password()
            
            if self.user_manager.authenticate(username, password):
                self.current_user = username
                self.session = UserSession(username, self.fs)
                
                # Change to user's home directory
                homedir = self.user_manager.get_homedir(username)
                self.fs.change_directory(homedir)
                
                return True
            else:
                self.terminal.writeln("Login incorrect")
                attempts += 1
                time.sleep(1)
        
        return False
    
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
        
        # Login process
        if not self.login():
            self.terminal.writeln("Too many login attempts")
            return
        
        self.terminal.writeln("Welcome to Xenix")
        self.terminal.writeln()
        
        # Execute .bashrc
        self.session.execute_bashrc(self.terminal)
        self.terminal.writeln()
        
        while self.running:
            prompt = "{}@{}:{}$ ".format(self.current_user, self.hostname, self.fs.get_current_path())
            self.terminal.write(prompt)
            cmd_line = self.terminal.read_line()
            
            if cmd_line:
                self.session.add_to_history(cmd_line)
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
            'useradd': self.cmd_useradd,
            'rmuser': self.cmd_rmuser,
            'history': self.cmd_history,
            'passwd': self.cmd_passwd,
        }
        
        if cmd in commands:
            commands[cmd](args)
        else:
            self.terminal.writeln("{}: command not found".format(cmd))
    
    def cmd_ls(self, args):
        files = self.fs.list_files()
        long_format = '-l' in args
        show_hidden = '-a' in args
        
        if not show_hidden:
            files = [f for f in files if not f.name.startswith('.')]
        
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
            homedir = self.user_manager.get_homedir(self.current_user)
            err = self.fs.change_directory(homedir)
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
        self.terminal.writeln("  PID TTY          TIME CMD")
        self.terminal.writeln("    1 ttyUSB{}     0:00 init".format(self.terminal.uart_id))
        self.terminal.writeln("   42 ttyUSB{}     0:01 xenix".format(self.terminal.uart_id))
    
    def cmd_who(self, args):
        self.terminal.writeln("{}     ttyUSB{}".format(self.current_user, self.terminal.uart_id))
    
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
    
    def cmd_useradd(self, args):
        """Add a new user"""
        if self.current_user != 'root':
            self.terminal.writeln("useradd: permission denied (root only)")
            return
        
        if not args:
            self.terminal.writeln("useradd: missing username")
            return
        
        username = args[0]
        
        # Prompt for password
        self.terminal.write("Enter password for {}: ".format(username))
        password = self.terminal.read_password()
        
        err = self.user_manager.add_user(username, password)
        if err:
            self.terminal.writeln(err)
        else:
            self.terminal.writeln("User '{}' created successfully".format(username))
    
    def cmd_rmuser(self, args):
        """Remove a user"""
        if self.current_user != 'root':
            self.terminal.writeln("rmuser: permission denied (root only)")
            return
        
        if not args:
            self.terminal.writeln("rmuser: missing username")
            return
        
        username = args[0]
        
        err = self.user_manager.remove_user(username)
        if err:
            self.terminal.writeln(err)
        else:
            self.terminal.writeln("User '{}' removed successfully".format(username))
            self.terminal.writeln("Note: Home directory /home/{} was not removed".format(username))
    
    def cmd_history(self, args):
        """Display command history"""
        history = self.session.get_history()
        for i, cmd in enumerate(history, 1):
            self.terminal.writeln("{:4d}  {}".format(i, cmd))
    
    def cmd_passwd(self, args):
        """Change user password"""
        if args and self.current_user != 'root':
            self.terminal.writeln("passwd: permission denied")
            return
        
        username = args[0] if args else self.current_user
        
        if not self.user_manager.user_exists(username):
            self.terminal.writeln("passwd: user '{}' does not exist".format(username))
            return
        
        # Root can change any password, others must verify current password
        if username != self.current_user and self.current_user == 'root':
            # Root changing another user's password
            self.terminal.write("Enter new password for {}: ".format(username))
            new_password = self.terminal.read_password()
            self.terminal.write("Retype new password: ")
            confirm = self.terminal.read_password()
            
            if new_password != confirm:
                self.terminal.writeln("Passwords do not match")
                return
            
            self.user_manager.users[username]['password'] = self.user_manager._hash_password(new_password)
            self.user_manager._save_passwd()
            self.terminal.writeln("Password updated successfully")
        else:
            # User changing own password
            self.terminal.write("Enter current password: ")
            current = self.terminal.read_password()
            
            if not self.user_manager.authenticate(username, current):
                self.terminal.writeln("passwd: authentication failed")
                return
            
            self.terminal.write("Enter new password: ")
            new_password = self.terminal.read_password()
            self.terminal.write("Retype new password: ")
            confirm = self.terminal.read_password()
            
            if new_password != confirm:
                self.terminal.writeln("Passwords do not match")
                return
            
            self.user_manager.users[username]['password'] = self.user_manager._hash_password(new_password)
            self.user_manager._save_passwd()
            self.terminal.writeln("Password updated successfully")
    
    def cmd_help(self, args):
        self.terminal.writeln("Available commands:")
        self.terminal.writeln("File: ls, cd, pwd, mkdir, rmdir, cat, echo, touch, rm, cp, mv, find")
        self.terminal.writeln("Text: grep, vi, ed")
        self.terminal.writeln("User: useradd, rmuser, passwd, who, history")
        self.terminal.writeln("System: ps, date, clear, uname, df, free, gpio")
        self.terminal.writeln("Other: help, exit")
    
    def cmd_gpio(self, args):
        """Show GPIO pin configuration"""
        self.terminal.writeln("GPIO Pin Configuration:")
        self.terminal.writeln("UART0: TX=GP0, RX=GP1 (default)")
        self.terminal.writeln("       RTS=GP2, CTS=GP3 (optional flow control)")
        self.terminal.writeln("UART1: TX=GP4, RX=GP5")
        self.terminal.writeln("       RTS=GP6, CTS=GP7 (optional flow control)")
        self.terminal.writeln("")
        self.terminal.writeln("Current session: UART{}".format(self.terminal.uart_id))
    
    def cmd_exit(self, args):
        self.terminal.writeln("Saving filesystem state...")
        err = self.fs.save(self.save_file)
        if err:
            self.terminal.writeln(err)
        else:
            self.terminal.writeln("Filesystem saved.")
        self.terminal.writeln("logout")
        self.running = False


class UARTMonitor:
    """Monitor multiple UART ports for incoming connections"""
    def __init__(self):
        self.uart_configs = [
            {'uart_id': 0, 'baudrate': 115200, 'tx_pin': 0, 'rx_pin': 1},
            {'uart_id': 1, 'baudrate': 115200, 'tx_pin': 4, 'rx_pin': 5},
        ]
        self.active_sessions = []
    
    def check_connection(self, config):
        """Check if there's activity on a UART port"""
        try:
            uart = UART(config['uart_id'], 
                       baudrate=config['baudrate'],
                       tx=Pin(config['tx_pin']),
                       rx=Pin(config['rx_pin']))
            uart.init(baudrate=config['baudrate'], bits=8, parity=None, stop=1)
            
            # Check for any data
            if uart.any():
                return True
            
            return False
        except:
            return False
    
    def start_session(self, config):
        """Start a new OS session on a UART"""
        try:
            _thread.start_new_thread(self.run_session, (config,))
            return True
        except:
            return False
    
    def run_session(self, config):
        """Run an OS instance on a specific UART"""
        try:
            os = XenixOS(
                uart_id=config['uart_id'],
                baudrate=config['baudrate'],
                tx_pin=config['tx_pin'],
                rx_pin=config['rx_pin']
            )
            os.boot()
        except Exception as e:
            print("Session error on UART{}: {}".format(config['uart_id'], str(e)))
    
    def monitor(self):
        """Monitor all UART ports for connections"""
        print("UART Monitor started")
        print("Monitoring UART0 (GP0/GP1) and UART1 (GP4/GP5)")
        
        checked = [False, False]
        
        while True:
            for i, config in enumerate(self.uart_configs):
                if not checked[i]:
                    if self.check_connection(config):
                        print("Connection detected on UART{}".format(config['uart_id']))
                        if self.start_session(config):
                            checked[i] = True
                            print("Session started on UART{}".format(config['uart_id']))
            
            time.sleep(0.5)


# Main entry point
def main():
    """
    Main entry point with multi-UART support
    
    Mode 1: Single UART session (default)
    Mode 2: Multi-UART monitoring (monitors both UART0 and UART1)
    """
    
    # Choose operating mode
    MODE = "single"  # Options: "single" or "monitor"
    
    if MODE == "single":
        # Single session mode - UART0 on GP0 (TX) and GP1 (RX) at 115200 baud
        os = XenixOS(uart_id=0, baudrate=115200, tx_pin=0, rx_pin=1)
        
        # Alternative: Use UART1 on GP4 (TX) and GP5 (RX)
        # os = XenixOS(uart_id=1, baudrate=115200, tx_pin=4, rx_pin=5)
        
        os.boot()
    
    elif MODE == "monitor":
        # Multi-UART monitoring mode
        # This will monitor both UART0 and UART1 for incoming connections
        # and spawn separate OS sessions for each
        monitor = UARTMonitor()
        monitor.monitor()


if __name__ == '__main__':
    main()
