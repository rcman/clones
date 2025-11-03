import sys
import time
import json
import os
from machine import UART, Pin
import select

class FileNode:
def **init**(self, name, is_directory=False, file_type=””):
self.name = name
self.is_directory = is_directory
self.file_type = file_type or (“DIR” if is_directory else “”)
self.content = “”
self.protection = “(RWED,RWED,RE,)”
self.owner = “[SYSTEM]”
self.size = 0
self.created = self._get_timestamp()
self.children = {} if is_directory else None
self.version = 1

```
def _get_timestamp(self):
    try:
        t = time.localtime()
        months = ["JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                 "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"]
        return "{}-{}-{} {:02d}:{:02d}:{:02d}".format(
            t[2], months[t[1]-1], t[0], t[3], t[4], t[5])
    except:
        return "1-JAN-1970 00:00:00"

def get_full_name(self):
    """Return VMS-style filename with version"""
    if self.is_directory:
        return self.name + ".DIR"
    elif self.file_type:
        return "{}.{};{}".format(self.name, self.file_type, self.version)
    else:
        return "{};{}".format(self.name, self.version)

def to_dict(self):
    d = {
        'name': self.name,
        'is_directory': self.is_directory,
        'file_type': self.file_type,
        'content': self.content,
        'protection': self.protection,
        'owner': self.owner,
        'size': self.size,
        'created': self.created,
        'version': self.version
    }
    if self.is_directory:
        d['children'] = {k: v.to_dict() for k, v in self.children.items()}
    return d

@staticmethod
def from_dict(d):
    node = FileNode(d['name'], d['is_directory'], d.get('file_type', ''))
    node.content = d.get('content', '')
    node.protection = d.get('protection', '(RWED,RWED,RE,)')
    node.owner = d.get('owner', '[SYSTEM]')
    node.size = d.get('size', 0)
    node.created = d.get('created', '1-JAN-1970 00:00:00')
    node.version = d.get('version', 1)
    if d['is_directory']:
        node.children = {k: FileNode.from_dict(v) for k, v in d.get('children', {}).items()}
    return node
```

class FileSystem:
def **init**(self):
self.root = FileNode(“PICO$DKA0”, True)
self.current = self.root
self.path_stack = [“PICO$DKA0:[000000]”]
self._create_initial_structure()

```
def _create_initial_structure(self):
    # Create VMS-style directory structure
    self.root.children['SYS0'] = FileNode('SYS0', True)
    self.root.children['SYS1'] = FileNode('SYS1', True)
    self.root.children['USER'] = FileNode('USER', True)
    
    sys0 = self.root.children['SYS0']
    sys0.children['SYSEXE'] = FileNode('SYSEXE', True)
    sys0.children['SYSMGR'] = FileNode('SYSMGR', True)
    sys0.children['SYSHLP'] = FileNode('SYSHLP', True)
    
    # Create LOGIN.COM
    sysmgr = sys0.children['SYSMGR']
    login_com = FileNode('LOGIN', False, 'COM')
    login_com.content = """$ ! LOGIN.COM - System Login Command Procedure
```

$ SET DEFAULT PICO$DKA0:[USER.SYSTEM]
$ SHOW TIME
$ WRITE SYS$OUTPUT “Welcome to VAX/VMS on Raspberry Pi Pico”
$ WRITE SYS$OUTPUT “”
“””
login_com.size = len(login_com.content)
sysmgr.children[‘LOGIN.COM’] = login_com

```
    # Create user directory
    user = self.root.children['USER']
    user.children['SYSTEM'] = FileNode('SYSTEM', True)
    
    # Create WELCOME.TXT
    system_dir = user.children['SYSTEM']
    welcome = FileNode('WELCOME', False, 'TXT')
    welcome.content = """Welcome to VAX/VMS Simulation
```

This is a VAX/VMS operating system simulator running on
the Raspberry Pi Pico with MicroPython.

VAX/VMS was Digital Equipment Corporation’s premier
operating system, known for its clustering, security,
and reliability features.

Type HELP for a list of available commands.
“””
welcome.size = len(welcome.content)
system_dir.children[‘WELCOME.TXT’] = welcome

```
def get_current_path(self):
    """Return VMS-style path"""
    return self.path_stack[-1]

def parse_vms_path(self, path):
    """Parse VMS-style path like PICO$DKA0:[SYS0.SYSMGR]"""
    if '[' in path and ']' in path:
        device = path.split('[')[0] if ':' in path else 'PICO$DKA0:'
        dirs = path[path.index('[')+1:path.index(']')].split('.')
        return device, dirs
    return None, [path]

def list_files(self):
    return list(self.current.children.values())

def change_directory(self, path):
    if path == '[-]':  # Parent directory
        if len(self.path_stack) > 1:
            parts = self.path_stack[-1].split('[')[1].split(']')[0].split('.')
            if len(parts) > 1:
                parts.pop()
                new_path = "PICO$DKA0:[{}]".format('.'.join(parts))
            else:
                new_path = "PICO$DKA0:[000000]"
            self.path_stack.pop()
            self.path_stack.append(new_path)
            self.current = self._navigate_to_vms_path(new_path)
        return None
    
    device, dirs = self.parse_vms_path(path)
    
    # Navigate to new directory
    node = self.root
    path_parts = []
    
    for dir_name in dirs:
        if dir_name == '000000':
            continue
        if dir_name in node.children and node.children[dir_name].is_directory:
            node = node.children[dir_name]
            path_parts.append(dir_name)
        else:
            return "%DIR-E-NODIR, directory not found"
    
    if path_parts:
        self.path_stack.append("PICO$DKA0:[{}]".format('.'.join(path_parts)))
    else:
        self.path_stack = ["PICO$DKA0:[000000]"]
    self.current = node
    return None

def _navigate_to_vms_path(self, path):
    device, dirs = self.parse_vms_path(path)
    node = self.root
    for dir_name in dirs:
        if dir_name == '000000':
            continue
        if dir_name in node.children:
            node = node.children[dir_name]
    return node

def create_directory(self, name):
    # Strip .DIR if present
    if name.endswith('.DIR'):
        name = name[:-4]
    
    if name not in self.current.children:
        self.current.children[name] = FileNode(name, True)
        return None
    return "%CREATE-E-EXISTS, {} already exists".format(name)

def delete_file(self, name):
    # Parse VMS filename
    parts = name.split(';')
    base_name = parts[0]
    
    if '.' in base_name:
        name_part, type_part = base_name.split('.', 1)
        full_key = name_part + '.' + type_part
    else:
        name_part = base_name
        full_key = None
    
    # Find matching file
    for key in list(self.current.children.keys()):
        if full_key and key == full_key:
            del self.current.children[key]
            return None
        elif not full_key and key.startswith(name_part):
            del self.current.children[key]
            return None
    
    return "%DELETE-E-FILNOT, file not found"

def create_file(self, name, file_type="TXT"):
    # Parse name
    if '.' in name:
        name_part, file_type = name.rsplit('.', 1)
    else:
        name_part = name
    
    key = name_part + '.' + file_type
    if key not in self.current.children:
        self.current.children[key] = FileNode(name_part, False, file_type)
    return None

def write_file(self, name, content):
    if '.' in name:
        name_part, file_type = name.rsplit('.', 1)
    else:
        name_part = name
        file_type = "TXT"
    
    key = name_part + '.' + file_type
    
    if key in self.current.children:
        file_node = self.current.children[key]
    else:
        file_node = FileNode(name_part, False, file_type)
        self.current.children[key] = file_node
    
    file_node.content = content
    file_node.size = len(content)
    file_node.created = file_node._get_timestamp()
    return None

def read_file(self, name):
    # Try to find file
    if '.' not in name:
        name = name + '.TXT'
    
    for key, node in self.current.children.items():
        if key.startswith(name.split(';')[0]):
            if not node.is_directory:
                return node.content, None
    
    return None, "%TYPE-E-FILNOT, file not found"

def save(self, filename):
    try:
        with open(filename, 'w') as f:
            json.dump(self.root.to_dict(), f)
        return None
    except Exception as e:
        return "%SAVE-E-ERROR, error saving filesystem: {}".format(str(e))

def load(self, filename):
    try:
        with open(filename, 'r') as f:
            data = json.load(f)
        self.root = FileNode.from_dict(data)
        self.current = self.root
        self.path_stack = ["PICO$DKA0:[000000]"]
        return None
    except:
        return None
```

class SerialTerminal:
def **init**(self, uart_id=0, baudrate=115200, tx_pin=0, rx_pin=1):
self.uart = UART(uart_id, baudrate=baudrate, tx=Pin(tx_pin), rx=Pin(rx_pin))
self.uart.init(baudrate=baudrate, bits=8, parity=None, stop=1)

```
    self.running = True
    self.poll = select.poll()
    self.poll.register(self.uart, select.POLLIN)
    
    # VMS uses different terminal sequences
    self.write('\x1b[2J')  # Clear screen
    self.write('\x1b[H')   # Home cursor
    
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
                    return line.upper() if echo else line
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
                    return None
            elif char == '\x15':
                for _ in range(len(line)):
                    self.write('\b \b')
                line = ""
            elif ord(char) >= 32 and ord(char) < 127:
                line += char
                if echo:
                    self.write(char.upper())

def clear_screen(self):
    self.write('\x1b[2J')
    self.write('\x1b[H')
```

class VAXVMS:
def **init**(self, uart_id=0, baudrate=9600, tx_pin=0, rx_pin=1):
self.fs = FileSystem()
self.current_user = “SYSTEM”
self.node_name = “PICO”
self.terminal = SerialTerminal(uart_id, baudrate, tx_pin, rx_pin)
self.running = True
self.save_file = “vms_state.json”
self.logged_in = False

```
    self.fs.load(self.save_file)

def boot(self):
    self.terminal.clear_screen()
    self.terminal.writeln("")
    self.terminal.writeln("VAX/VMS Version V5.5-2")
    self.terminal.writeln("")
    
    # Login sequence
    while not self.logged_in:
        self.terminal.write("Username: ")
        username = self.terminal.read_line()
        if username is None:
            return
        
        self.terminal.write("Password: ")
        password = self.terminal.read_line(echo=False)
        
        if username:
            self.current_user = username
            self.logged_in = True
            self.terminal.writeln("")
            self.terminal.writeln("        Welcome to VAX/VMS on PICO")
            self.terminal.writeln("")
            
            # Show last login
            t = time.localtime()
            self.terminal.writeln("    Last interactive login on {}".format(
                self._format_vms_time(t)))
            self.terminal.writeln("")
    
    # Execute LOGIN.COM
    login_content, _ = self.fs.read_file('PICO$DKA0:[SYS0.SYSMGR]LOGIN.COM')
    
    # Main command loop
    while self.running:
        prompt = "$ "
        self.terminal.write(prompt)
        cmd_line = self.terminal.read_line()
        
        if cmd_line is None:
            break
        
        if cmd_line:
            self.execute_command(cmd_line)

def _format_vms_time(self, t):
    months = ["JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
             "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"]
    days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]
    return "{}, {}-{}-{} {:02d}:{:02d}:{:02d}.00".format(
        days[t[6]], t[2], months[t[1]-1], t[0], t[3], t[4], t[5])

def execute_command(self, cmd_line):
    # VMS commands start with $ in scripts, strip it
    if cmd_line.startswith('$'):
        cmd_line = cmd_line[1:].strip()
    
    parts = cmd_line.split()
    if not parts:
        return
    
    cmd = parts[0]
    args = parts[1:]
    
    commands = {
        'DIR': self.cmd_dir,
        'DIRECTORY': self.cmd_dir,
        'SET': self.cmd_set,
        'SHOW': self.cmd_show,
        'TYPE': self.cmd_type,
        'CREATE': self.cmd_create,
        'DELETE': self.cmd_delete,
        'COPY': self.cmd_copy,
        'RENAME': self.cmd_rename,
        'PRINT': self.cmd_print,
        'HELP': self.cmd_help,
        'LOGOUT': self.cmd_logout,
        'EXIT': self.cmd_logout,
        'WRITE': self.cmd_write,
        'PURGE': self.cmd_purge,
        'SEARCH': self.cmd_search,
        'EDIT': self.cmd_edit,
    }
    
    if cmd in commands:
        commands[cmd](args)
    else:
        self.terminal.writeln("%DCL-W-IVVERB, unrecognized command verb - check validity and spelling")

def cmd_dir(self, args):
    self.terminal.writeln("")
    self.terminal.writeln("Directory {}".format(self.fs.get_current_path()))
    self.terminal.writeln("")
    
    files = self.fs.list_files()
    total_blocks = 0
    
    for f in files:
        blocks = (f.size // 512) + 1 if f.size > 0 else 1
        total_blocks += blocks
        self.terminal.writeln("{:20s} {:6d}  {}".format(
            f.get_full_name(), blocks, f.created))
    
    self.terminal.writeln("")
    self.terminal.writeln("Total of {} files, {} blocks.".format(len(files), total_blocks))
    self.terminal.writeln("")

def cmd_set(self, args):
    if not args:
        self.terminal.writeln("%SET-E-INSFPRM, insufficient parameters")
        return
    
    if args[0] == 'DEFAULT' and len(args) > 1:
        err = self.fs.change_directory(args[1])
        if err:
            self.terminal.writeln(err)
    else:
        self.terminal.writeln("%SET-W-NOTSET, item not set")

def cmd_show(self, args):
    if not args:
        args = ['DEFAULT']
    
    if args[0] == 'DEFAULT':
        self.terminal.writeln("  {}".format(self.fs.get_current_path()))
    elif args[0] == 'TIME':
        t = time.localtime()
        self.terminal.writeln("  {}".format(self._format_vms_time(t)))
    elif args[0] == 'SYSTEM':
        self.terminal.writeln("VAX/VMS V5.5-2 on node {} {}".format(
            self.node_name, self._format_vms_time(time.localtime())))
    elif args[0] == 'USERS':
        self.terminal.writeln("    VAX/VMS User Processes at {}-{:02d}:{:02d}".format(
            time.localtime()[2], time.localtime()[3], time.localtime()[4]))
        self.terminal.writeln("")
        self.terminal.writeln("    Total number of users = 1")
        self.terminal.writeln("")
        self.terminal.writeln(" Username     Process Name      PID    Terminal")
        self.terminal.writeln(" {}   {}    00000042  TTA0:".format(
            self.current_user.ljust(12), (self.current_user[:8]).ljust(16)))
    elif args[0] == 'MEMORY':
        import gc
        gc.collect()
        free = gc.mem_free()
        total = gc.mem_alloc() + free
        used = gc.mem_alloc()
        self.terminal.writeln("")
        self.terminal.writeln("System Memory Resources on {}-{:02d}:{:02d}".format(
            time.localtime()[2], time.localtime()[3], time.localtime()[4]))
        self.terminal.writeln("")
        self.terminal.writeln("Physical Memory Usage (bytes):       {}".format(total))
        self.terminal.writeln("                      In Use:         {}".format(used))
        self.terminal.writeln("                      Free:           {}".format(free))
    else:
        self.terminal.writeln("%SHOW-W-NOTSHOWN, item not shown")

def cmd_type(self, args):
    if not args:
        self.terminal.writeln("%TYPE-E-INSFPRM, insufficient parameters")
        return
    
    content, err = self.fs.read_file(args[0])
    if err:
        self.terminal.writeln(err)
    elif content:
        self.terminal.writeln(content)

def cmd_create(self, args):
    if not args:
        self.terminal.writeln("%CREATE-E-INSFPRM, insufficient parameters")
        return
    
    if '/DIR' in args or '/DIRECTORY' in args:
        name = args[0]
        err = self.fs.create_directory(name)
        if err:
            self.terminal.writeln(err)
        else:
            self.terminal.writeln("%CREATE-I-CREATED, {} created".format(name + '.DIR'))
    else:
        # Create file with editor
        filename = args[0]
        self.terminal.writeln("Input file text (press Ctrl+D when done)")
        lines = []
        while True:
            line = self.terminal.read_line()
            if line is None:
                break
            lines.append(line)
        
        content = '\n'.join(lines)
        self.fs.write_file(filename, content)
        self.terminal.writeln("%CREATE-I-CREATED, {} created".format(filename))

def cmd_delete(self, args):
    if not args:
        self.terminal.writeln("%DELETE-E-INSFPRM, insufficient parameters")
        return
    
    err = self.fs.delete_file(args[0])
    if err:
        self.terminal.writeln(err)
    else:
        self.terminal.writeln("%DELETE-I-DELETED, file deleted")

def cmd_copy(self, args):
    if len(args) < 2:
        self.terminal.writeln("%COPY-E-INSFPRM, insufficient parameters")
        return
    self.terminal.writeln("%COPY-S-COPIED, {} copied to {}".format(args[0], args[1]))

def cmd_rename(self, args):
    if len(args) < 2:
        self.terminal.writeln("%RENAME-E-INSFPRM, insufficient parameters")
        return
    self.terminal.writeln("%RENAME-S-RENAMED, {} renamed to {}".format(args[0], args[1]))

def cmd_print(self, args):
    if not args:
        self.terminal.writeln("%PRINT-E-INSFPRM, insufficient parameters")
        return
    self.terminal.writeln("%PRINT-S-QUEUED, {} queued to SYS$PRINT".format(args[0]))

def cmd_write(self, args):
    text = ' '.join(args)
    # Remove quotes if present
    if text.startswith('"') and text.endswith('"'):
        text = text[1:-1]
    self.terminal.writeln(text)

def cmd_purge(self, args):
    self.terminal.writeln("%PURGE-I-NOPURGE, no files purged")

def cmd_search(self, args):
    if len(args) < 2:
        self.terminal.writeln("%SEARCH-E-INSFPRM, insufficient parameters")
        return
    self.terminal.writeln("%SEARCH-I-NOMATCHES, no strings matched")

def cmd_edit(self, args):
    if not args:
        self.terminal.writeln("%EDIT-E-INSFPRM, insufficient parameters")
        return
    
    self.terminal.writeln("EDT - Line Mode Editor")
    self.terminal.writeln("Type HELP for help, EXIT to quit")
    self.terminal.writeln("")
    
    filename = args[0]
    content, _ = self.fs.read_file(filename)
    if content is None:
        content = ""
    
    lines = content.split('\n') if content else []
    
    editing = True
    while editing:
        self.terminal.write("*")
        cmd = self.terminal.read_line()
        
        if cmd == 'EXIT' or cmd == 'QUIT':
            # Save file
            self.fs.write_file(filename, '\n'.join(lines))
            editing = False
        elif cmd == 'LIST' or cmd == 'L':
            for i, line in enumerate(lines, 1):
                self.terminal.writeln("{:4d}  {}".format(i, line))
        elif cmd.startswith('I'):  # Insert
            self.terminal.writeln("Insert mode. Press Ctrl+D when done.")
            while True:
                line = self.terminal.read_line()
                if line is None:
                    break
                lines.append(line)
        elif cmd == 'HELP':
            self.terminal.writeln("Commands: LIST, INSERT, EXIT")
        else:
            self.terminal.writeln("?Unrecognized command")

def cmd_help(self, args):
    self.terminal.writeln("")
    self.terminal.writeln("Help available for:")
    self.terminal.writeln("")
    self.terminal.writeln("  DIR          DIRECTORY    SET          SHOW")
    self.terminal.writeln("  TYPE         CREATE       DELETE       COPY")
    self.terminal.writeln("  RENAME       PRINT        WRITE        PURGE")
    self.terminal.writeln("  SEARCH       EDIT         HELP         LOGOUT")
    self.terminal.writeln("")
    self.terminal.writeln("Additional information available:")
    self.terminal.writeln("")

def cmd_logout(self, args):
    self.terminal.writeln("")
    self.terminal.writeln("  SYSTEM       logged out at {}".format(
        self._format_vms_time(time.localtime())))
    self.terminal.writeln("")
    
    self.terminal.writeln("Saving filesystem state...")
    err = self.fs.save(self.save_file)
    if err:
        self.terminal.writeln(err)
    
    self.running = False
```

def main():
“””
Main entry point for VAX/VMS simulation
Default: UART0 on GP0 (TX) and GP1 (RX) at 9600 baud (typical for VAX terminals)
“””
vms = VAXVMS(uart_id=0, baudrate=9600, tx_pin=0, rx_pin=1)
vms.boot()

if **name** == ‘**main**’:
main()