import sys
import time
import json

# MicroPython compatibility
try:
    import gc
    MICROPYTHON = True
except ImportError:
    MICROPYTHON = False


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
            return "Jan  1 00:00"
    
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
        node.permissions = d.get('permissions', 
            'drwxr-xr-x' if d['is_directory'] else '-rw-r--r--')
        node.owner = d.get('owner', 'root')
        node.group = d.get('group', 'root')
        node.size = d.get('size', 0)
        node.modified = d.get('modified', 'Jan  1 00:00')
        if d['is_directory']:
            node.children = {
                k: FileNode.from_dict(v) 
                for k, v in d.get('children', {}).items()
            }
        return node


class FileSystem:
    def __init__(self):
        self.root = FileNode("/", True)
        self.current = self.root
        self.path_stack = ["/"]
        self._create_initial_structure()
    
    def _create_initial_structure(self):
        dirs = ['bin', 'etc', 'usr', 'tmp', 'home', 'mnt', 'dev', 'var']
        for d in dirs:
            self.root.children[d] = FileNode(d, True)

        # Create usr subdirectories
        usr = self.root.children['usr']
        for d in ['bin', 'lib', 'spool']:
            usr.children[d] = FileNode(d, True)

        # Create home/root
        home = self.root.children['home']
        home.children['root'] = FileNode('root', True)

        # Create var/log
        var = self.root.children['var']
        var.children['log'] = FileNode('log', True)

        # Create MOTD
        etc = self.root.children['etc']
        motd = FileNode('motd', False)
        motd.content = """
                     RESTRICTED RIGHTS LEGEND

Use, duplication, or disclosure is subject to restrictions as set forth
in subparagraph (c)(1)(ii) of the Rights in Technical Data and Computer
Software clause at DFARS 52.227-7013.

Microsoft Corporation
One Microsoft Way
Redmond, Washington  98052-6399
"""
        motd.size = len(motd.content)
        etc.children['motd'] = motd

        # Create passwd file
        passwd = FileNode('passwd', False)
        passwd.content = "root:x:0:0:root:/home/root:/bin/sh\n"
        passwd.size = len(passwd.content)
        etc.children['passwd'] = passwd
    
    def get_current_path(self):
        if len(self.path_stack) == 1:
            return "/"
        return '/'.join(self.path_stack).replace('//', '/')
    
    def list_files(self):
        return list(self.current.children.values())
    
    def _resolve_path(self, path):
        """Resolve a path to a node, handling . and .."""
        if path.startswith('/'):
            parts = [p for p in path.split('/') if p]
            node = self.root
            traversed = ['/']
        else:
            parts = [p for p in path.split('/') if p]
            node = self.current
            traversed = list(self.path_stack)
        
        for part in parts:
            if part == '.':
                continue
            elif part == '..':
                if len(traversed) > 1:
                    traversed.pop()
                    node = self._navigate_to_path(traversed)
            elif node.is_directory and part in node.children:
                node = node.children[part]
                traversed.append(part)
            else:
                return None, None
        
        return node, traversed
    
    def change_directory(self, path):
        if path == '~' or path == '':
            path = '/home/root'
        
        node, new_path = self._resolve_path(path)
        
        if node is None:
            return "cd: {}: No such file or directory".format(path)
        if not node.is_directory:
            return "cd: {}: Not a directory".format(path)
        
        self.current = node
        self.path_stack = new_path
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
        if '/' in name:
            return "mkdir: cannot create directory '{}': Invalid name".format(name)
        if name in self.current.children:
            return "mkdir: cannot create directory '{}': File exists".format(name)
        self.current.children[name] = FileNode(name, True)
        return None
    
    def remove_directory(self, name):
        if name not in self.current.children:
            return "rmdir: failed to remove '{}': No such file or directory".format(name)
        node = self.current.children[name]
        if not node.is_directory:
            return "rmdir: failed to remove '{}': Not a directory".format(name)
        if node.children:
            return "rmdir: failed to remove '{}': Directory not empty".format(name)
        del self.current.children[name]
        return None
    
    def create_file(self, name):
        if name not in self.current.children:
            self.current.children[name] = FileNode(name, False)
        else:
            # Update timestamp
            self.current.children[name].modified = FileNode(name)._get_timestamp()
        return None
    
    def write_file(self, name, content, append=False):
        if name in self.current.children:
            file_node = self.current.children[name]
            if file_node.is_directory:
                return "cannot write to '{}': Is a directory".format(name)
        else:
            file_node = FileNode(name, False)
            self.current.children[name] = file_node
        
        if append:
            file_node.content += content
        else:
            file_node.content = content
        file_node.size = len(file_node.content)
        file_node.modified = file_node._get_timestamp()
        return None
    
    def read_file(self, name):
        node, _ = self._resolve_path(name)
        
        if node is None:
            return None, "cat: {}: No such file or directory".format(name)
        if node.is_directory:
            return None, "cat: {}: Is a directory".format(name)
        
        return node.content, None
    
    def remove_file(self, name, force=False, recursive=False):
        if name not in self.current.children:
            if force:
                return None
            return "rm: cannot remove '{}': No such file or directory".format(name)
        
        node = self.current.children[name]
        if node.is_directory:
            if not recursive:
                return "rm: cannot remove '{}': Is a directory".format(name)
            # Recursive delete
        
        del self.current.children[name]
        return None
    
    def copy_file(self, src, dest):
        src_node, _ = self._resolve_path(src)
        
        if src_node is None:
            return "cp: cannot stat '{}': No such file or directory".format(src)
        if src_node.is_directory:
            return "cp: -r not specified; omitting directory '{}'".format(src)
        
        copy = FileNode(dest, False)
        copy.content = src_node.content
        copy.size = src_node.size
        self.current.children[dest] = copy
        return None
    
    def move_file(self, src, dest):
        if src not in self.current.children:
            return "mv: cannot stat '{}': No such file or directory".format(src)
        
        source = self.current.children[src]
        del self.current.children[src]
        source.name = dest
        self.current.children[dest] = source
        return None
    
    def chmod(self, mode, name):
        if name not in self.current.children:
            return "chmod: cannot access '{}': No such file or directory".format(name)
        
        node = self.current.children[name]
        # Simple chmod - just store the mode string
        try:
            mode_int = int(mode, 8)
            perms = 'd' if node.is_directory else '-'
            for i in range(3):
                bits = (mode_int >> (6 - i*3)) & 7
                perms += 'r' if bits & 4 else '-'
                perms += 'w' if bits & 2 else '-'
                perms += 'x' if bits & 1 else '-'
            node.permissions = perms
        except ValueError:
            return "chmod: invalid mode: '{}'".format(mode)
        return None
    
    def find_recursive(self, node, path, pattern, results):
        for name, child in node.children.items():
            child_path = path + '/' + name if path != '/' else '/' + name
            if pattern == '*' or pattern in name:
                results.append(child_path)
            if child.is_directory:
                self.find_recursive(child, child_path, pattern, results)
    
    def find(self, pattern, start_path='.'):
        results = []
        if start_path == '.':
            start_node = self.current
            start_path = self.get_current_path()
        else:
            start_node, _ = self._resolve_path(start_path)
            if start_node is None:
                return []
        
        if start_node.is_directory:
            self.find_recursive(start_node, start_path, pattern, results)
        
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
    def __init__(self):
        self.running = True
        self.history = []
        self.history_index = 0
    
    def write(self, text):
        text = text.replace('\n', '\r\n')
        sys.stdout.write(text)
        if hasattr(sys.stdout, 'flush'):
            sys.stdout.flush()
    
    def writeln(self, text=""):
        self.write(text + '\n')
    
    def read_line(self):
        line = ""
        self.history_index = len(self.history)
        
        while True:
            try:
                char = sys.stdin.read(1)
            except:
                return "exit"
            
            if not char:
                continue
            
            if char in ('\r', '\n'):
                self.writeln()
                if line:
                    self.history.append(line)
                    # Keep history bounded
                    if len(self.history) > 50:
                        self.history.pop(0)
                return line
            elif char in ('\x7f', '\x08'):  # Backspace/DEL
                if line:
                    line = line[:-1]
                    self.write('\b \b')
            elif char == '\x03':  # Ctrl+C
                self.writeln('^C')
                return ""
            elif char == '\x04':  # Ctrl+D
                if not line:
                    self.writeln("logout")
                    return "exit"
            elif char == '\x15':  # Ctrl+U
                while line:
                    self.write('\b \b')
                    line = line[:-1]
            elif char == '\x17':  # Ctrl+W (delete word)
                while line and line[-1] == ' ':
                    self.write('\b \b')
                    line = line[:-1]
                while line and line[-1] != ' ':
                    self.write('\b \b')
                    line = line[:-1]
            elif char == '\x1b':  # Escape sequence
                # Try to read arrow keys
                try:
                    seq1 = sys.stdin.read(1)
                    if seq1 == '[':
                        seq2 = sys.stdin.read(1)
                        if seq2 == 'A':  # Up arrow
                            if self.history and self.history_index > 0:
                                # Clear current line
                                while line:
                                    self.write('\b \b')
                                    line = line[:-1]
                                self.history_index -= 1
                                line = self.history[self.history_index]
                                self.write(line)
                        elif seq2 == 'B':  # Down arrow
                            while line:
                                self.write('\b \b')
                                line = line[:-1]
                            if self.history_index < len(self.history) - 1:
                                self.history_index += 1
                                line = self.history[self.history_index]
                                self.write(line)
                            else:
                                self.history_index = len(self.history)
                except:
                    pass
            elif 32 <= ord(char) < 127:
                line += char
                self.write(char)
        
        return line
    
    def clear_screen(self):
        self.write('\x1b[2J\x1b[H')


class XenixOS:
    def __init__(self):
        self.fs = FileSystem()
        self.current_user = "root"
        self.hostname = "pico"
        self.terminal = SerialTerminal()
        self.running = True
        self.save_file = "xenix_state.json"
        self.env = {
            'PATH': '/bin:/usr/bin',
            'HOME': '/home/root',
            'SHELL': '/bin/sh',
            'USER': 'root',
            'TERM': 'vt100'
        }
        
        self.fs.load(self.save_file)
    
    def boot(self):
        self.terminal.clear_screen()
        self.terminal.writeln()
        self.terminal.writeln("The XENIX System")
        self.terminal.writeln()
        self.terminal.writeln()
        self.terminal.writeln("Copyright (c) 1984, 1985, 1986, 1987, 1988")
        self.terminal.writeln("The Santa Cruz Operation, Inc.")
        self.terminal.writeln("Microsoft Corporation")
        self.terminal.writeln("AT&T")
        self.terminal.writeln("All Rights Reserved")
        self.terminal.writeln()
        self.terminal.writeln()

        self.terminal.write("login: ")
        self.terminal.writeln(self.current_user)
        self.terminal.writeln()

        content, _ = self.fs.read_file('/etc/motd')
        if content:
            self.terminal.write(content)

        self.terminal.writeln()
        self._show_login_time()
        self.terminal.writeln()

        while self.running:
            prompt = "# " if self.current_user == "root" else "$ "
            self.terminal.write(prompt)
            cmd_line = self.terminal.read_line()

            if cmd_line:
                self.execute_command(cmd_line)
    
    def _show_login_time(self):
        try:
            t = time.localtime()
            days = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
            months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
            self.terminal.writeln("Last login: {} {} {:2d} {:02d}:{:02d} on tty01".format(
                days[t[6]], months[t[1]-1], t[2], t[3], t[4]))
        except:
            self.terminal.writeln("Last login: Mon Jan  1 00:00 on tty01")
    
    def execute_command(self, cmd_line):
        # Handle environment variable expansion
        for key, val in self.env.items():
            cmd_line = cmd_line.replace('$' + key, val)
        
        # Handle pipes (simple implementation)
        if '|' in cmd_line:
            self.terminal.writeln("sh: pipes not yet implemented")
            return
        
        # Parse command and redirections
        parts = cmd_line.strip().split()
        if not parts:
            return
        
        cmd = parts[0]
        args = parts[1:]
        
        # Handle append redirect
        append_file = None
        if '>>' in args:
            idx = args.index('>>')
            if idx + 1 < len(args):
                append_file = args[idx + 1]
                args = args[:idx]
        
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
            'head': self.cmd_head,
            'tail': self.cmd_tail,
            'wc': self.cmd_wc,
            'ps': self.cmd_ps,
            'who': self.cmd_who,
            'whoami': self.cmd_whoami,
            'date': self.cmd_date,
            'clear': self.cmd_clear,
            'help': self.cmd_help,
            'exit': self.cmd_exit,
            'logout': self.cmd_exit,
            'uname': self.cmd_uname,
            'df': self.cmd_df,
            'free': self.cmd_free,
            'vi': self.cmd_vi,
            'ed': self.cmd_ed,
            'chmod': self.cmd_chmod,
            'env': self.cmd_env,
            'export': self.cmd_export,
            'history': self.cmd_history,
            'banner': self.cmd_banner,
            'write': self.cmd_write,
            'wall': self.cmd_wall,
            'mesg': self.cmd_mesg,
            'true': lambda a: None,
            'false': lambda a: self.terminal.writeln(""),
        }
        
        if cmd in commands:
            commands[cmd](args)
        else:
            self.terminal.writeln("{}: not found".format(cmd))
    
    def cmd_ls(self, args):
        files = self.fs.list_files()
        long_format = '-l' in args
        all_files = '-a' in args
        
        # Filter out flags from args
        path_args = [a for a in args if not a.startswith('-')]
        
        if long_format:
            self.terminal.writeln("total {}".format(len(files)))
            for f in sorted(files, key=lambda x: x.name):
                self.terminal.writeln("{} {:2d} {:8s} {:8s} {:8d} {} {}".format(
                    f.permissions, 1, f.owner, f.group, f.size, f.modified, f.name))
        else:
            names = []
            if all_files:
                names = ['.', '..']
            names.extend(sorted(f.name for f in files))
            
            if names:
                # Simple column output
                self.terminal.writeln("  ".join(names))
    
    def cmd_cd(self, args):
        path = args[0] if args else '~'
        err = self.fs.change_directory(path)
        if err:
            self.terminal.writeln(err)
    
    def cmd_pwd(self, args):
        self.terminal.writeln(self.fs.get_current_path())
    
    def cmd_mkdir(self, args):
        if not args:
            self.terminal.writeln("usage: mkdir directory ...")
            return
        for name in args:
            if name.startswith('-'):
                continue
            err = self.fs.create_directory(name)
            if err:
                self.terminal.writeln(err)
    
    def cmd_rmdir(self, args):
        if not args:
            self.terminal.writeln("usage: rmdir directory ...")
            return
        for name in args:
            err = self.fs.remove_directory(name)
            if err:
                self.terminal.writeln(err)
    
    def cmd_cat(self, args):
        if not args:
            # Read from stdin until Ctrl+D
            self.terminal.writeln("(reading from stdin, Ctrl+D to end)")
            while True:
                line = self.terminal.read_line()
                if line == "exit":
                    break
            return
        
        for filename in args:
            content, err = self.fs.read_file(filename)
            if err:
                self.terminal.writeln(err)
            elif content:
                # Don't add extra newline if content ends with one
                if content.endswith('\n'):
                    self.terminal.write(content)
                else:
                    self.terminal.writeln(content)
    
    def cmd_echo(self, args):
        if not args:
            self.terminal.writeln()
            return
        
        no_newline = False
        if args[0] == '-n':
            no_newline = True
            args = args[1:]
        
        # Check for redirect
        if '>' in args:
            idx = args.index('>')
            text = ' '.join(args[:idx])
            if idx + 1 < len(args):
                filename = args[idx + 1]
                self.fs.write_file(filename, text + '\n')
            return
        
        text = ' '.join(args)
        if no_newline:
            self.terminal.write(text)
        else:
            self.terminal.writeln(text)
    
    def cmd_touch(self, args):
        if not args:
            self.terminal.writeln("usage: touch file ...")
            return
        for name in args:
            self.fs.create_file(name)
    
    def cmd_rm(self, args):
        if not args:
            self.terminal.writeln("usage: rm [-rf] file ...")
            return
        
        force = '-f' in args
        recursive = '-r' in args or '-rf' in args or '-fr' in args
        
        for name in args:
            if name.startswith('-'):
                continue
            err = self.fs.remove_file(name, force, recursive)
            if err:
                self.terminal.writeln(err)
    
    def cmd_cp(self, args):
        if len(args) < 2:
            self.terminal.writeln("usage: cp source dest")
            return
        err = self.fs.copy_file(args[0], args[1])
        if err:
            self.terminal.writeln(err)
    
    def cmd_mv(self, args):
        if len(args) < 2:
            self.terminal.writeln("usage: mv source dest")
            return
        err = self.fs.move_file(args[0], args[1])
        if err:
            self.terminal.writeln(err)
    
    def cmd_chmod(self, args):
        if len(args) < 2:
            self.terminal.writeln("usage: chmod mode file")
            return
        err = self.fs.chmod(args[0], args[1])
        if err:
            self.terminal.writeln(err)
    
    def cmd_find(self, args):
        start_path = '.'
        pattern = '*'
        
        i = 0
        while i < len(args):
            if args[i] == '-name' and i + 1 < len(args):
                pattern = args[i + 1].replace('*', '')
                i += 2
            elif not args[i].startswith('-'):
                start_path = args[i]
                i += 1
            else:
                i += 1
        
        results = self.fs.find(pattern, start_path)
        for path in results:
            self.terminal.writeln(path)
    
    def cmd_grep(self, args):
        if len(args) < 2:
            self.terminal.writeln("usage: grep pattern file")
            return
        
        ignore_case = '-i' in args
        args = [a for a in args if not a.startswith('-')]
        
        if len(args) < 2:
            self.terminal.writeln("usage: grep pattern file")
            return
        
        pattern = args[0]
        filename = args[1]
        
        content, err = self.fs.read_file(filename)
        if err:
            self.terminal.writeln(err)
            return
        
        if content:
            search_pattern = pattern.lower() if ignore_case else pattern
            for line in content.split('\n'):
                search_line = line.lower() if ignore_case else line
                if search_pattern in search_line:
                    self.terminal.writeln(line)
    
    def cmd_head(self, args):
        lines = 10
        filename = None
        
        i = 0
        while i < len(args):
            if args[i] == '-n' and i + 1 < len(args):
                try:
                    lines = int(args[i + 1])
                except ValueError:
                    pass
                i += 2
            elif args[i].startswith('-') and args[i][1:].isdigit():
                lines = int(args[i][1:])
                i += 1
            else:
                filename = args[i]
                i += 1
        
        if not filename:
            self.terminal.writeln("usage: head [-n lines] file")
            return
        
        content, err = self.fs.read_file(filename)
        if err:
            self.terminal.writeln(err)
            return
        
        if content:
            for line in content.split('\n')[:lines]:
                self.terminal.writeln(line)
    
    def cmd_tail(self, args):
        lines = 10
        filename = None
        
        i = 0
        while i < len(args):
            if args[i] == '-n' and i + 1 < len(args):
                try:
                    lines = int(args[i + 1])
                except ValueError:
                    pass
                i += 2
            elif args[i].startswith('-') and args[i][1:].isdigit():
                lines = int(args[i][1:])
                i += 1
            else:
                filename = args[i]
                i += 1
        
        if not filename:
            self.terminal.writeln("usage: tail [-n lines] file")
            return
        
        content, err = self.fs.read_file(filename)
        if err:
            self.terminal.writeln(err)
            return
        
        if content:
            all_lines = content.split('\n')
            for line in all_lines[-lines:]:
                self.terminal.writeln(line)
    
    def cmd_wc(self, args):
        if not args:
            self.terminal.writeln("usage: wc file")
            return
        
        filename = args[-1]
        content, err = self.fs.read_file(filename)
        if err:
            self.terminal.writeln(err)
            return
        
        if content:
            lines = content.count('\n')
            words = len(content.split())
            chars = len(content)
            self.terminal.writeln("{:8d}{:8d}{:8d} {}".format(lines, words, chars, filename))
    
    def cmd_ps(self, args):
        self.terminal.writeln("  PID TTY      TIME CMD")
        self.terminal.writeln("    1 tty01    0:01 init")
        self.terminal.writeln("   15 tty01    0:00 sh")
    
    def cmd_who(self, args):
        try:
            t = time.localtime()
            months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
            self.terminal.writeln("{:8s} tty01        {} {:2d} {:02d}:{:02d}".format(
                self.current_user, months[t[1]-1], t[2], t[3], t[4]))
        except:
            self.terminal.writeln("{:8s} tty01        Jan  1 00:00".format(self.current_user))
    
    def cmd_whoami(self, args):
        self.terminal.writeln(self.current_user)
    
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
        if '-a' in args:
            self.terminal.writeln("XENIX pico 2.3.2 i286 i8086")
        elif '-r' in args:
            self.terminal.writeln("2.3.2")
        elif '-m' in args:
            self.terminal.writeln("i286")
        else:
            self.terminal.writeln("XENIX")
    
    def cmd_df(self, args):
        self.terminal.writeln("Filesystem      1K-blocks  Used Available Use% Mounted on")
        self.terminal.writeln("/dev/root           12345  2345      8000  23% /")
        self.terminal.writeln("/dev/tmp             5678   123      5555   3% /tmp")
    
    def cmd_free(self, args):
        if MICROPYTHON:
            gc.collect()
            free_mem = gc.mem_free()
            total = gc.mem_alloc() + free_mem
            used = gc.mem_alloc()
        else:
            total = 65536
            used = 32768
            free_mem = 32768
        
        self.terminal.writeln("              total        used        free")
        self.terminal.writeln("Mem:       {:8d}    {:8d}    {:8d}".format(total, used, free_mem))
    
    def cmd_env(self, args):
        for key, val in sorted(self.env.items()):
            self.terminal.writeln("{}={}".format(key, val))
    
    def cmd_export(self, args):
        if not args:
            self.cmd_env([])
            return
        
        for arg in args:
            if '=' in arg:
                key, val = arg.split('=', 1)
                self.env[key] = val
    
    def cmd_history(self, args):
        for i, cmd in enumerate(self.terminal.history, 1):
            self.terminal.writeln("{:5d}  {}".format(i, cmd))
    
    def cmd_vi(self, args):
        if not args:
            self.terminal.writeln("usage: vi file")
            return
        
        filename = args[0]
        content, _ = self.fs.read_file(filename)
        lines = content.split('\n') if content else ['']
        
        self.terminal.writeln('"{}" {} lines'.format(filename, len(lines)))
        self.terminal.writeln("Commands: i=insert, :w=save, :q=quit, :wq=both, n/p=next/prev line")
        
        editing = True
        insert_mode = False
        current_line = 0
        
        while editing:
            if not insert_mode:
                self.terminal.writeln("\n-- Line {}/{} --".format(current_line + 1, len(lines)))
                if current_line < len(lines):
                    self.terminal.writeln(lines[current_line])
                self.terminal.write(":")
            
            input_text = self.terminal.read_line()
            
            if insert_mode:
                if input_text.upper() == 'ESC':
                    insert_mode = False
                    self.terminal.writeln("-- COMMAND --")
                else:
                    lines[current_line] = input_text
                    current_line += 1
                    if current_line >= len(lines):
                        lines.append('')
            else:
                cmd = input_text.strip().lower()
                if cmd == 'i':
                    insert_mode = True
                    self.terminal.writeln("-- INSERT -- (ESC to exit)")
                elif cmd in ('w', ':w'):
                    self.fs.write_file(filename, '\n'.join(lines))
                    self.terminal.writeln('"{}" written'.format(filename))
                elif cmd in ('q', ':q'):
                    editing = False
                elif cmd in ('wq', ':wq', 'x', ':x'):
                    self.fs.write_file(filename, '\n'.join(lines))
                    self.terminal.writeln('"{}" written'.format(filename))
                    editing = False
                elif cmd == 'n' and current_line < len(lines) - 1:
                    current_line += 1
                elif cmd == 'p' and current_line > 0:
                    current_line -= 1
                elif cmd == 'dd':
                    if len(lines) > 1:
                        lines.pop(current_line)
                        if current_line >= len(lines):
                            current_line = len(lines) - 1
                elif cmd == 'o':
                    lines.insert(current_line + 1, '')
                    current_line += 1
                    insert_mode = True
                    self.terminal.writeln("-- INSERT --")
    
    def cmd_ed(self, args):
        if not args:
            self.terminal.writeln("?")
            return
        
        filename = args[0]
        content, _ = self.fs.read_file(filename)
        buffer_lines = content.split('\n') if content else []
        
        if content:
            self.terminal.writeln(str(len(content)))
        
        editing = True
        current = len(buffer_lines)
        
        while editing:
            self.terminal.write("")
            cmd = self.terminal.read_line().strip()
            
            if cmd == 'a':
                while True:
                    line = self.terminal.read_line()
                    if line == '.':
                        break
                    buffer_lines.append(line)
                current = len(buffer_lines)
            elif cmd == 'p':
                if buffer_lines:
                    self.terminal.writeln(buffer_lines[current - 1] if current > 0 else buffer_lines[0])
            elif cmd == ',p':
                for line in buffer_lines:
                    self.terminal.writeln(line)
            elif cmd == 'w':
                text = '\n'.join(buffer_lines)
                self.fs.write_file(filename, text)
                self.terminal.writeln(str(len(text)))
            elif cmd == 'q':
                editing = False
            elif cmd.isdigit():
                n = int(cmd)
                if 1 <= n <= len(buffer_lines):
                    current = n
                    self.terminal.writeln(buffer_lines[current - 1])
            else:
                self.terminal.writeln("?")
    
    def cmd_banner(self, args):
        if not args:
            return
        
        text = ' '.join(args).upper()[:8]
        
        # Simple block letters
        patterns = {
            'A': ["  #  ", " # # ", "#####", "#   #", "#   #"],
            'B': ["#### ", "#   #", "#### ", "#   #", "#### "],
            'C': [" ####", "#    ", "#    ", "#    ", " ####"],
            'D': ["#### ", "#   #", "#   #", "#   #", "#### "],
            'E': ["#####", "#    ", "###  ", "#    ", "#####"],
            'F': ["#####", "#    ", "###  ", "#    ", "#    "],
            'G': [" ####", "#    ", "#  ##", "#   #", " ### "],
            'H': ["#   #", "#   #", "#####", "#   #", "#   #"],
            'I': ["#####", "  #  ", "  #  ", "  #  ", "#####"],
            'J': ["#####", "   # ", "   # ", "#  # ", " ##  "],
            'K': ["#   #", "#  # ", "###  ", "#  # ", "#   #"],
            'L': ["#    ", "#    ", "#    ", "#    ", "#####"],
            'M': ["#   #", "## ##", "# # #", "#   #", "#   #"],
            'N': ["#   #", "##  #", "# # #", "#  ##", "#   #"],
            'O': [" ### ", "#   #", "#   #", "#   #", " ### "],
            'P': ["#### ", "#   #", "#### ", "#    ", "#    "],
            'Q': [" ### ", "#   #", "# # #", "#  # ", " ## #"],
            'R': ["#### ", "#   #", "#### ", "#  # ", "#   #"],
            'S': [" ####", "#    ", " ### ", "    #", "#### "],
            'T': ["#####", "  #  ", "  #  ", "  #  ", "  #  "],
            'U': ["#   #", "#   #", "#   #", "#   #", " ### "],
            'V': ["#   #", "#   #", "#   #", " # # ", "  #  "],
            'W': ["#   #", "#   #", "# # #", "## ##", "#   #"],
            'X': ["#   #", " # # ", "  #  ", " # # ", "#   #"],
            'Y': ["#   #", " # # ", "  #  ", "  #  ", "  #  "],
            'Z': ["#####", "   # ", "  #  ", " #   ", "#####"],
            ' ': ["     ", "     ", "     ", "     ", "     "],
            '0': [" ### ", "#   #", "#   #", "#   #", " ### "],
            '1': ["  #  ", " ##  ", "  #  ", "  #  ", " ### "],
            '2': [" ### ", "    #", " ### ", "#    ", "#####"],
            '3': [" ### ", "    #", " ### ", "    #", " ### "],
            '4': ["#   #", "#   #", "#####", "    #", "    #"],
            '5': ["#####", "#    ", "#### ", "    #", "#### "],
            '6': [" ### ", "#    ", "#### ", "#   #", " ### "],
            '7': ["#####", "    #", "   # ", "  #  ", "  #  "],
            '8': [" ### ", "#   #", " ### ", "#   #", " ### "],
            '9': [" ### ", "#   #", " ####", "    #", " ### "],
        }
        
        self.terminal.writeln()
        for row in range(5):
            line = ""
            for char in text:
                if char in patterns:
                    line += patterns[char][row] + " "
                else:
                    line += "      "
            self.terminal.writeln(line)
        self.terminal.writeln()
    
    def cmd_write(self, args):
        if not args:
            self.terminal.writeln("usage: write user [tty]")
            return
        self.terminal.writeln("write: {} is not logged on".format(args[0]))
    
    def cmd_wall(self, args):
        self.terminal.writeln()
        self.terminal.writeln("Broadcast message from {}@{} (tty01):".format(
            self.current_user, self.hostname))
        self.terminal.writeln()
        if args:
            self.terminal.writeln(' '.join(args))
        self.terminal.writeln()
    
    def cmd_mesg(self, args):
        if args:
            self.terminal.writeln("is y")
        else:
            self.terminal.writeln("is y")
    
    def cmd_help(self, args):
        self.terminal.writeln("XENIX Commands:")
        self.terminal.writeln()
        self.terminal.writeln("Files:    ls cd pwd mkdir rmdir cat touch rm cp mv chmod")
        self.terminal.writeln("Text:     grep head tail wc vi ed")
        self.terminal.writeln("Search:   find")
        self.terminal.writeln("System:   ps who whoami date clear uname df free")
        self.terminal.writeln("Shell:    echo env export history")
        self.terminal.writeln("Misc:     banner write wall mesg help exit")
    
    def cmd_exit(self, args):
        self.terminal.writeln("Saving...")
        err = self.fs.save(self.save_file)
        if err:
            self.terminal.writeln(err)
        self.terminal.writeln()
        self.terminal.writeln("logout")
        self.running = False


def main():
    xenix = XenixOS()
    xenix.boot()


if __name__ == '__main__':
    main()