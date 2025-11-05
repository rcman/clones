import sys
import time
import json
import os

class FileNode:
    def __init__(self, name, is_directory=False):
        self.name = name
        self.is_directory = is_directory
        self.content = ""
        self.permissions = "RWE,RE,RE,E" if is_directory else "RW,R,R,"
        self.owner = "SYSTEM"
        self.size = 0
        self.version = 1
        self.modified = self._get_timestamp()
        self.children = {} if is_directory else None
    
    def _get_timestamp(self):
        try:
            t = time.localtime()
            months = ["JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                     "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"]
            return "{}-{}-{} {:02d}:{:02d}:{:02d}.00".format(
                t[2], months[t[1]-1], t[0], t[3], t[4], t[5])
        except:
            return "1-JAN-1989 00:00:00.00"
    
    def to_dict(self):
        d = {
            'name': self.name,
            'is_directory': self.is_directory,
            'content': self.content,
            'permissions': self.permissions,
            'owner': self.owner,
            'size': self.size,
            'version': self.version,
            'modified': self.modified
        }
        if self.is_directory:
            d['children'] = {k: v.to_dict() for k, v in self.children.items()}
        return d
    
    @staticmethod
    def from_dict(d):
        node = FileNode(d['name'], d['is_directory'])
        node.content = d.get('content', '')
        node.permissions = d.get('permissions', 'RWE,RE,RE,E' if d['is_directory'] else 'RW,R,R,')
        node.owner = d.get('owner', 'SYSTEM')
        node.size = d.get('size', 0)
        node.version = d.get('version', 1)
        node.modified = d.get('modified', '1-JAN-1989 00:00:00.00')
        if d['is_directory']:
            node.children = {k: FileNode.from_dict(v) for k, v in d.get('children', {}).items()}
        return node


class FileSystem:
    def __init__(self):
        self.root = FileNode("SYS$SYSDEVICE:[000000]", True)
        self.current = self.root
        self.path_stack = ["SYS$SYSDEVICE:[000000]"]
        self._create_initial_structure()
    
    def _create_initial_structure(self):
        # Create VMS-style directory structure
        self.root.children['SYSMGR'] = FileNode('SYSMGR', True)
        self.root.children['SYSEXE'] = FileNode('SYSEXE', True)
        self.root.children['SYSMAINT'] = FileNode('SYSMAINT', True)
        self.root.children['USER'] = FileNode('USER', True)
        self.root.children['SCRATCH'] = FileNode('SCRATCH', True)
        
        user = self.root.children['USER']
        user.children['SYSTEM'] = FileNode('SYSTEM', True)
        
        # Create welcome message
        sysmgr = self.root.children['SYSMGR']
        welcome = FileNode('WELCOME.TXT', False)
        welcome.content = "Welcome to VAX/VMS Operating System\n\nVAX/VMS Version 5.5-2\n\nFor help, type HELP at the $ prompt.\n"
        welcome.size = len(welcome.content)
        sysmgr.children['WELCOME.TXT'] = welcome
    
    def get_current_path(self):
        if len(self.path_stack) == 1:
            return self.path_stack[0]
        path = "SYS$SYSDEVICE:[" + '.'.join(self.path_stack[1:]) + "]"
        return path
    
    def list_files(self):
        return list(self.current.children.values())
    
    def change_directory(self, path):
        path = path.upper()
        
        if path == '[-]':  # Parent directory in VMS
            if len(self.path_stack) > 1:
                self.path_stack.pop()
                self.current = self._navigate_to_path(self.path_stack)
        elif path.startswith('[') and path.endswith(']'):
            # VMS-style path
            inner = path[1:-1]
            if inner == '000000':
                self.path_stack = ["SYS$SYSDEVICE:[000000]"]
                self.current = self.root
            else:
                parts = inner.split('.')
                new_path = ["SYS$SYSDEVICE:[000000]"]
                node = self.root
                
                for part in parts:
                    if part in node.children and node.children[part].is_directory:
                        node = node.children[part]
                        new_path.append(part)
                    else:
                        return "%SET-E-DIRNOTFOUND, directory not found"
                
                self.path_stack = new_path
                self.current = node
        else:
            # Simple directory name
            if path in self.current.children and self.current.children[path].is_directory:
                self.path_stack.append(path)
                self.current = self.current.children[path]
            else:
                return "%SET-E-DIRNOTFOUND, directory not found"
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
        name = name.upper()
        if name.startswith('[') and name.endswith(']'):
            name = name[1:-1].split('.')[-1]
        
        if name not in self.current.children:
            self.current.children[name] = FileNode(name, True)
        else:
            return "%CREATE-E-EXISTS, directory already exists"
        return None
    
    def delete_directory(self, name):
        name = name.upper()
        if name in self.current.children:
            node = self.current.children[name]
            if node.is_directory and len(node.children) == 0:
                del self.current.children[name]
            else:
                return "%DELETE-E-DIRNOTEMPTY, directory not empty"
        else:
            return "%DELETE-E-DIRNOTFOUND, directory not found"
        return None
    
    def create_file(self, name):
        name = name.upper()
        if '.' not in name:
            name = name + '.DAT'
        
        if name not in self.current.children:
            file_node = FileNode(name, False)
            self.current.children[name] = file_node
        return None
    
    def write_file(self, name, content):
        name = name.upper()
        if '.' not in name:
            name = name + '.DAT'
        
        if name in self.current.children:
            file_node = self.current.children[name]
            if not file_node.is_directory:
                file_node.version += 1
        else:
            file_node = FileNode(name, False)
            self.current.children[name] = file_node
        
        if not file_node.is_directory:
            file_node.content = content
            file_node.size = len(content)
            file_node.modified = file_node._get_timestamp()
        return None
    
    def read_file(self, name):
        name = name.upper()
        if '.' not in name:
            name = name + '.DAT'
        
        if name in self.current.children:
            node = self.current.children[name]
            if not node.is_directory:
                return node.content, None
            return None, "%TYPE-E-ISDIR, file is a directory"
        return None, "%TYPE-E-FILENOTFOUND, file not found"
    
    def delete_file(self, name):
        name = name.upper()
        if '.' not in name:
            name = name + '.DAT'
        
        if name in self.current.children:
            del self.current.children[name]
        else:
            return "%DELETE-E-FILENOTFOUND, file not found"
        return None
    
    def copy_file(self, src, dest):
        src = src.upper()
        dest = dest.upper()
        
        if '.' not in src:
            src = src + '.DAT'
        if '.' not in dest:
            dest = dest + '.DAT'
        
        if src in self.current.children and not self.current.children[src].is_directory:
            source = self.current.children[src]
            copy = FileNode(dest, False)
            copy.content = source.content
            copy.size = source.size
            self.current.children[dest] = copy
        else:
            return "%COPY-E-FILENOTFOUND, file not found"
        return None
    
    def rename_file(self, src, dest):
        src = src.upper()
        dest = dest.upper()
        
        if '.' not in src:
            src = src + '.DAT'
        if '.' not in dest:
            dest = dest + '.DAT'
        
        if src in self.current.children:
            source = self.current.children[src]
            del self.current.children[src]
            source.name = dest
            self.current.children[dest] = source
        else:
            return "%RENAME-E-FILENOTFOUND, file not found"
        return None
    
    def search_files(self, pattern):
        results = []
        pattern = pattern.upper()
        for name in self.current.children.keys():
            if pattern == '*' or pattern.replace('*', '') in name:
                results.append(name)
        return results
    
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
            self.path_stack = ["SYS$SYSDEVICE:[000000]"]
            return None
        except:
            return None


class SerialTerminal:
    def __init__(self):
        self.uart = sys.stdin
        self.running = True
    
    def write(self, text):
        text = text.replace('\n', '\r\n')
        sys.stdout.write(text)
    
    def writeln(self, text=""):
        self.write(text + '\n')
    
    def read_line(self):
        line = ""
        while True:
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
                    self.writeln("LOGOUT")
                    return "LOGOUT"
            elif char == '\x15':
                for _ in range(len(line)):
                    self.write('\b \b')
                line = ""
            elif ord(char) >= 32 and ord(char) < 127:
                line += char
                self.write(char)
    
    def clear_screen(self):
        self.write('\x1b[2J')
        self.write('\x1b[H')


class VAXVMS:
    def __init__(self):
        self.fs = FileSystem()
        self.current_user = "SYSTEM"
        self.node = "PICO"
        self.terminal = SerialTerminal()
        self.running = True
        self.save_file = "vaxvms_state.json"
        
        self.fs.load(self.save_file)
    
    def boot(self):
        self.terminal.clear_screen()
        self.terminal.writeln("")
        self.terminal.writeln("VAX/VMS Version 5.5-2")
        self.terminal.writeln("")
        self.terminal.writeln("    Welcome to VAX/VMS (TM) on Raspberry Pi Pico")
        self.terminal.writeln("")
        self.terminal.writeln("    Digital Equipment Corporation")
        self.terminal.writeln("")
        
        # Display welcome message
        content, err = self.fs.read_file('[SYSMGR]WELCOME.TXT')
        if content:
            self.terminal.writeln(content)
        
        self.terminal.writeln("")
        self.terminal.writeln("Username: " + self.current_user)
        self.terminal.writeln("")
        self.terminal.writeln("%LOGIN-I-LOGGEDIN, logged in as " + self.current_user)
        self.terminal.writeln("")
        
        while self.running:
            prompt = "$ "
            self.terminal.write(prompt)
            cmd_line = self.terminal.read_line()
            
            if cmd_line:
                self.execute_command(cmd_line)
    
    def execute_command(self, cmd_line):
        # VMS commands are case-insensitive
        parts = cmd_line.strip().split()
        if not parts:
            return
        
        cmd = parts[0].upper()
        args = parts[1:]
        
        commands = {
            'DIR': self.cmd_dir,
            'DIRECTORY': self.cmd_dir,
            'SET': self.cmd_set,
            'SHOW': self.cmd_show,
            'CREATE': self.cmd_create,
            'DELETE': self.cmd_delete,
            'TYPE': self.cmd_type,
            'PRINT': self.cmd_print,
            'COPY': self.cmd_copy,
            'RENAME': self.cmd_rename,
            'SEARCH': self.cmd_search,
            'HELP': self.cmd_help,
            'LOGOUT': self.cmd_logout,
            'EXIT': self.cmd_logout,
            'EDIT': self.cmd_edit,
            'RUN': self.cmd_run,
            'SUBMIT': self.cmd_submit,
            'MAIL': self.cmd_mail,
            'PHONE': self.cmd_phone,
            'PURGE': self.cmd_purge,
        }
        
        if cmd in commands:
            commands[cmd](args)
        else:
            self.terminal.writeln("%DCL-W-IVVERB, unrecognized command verb - check validity and spelling")
    
    def cmd_dir(self, args):
        files = self.fs.list_files()
        
        if not files:
            self.terminal.writeln("")
            self.terminal.writeln("No files found.")
            return
        
        self.terminal.writeln("")
        self.terminal.writeln("Directory " + self.fs.get_current_path())
        self.terminal.writeln("")
        
        for f in files:
            if f.is_directory:
                self.terminal.writeln("{}.DIR;1".format(f.name))
            else:
                self.terminal.writeln("{};{}    {:8d}  {}".format(
                    f.name, f.version, f.size, f.modified))
        
        self.terminal.writeln("")
        self.terminal.writeln("Total of {} files".format(len(files)))
    
    def cmd_set(self, args):
        if not args:
            self.terminal.writeln("%SET-E-TOOFEW, too few arguments")
            return
        
        subcmd = args[0].upper()
        
        if subcmd == 'DEFAULT':
            if len(args) > 1:
                err = self.fs.change_directory(args[1])
                if err:
                    self.terminal.writeln(err)
            else:
                self.terminal.writeln(self.fs.get_current_path())
        elif subcmd == 'TERMINAL':
            self.terminal.writeln("%SET-I-TERMINAL, terminal characteristics set")
        else:
            self.terminal.writeln("%SET-W-NOTSET, item not set")
    
    def cmd_show(self, args):
        if not args:
            self.terminal.writeln("%SHOW-E-TOOFEW, too few arguments")
            return
        
        subcmd = args[0].upper()
        
        if subcmd == 'DEFAULT':
            self.terminal.writeln("  " + self.fs.get_current_path())
        elif subcmd == 'TIME':
            try:
                t = time.localtime()
                months = ["JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"]
                self.terminal.writeln("  {}-{}-{} {:02d}:{:02d}:{:02d}".format(
                    t[2], months[t[1]-1], t[0], t[3], t[4], t[5]))
            except:
                self.terminal.writeln("  1-JAN-1989 00:00:00")
        elif subcmd == 'SYSTEM':
            self.terminal.writeln("  VAX/VMS V5.5-2 on node " + self.node)
        elif subcmd == 'USERS':
            self.terminal.writeln("    VAX/VMS Interactive Users")
            self.terminal.writeln("")
            self.terminal.writeln("  Username  Process Name      PID     Terminal")
            self.terminal.writeln("  " + self.current_user.ljust(10) + "SYSTEM          00000042  TXA0:")
        elif subcmd == 'MEMORY':
            try:
                import gc
                gc.collect()
                free = gc.mem_free()
                total = gc.mem_alloc() + free
                used = gc.mem_alloc()
                self.terminal.writeln("  Total Physical Memory:    {:8d} bytes".format(total))
                self.terminal.writeln("  Free Memory:              {:8d} bytes".format(free))
                self.terminal.writeln("  Used Memory:              {:8d} bytes".format(used))
            except:
                self.terminal.writeln("  Memory information unavailable")
        elif subcmd == 'DEVICES':
            self.terminal.writeln("  Device  Type           Status")
            self.terminal.writeln("  TXA0:   Terminal       Online")
            self.terminal.writeln("  SYS$:   System Disk    Mounted")
        else:
            self.terminal.writeln("%SHOW-W-NOTFOUND, item not found")
    
    def cmd_create(self, args):
        if not args:
            self.terminal.writeln("%CREATE-E-TOOFEW, too few arguments")
            return
        
        if args[0].upper().startswith('/DIR'):
            if len(args) < 2:
                self.terminal.writeln("%CREATE-E-NOSPEC, no directory specified")
                return
            err = self.fs.create_directory(args[1])
            if err:
                self.terminal.writeln(err)
            else:
                self.terminal.writeln("%CREATE-I-CREATED, " + args[1].upper() + ".DIR;1 created")
        else:
            self.fs.create_file(args[0])
            self.terminal.writeln("%CREATE-I-CREATED, " + args[0].upper() + " created")
    
    def cmd_delete(self, args):
        if not args:
            self.terminal.writeln("%DELETE-E-TOOFEW, too few arguments")
            return
        
        name = args[0]
        if name.endswith('.DIR'):
            name = name[:-4]
            err = self.fs.delete_directory(name)
        else:
            err = self.fs.delete_file(name)
        
        if err:
            self.terminal.writeln(err)
    
    def cmd_type(self, args):
        if not args:
            self.terminal.writeln("%TYPE-E-TOOFEW, too few arguments")
            return
        
        content, err = self.fs.read_file(args[0])
        if err:
            self.terminal.writeln(err)
        elif content:
            self.terminal.writeln("")
            self.terminal.writeln(content)
    
    def cmd_print(self, args):
        if not args:
            self.terminal.writeln("%PRINT-E-TOOFEW, too few arguments")
            return
        self.terminal.writeln("%PRINT-I-QUEUED, file queued to SYS$PRINT")
    
    def cmd_copy(self, args):
        if len(args) < 2:
            self.terminal.writeln("%COPY-E-TOOFEW, too few arguments")
            return
        err = self.fs.copy_file(args[0], args[1])
        if err:
            self.terminal.writeln(err)
    
    def cmd_rename(self, args):
        if len(args) < 2:
            self.terminal.writeln("%RENAME-E-TOOFEW, too few arguments")
            return
        err = self.fs.rename_file(args[0], args[1])
        if err:
            self.terminal.writeln(err)
    
    def cmd_search(self, args):
        if len(args) < 2:
            self.terminal.writeln("%SEARCH-E-TOOFEW, too few arguments")
            return
        
        pattern = args[1]
        filename = args[0]
        content, err = self.fs.read_file(filename)
        if err:
            self.terminal.writeln(err)
        elif content:
            for line in content.split('\n'):
                if pattern in line:
                    self.terminal.writeln(line)
    
    def cmd_edit(self, args):
        if not args:
            self.terminal.writeln("%EDIT-E-TOOFEW, too few arguments")
            return
        
        filename = args[0]
        content, _ = self.fs.read_file(filename)
        if content is None:
            content = ""
        
        self.terminal.writeln("")
        self.terminal.writeln("EDT Editor")
        self.terminal.writeln("Commands: INSERT (add text), LIST (show), EXIT (save & quit), QUIT (quit)")
        self.terminal.writeln("")
        
        buffer = content
        editing = True
        
        while editing:
            self.terminal.write("*")
            cmd = self.terminal.read_line().strip().upper()
            
            if cmd == 'INSERT' or cmd == 'I':
                self.terminal.writeln("Enter text (type . on a line to finish):")
                while True:
                    line = self.terminal.read_line()
                    if line == '.':
                        break
                    buffer += line + '\n'
            elif cmd == 'LIST' or cmd == 'L':
                self.terminal.writeln("")
                self.terminal.writeln(buffer)
            elif cmd == 'EXIT' or cmd == 'E':
                self.fs.write_file(filename, buffer)
                self.terminal.writeln("%EDIT-I-SAVED, file saved")
                editing = False
            elif cmd == 'QUIT' or cmd == 'Q':
                editing = False
            else:
                self.terminal.writeln("?")
    
    def cmd_run(self, args):
        if not args:
            self.terminal.writeln("%RUN-E-TOOFEW, too few arguments")
            return
        self.terminal.writeln("%RUN-I-PROC, process created")
    
    def cmd_submit(self, args):
        if not args:
            self.terminal.writeln("%SUBMIT-E-TOOFEW, too few arguments")
            return
        self.terminal.writeln("%SUBMIT-I-QUEUED, job queued to SYS$BATCH")
    
    def cmd_mail(self, args):
        self.terminal.writeln("VAX/VMS MAIL Utility V5.5")
        self.terminal.writeln("MAIL> Type EXIT to return to DCL")
    
    def cmd_phone(self, args):
        self.terminal.writeln("VAX/VMS Phone Facility")
        self.terminal.writeln("%PHONE-I-NOUSER, no other users available")
    
    def cmd_purge(self, args):
        self.terminal.writeln("%PURGE-I-FILPURG, old versions purged")
    
    def cmd_help(self, args):
        self.terminal.writeln("")
        self.terminal.writeln("Available commands:")
        self.terminal.writeln("")
        self.terminal.writeln("  File Operations:")
        self.terminal.writeln("    DIRECTORY (DIR)    - List files")
        self.terminal.writeln("    TYPE              - Display file contents")
        self.terminal.writeln("    COPY              - Copy files")
        self.terminal.writeln("    RENAME            - Rename files")
        self.terminal.writeln("    DELETE            - Delete files")
        self.terminal.writeln("    CREATE/DIR        - Create directory")
        self.terminal.writeln("    PURGE             - Remove old versions")
        self.terminal.writeln("")
        self.terminal.writeln("  System Commands:")
        self.terminal.writeln("    SET DEFAULT       - Change directory")
        self.terminal.writeln("    SHOW DEFAULT      - Display current directory")
        self.terminal.writeln("    SHOW TIME         - Display date/time")
        self.terminal.writeln("    SHOW SYSTEM       - System information")
        self.terminal.writeln("    SHOW USERS        - Display logged in users")
        self.terminal.writeln("")
        self.terminal.writeln("  Other:")
        self.terminal.writeln("    EDIT              - Text editor")
        self.terminal.writeln("    SEARCH            - Search in files")
        self.terminal.writeln("    LOGOUT            - Exit system")
        self.terminal.writeln("")
    
    def cmd_logout(self, args):
        self.terminal.writeln("")
        self.terminal.writeln("%SYSTEM-I-SAVING, saving filesystem state...")
        err = self.fs.save(self.save_file)
        if err:
            self.terminal.writeln(err)
        else:
            self.terminal.writeln("%SYSTEM-I-SAVED, filesystem saved")
        self.terminal.writeln("")
        self.terminal.writeln("  SYSTEM  logged out at " + time.strftime("%d-%b-%Y %H:%M:%S"))
        self.terminal.writeln("")
        self.running = False


def main():
    vms = VAXVMS()
    vms.boot()


if __name__ == '__main__':
    main()
