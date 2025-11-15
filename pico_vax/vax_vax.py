import sys
import time
import json
import os
import re

class FileNode:
    def __init__(self, name, is_directory=False, uic="[1,4]"):
        self.name = name
        self.is_directory = is_directory
        self.content = ""
        # VMS file protection: (System,Owner,Group,World) - RWED bits
        self.protection = "(S:RWED,O:RWED,G:RE,W:)" if is_directory else "(S:RWED,O:RWED,G:R,W:)"
        self.owner = "SYSTEM"
        self.uic = uic  # User Identification Code [group,member]
        self.size = 0
        self.version = 1
        self.modified = self._get_timestamp()
        self.record_format = "Variable" if not is_directory else None
        self.record_attributes = "Carriage return carriage control" if not is_directory else None
        self.children = {} if is_directory else None
        self.versions = {} if not is_directory else None  # Store multiple versions
    
    def _get_timestamp(self):
        try:
            t = time.localtime()
            months = ["JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                     "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"]
            return f"{t[2]}-{months[t[1]-1]}-{t[0]} {t[3]:02d}:{t[4]:02d}:{t[5]:02d}.00"
        except (IndexError, ValueError, OSError):
            return "1-JAN-1989 00:00:00.00"
    
    def to_dict(self):
        d = {
            'name': self.name,
            'is_directory': self.is_directory,
            'content': self.content,
            'protection': self.protection,
            'owner': self.owner,
            'uic': self.uic,
            'size': self.size,
            'version': self.version,
            'modified': self.modified,
            'record_format': self.record_format,
            'record_attributes': self.record_attributes
        }
        if self.is_directory:
            d['children'] = {k: v.to_dict() for k, v in self.children.items()}
        else:
            d['versions'] = self.versions if self.versions else {}
        return d
    
    @staticmethod
    def from_dict(d):
        node = FileNode(d['name'], d['is_directory'], d.get('uic', '[1,4]'))
        node.content = d.get('content', '')
        # Handle old 'permissions' field or new 'protection' field
        node.protection = d.get('protection', d.get('permissions',
            '(S:RWED,O:RWED,G:RE,W:)' if d['is_directory'] else '(S:RWED,O:RWED,G:R,W:)'))
        node.owner = d.get('owner', 'SYSTEM')
        node.size = d.get('size', 0)
        node.version = d.get('version', 1)
        node.modified = d.get('modified', '1-JAN-1989 00:00:00.00')
        node.record_format = d.get('record_format', 'Variable' if not d['is_directory'] else None)
        node.record_attributes = d.get('record_attributes', 'Carriage return carriage control' if not d['is_directory'] else None)
        if d['is_directory']:
            node.children = {k: FileNode.from_dict(v) for k, v in d.get('children', {}).items()}
        else:
            node.versions = d.get('versions', {})
        return node


class FileSystem:
    def __init__(self):
        self.root = FileNode("SYS$SYSDEVICE:[000000]", True)
        self.current = self.root
        self.path_stack = ["SYS$SYSDEVICE:[000000]"]
        self._create_initial_structure()

    def _match_wildcard(self, pattern, text):
        """Simple wildcard matching for VMS filenames (* and %)"""
        pattern = pattern.replace('*', '.*').replace('%', '.')
        return re.match('^' + pattern + '$', text) is not None
    
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
                # Store old version
                old_version = file_node.version
                if file_node.versions is None:
                    file_node.versions = {}
                file_node.versions[old_version] = {
                    'content': file_node.content,
                    'size': file_node.size,
                    'modified': file_node.modified
                }
                file_node.version += 1
        else:
            file_node = FileNode(name, False)
            self.current.children[name] = file_node

        if not file_node.is_directory:
            file_node.content = content
            file_node.size = len(content)
            file_node.modified = file_node._get_timestamp()
        return None
    
    def read_file(self, name, version=None):
        """Read a file, optionally specifying version number"""
        name = name.upper()

        # Check if version is specified in filename (e.g., FILE.TXT;2)
        if ';' in name:
            parts = name.split(';')
            name = parts[0]
            try:
                version = int(parts[1])
            except:
                pass

        if '.' not in name:
            name = name + '.DAT'

        if name in self.current.children:
            node = self.current.children[name]
            if not node.is_directory:
                # If specific version requested
                if version is not None and version != node.version:
                    if node.versions and version in node.versions:
                        return node.versions[version]['content'], None
                    else:
                        return None, "%TYPE-E-FILENOTFOUND, version not found"
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
            if pattern == '*' or pattern == '*.*' or self._match_wildcard(pattern, name):
                results.append(name)
        return results

    def purge_files(self, pattern='*.*', keep=1):
        """Purge old versions of files, keeping only the newest 'keep' versions"""
        purged = 0
        pattern = pattern.upper()
        if '.' not in pattern and pattern != '*':
            pattern = pattern + '.DAT'

        for name, node in list(self.current.children.items()):
            if node.is_directory:
                continue
            if self._match_wildcard(pattern, name):
                if node.versions and len(node.versions) > 0:
                    # Keep only the newest version(s)
                    versions_to_delete = len(node.versions)
                    node.versions = {}
                    purged += versions_to_delete

        return purged
    
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
        except (OSError, ValueError, KeyError):
            # If file doesn't exist or is corrupt, just use default filesystem
            # Note: MicroPython uses OSError instead of FileNotFoundError
            # ValueError covers json.JSONDecodeError
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

            # Handle empty string (EOF condition)
            if not char or char == '\x04':
                if not line:
                    self.writeln("LOGOUT")
                    return "LOGOUT"
                else:
                    # EOF while typing - just return what we have
                    self.writeln()
                    return line

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
            elif char == '\x15':
                for _ in range(len(line)):
                    self.write('\b \b')
                line = ""
            elif len(char) > 0 and ord(char) >= 32 and ord(char) < 127:
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
        self.symbols = {}  # DCL symbols/variables

        self.fs.load(self.save_file)

    def _match_command(self, input_cmd, valid_cmds):
        """Match abbreviated command to full command name (minimum uniqueness)"""
        input_cmd = input_cmd.upper()
        matches = []
        for cmd in valid_cmds:
            if cmd.startswith(input_cmd):
                matches.append(cmd)

        if len(matches) == 1:
            return matches[0]
        elif len(matches) > 1:
            # Check if input is exactly one of the matches
            if input_cmd in matches:
                return input_cmd
            return None  # Ambiguous
        return None  # No match

    def _parse_qualifiers(self, args):
        """Parse DCL qualifiers (words starting with /) from arguments"""
        params = []
        qualifiers = {}

        for arg in args:
            if arg.startswith('/'):
                # It's a qualifier
                qual = arg[1:].upper()
                if '=' in qual:
                    key, value = qual.split('=', 1)
                    qualifiers[key] = value
                else:
                    qualifiers[qual] = True
            else:
                params.append(arg)

        return params, qualifiers
    
    def boot(self):
        self.terminal.clear_screen()
        self.terminal.writeln("")
        self.terminal.writeln("")
        self.terminal.writeln("VAX/VMS Version V5.5-2")
        self.terminal.writeln("")
        self.terminal.writeln("    Welcome to VAX/VMS (TM) on Raspberry Pi Pico")
        self.terminal.writeln("")
        try:
            import gc
            gc.collect()
            free = gc.mem_free()
            total = gc.mem_alloc() + free
            self.terminal.writeln("    Memory: {} bytes".format(total))
        except:
            pass
        self.terminal.writeln("")
        self.terminal.writeln(" (c) Copyright 1991 Digital Equipment Corporation.")
        self.terminal.writeln("")

        # Display welcome message
        content, err = self.fs.read_file('[SYSMGR]WELCOME.TXT')
        if content:
            self.terminal.writeln(content)

        self.terminal.writeln("")
        self.terminal.writeln("Username: " + self.current_user)
        self.terminal.writeln("")
        self.terminal.writeln("%LOGIN-I-LOGGEDIN, logged in as " + self.current_user)
        try:
            t = time.localtime()
            months = ["JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                     "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"]
            timestr = "{}-{}-{} {:02d}:{:02d}:{:02d}".format(
                t[2], months[t[1]-1], t[0], t[3], t[4], t[5])
            self.terminal.writeln("        on {}::TXA0: at {}".format(self.node, timestr))
        except:
            pass
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

        # Define valid commands (in order for abbreviation matching)
        commands = [
            'ALLOCATE', 'ANALYZE', 'APPEND', 'ASSIGN', 'BACKUP',
            'CLOSE', 'CONTINUE', 'COPY', 'CREATE', 'DEALLOCATE',
            'DEASSIGN', 'DEBUG', 'DEFINE', 'DELETE', 'DIFFERENCES',
            'DIRECTORY', 'DISMOUNT', 'DUMP', 'EDIT', 'EXIT',
            'GOTO', 'HELP', 'IF', 'INITIALIZE', 'INSTALL',
            'LIBRARY', 'LINK', 'LOGIN', 'LOGOUT', 'MAIL',
            'MERGE', 'MOUNT', 'ON', 'OPEN', 'PATCH',
            'PHONE', 'PRINT', 'PURGE', 'READ', 'RECALL',
            'RENAME', 'RUN', 'SEARCH', 'SET', 'SHOW',
            'SORT', 'SPAWN', 'STOP', 'SUBMIT', 'TYPE',
            'WAIT', 'WRITE'
        ]

        # Handle DIR as alias for DIRECTORY
        if cmd == 'DIR':
            cmd = 'DIRECTORY'

        # Try to match abbreviated command
        matched_cmd = self._match_command(cmd, commands)

        if matched_cmd is None:
            # Check if it's an exact match that we support
            if cmd in ['DIR', 'DIRECTORY', 'SET', 'SHOW', 'CREATE', 'DELETE',
                      'TYPE', 'PRINT', 'COPY', 'RENAME', 'SEARCH', 'HELP',
                      'LOGOUT', 'EXIT', 'EDIT', 'RUN', 'SUBMIT', 'MAIL',
                      'PHONE', 'PURGE', 'ASSIGN', 'DEASSIGN', 'APPEND',
                      'DIFFERENCES', 'DUMP', 'ANALYZE', 'SORT', 'SPAWN',
                      'BACKUP', 'MOUNT', 'DISMOUNT', 'INITIALIZE']:
                matched_cmd = cmd
            else:
                self.terminal.writeln("%DCL-W-IVVERB, unrecognized command verb - check validity and spelling")
                self.terminal.writeln("\\{0}\\".format(cmd))
                return

        # Execute the matched command
        cmd_map = {
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
            'ASSIGN': self.cmd_assign,
            'DEASSIGN': self.cmd_deassign,
            'APPEND': self.cmd_append,
            'DIFFERENCES': self.cmd_differences,
            'DUMP': self.cmd_dump,
            'ANALYZE': self.cmd_analyze,
            'SORT': self.cmd_sort,
            'SPAWN': self.cmd_spawn,
            'BACKUP': self.cmd_backup,
            'MOUNT': self.cmd_mount,
            'DISMOUNT': self.cmd_dismount,
            'INITIALIZE': self.cmd_initialize,
        }

        if matched_cmd in cmd_map:
            cmd_map[matched_cmd](args)
        else:
            self.terminal.writeln("%DCL-W-NOTINSTALLED, command not installed")
            self.terminal.writeln("\\{0}\\".format(matched_cmd))
    
    def cmd_dir(self, args):
        params, qualifiers = self._parse_qualifiers(args)

        # Get file pattern (default to *.*)
        pattern = params[0] if params else '*.*'

        files = self.fs.list_files()

        # Filter by pattern
        if pattern != '*.*' and pattern != '*':
            pattern_upper = pattern.upper()
            if ';' in pattern_upper:
                pattern_upper = pattern_upper.split(';')[0]
            if '.' not in pattern_upper and pattern_upper != '*':
                pattern_upper = pattern_upper + '.DAT'
            files = [f for f in files if self.fs._match_wildcard(pattern_upper, f.name)]

        if not files:
            self.terminal.writeln("")
            self.terminal.writeln("%DIRECT-W-NOFILES, no files found")
            return

        self.terminal.writeln("")
        self.terminal.writeln("Directory " + self.fs.get_current_path())
        self.terminal.writeln("")

        # Check for /FULL or /BRIEF qualifier
        if qualifiers.get('FULL'):
            for f in files:
                if f.is_directory:
                    self.terminal.writeln("")
                    self.terminal.writeln("{}.DIR;1".format(f.name))
                    self.terminal.writeln("  Owner:           [{}]".format(f.uic))
                    self.terminal.writeln("  File protection: {}".format(f.protection))
                else:
                    self.terminal.writeln("")
                    self.terminal.writeln("{};{}".format(f.name, f.version))
                    self.terminal.writeln("  Size:            {:8d}".format(f.size))
                    self.terminal.writeln("  Owner:           [{},{}]".format(f.owner, f.uic))
                    self.terminal.writeln("  Created:         {}".format(f.modified))
                    self.terminal.writeln("  Revised:         {} ({})".format(f.modified, f.version))
                    self.terminal.writeln("  File protection: {}".format(f.protection))
                    if f.record_format:
                        self.terminal.writeln("  File organization: Sequential")
                        self.terminal.writeln("  Record format:   {}".format(f.record_format))
                        if f.record_attributes:
                            self.terminal.writeln("  Record attributes: {}".format(f.record_attributes))
        else:
            # Brief/normal format
            for f in files:
                if f.is_directory:
                    self.terminal.writeln("{}.DIR;1".format(f.name))
                else:
                    # Show all versions if they exist
                    self.terminal.writeln("{};{}    {:8d}  {}".format(
                        f.name, f.version, f.size, f.modified))
                    if f.versions and qualifiers.get('VERSIONS'):
                        for ver_num in sorted(f.versions.keys(), reverse=True):
                            ver_data = f.versions[ver_num]
                            self.terminal.writeln("{};{}    {:8d}  {}".format(
                                f.name, ver_num, ver_data['size'], ver_data['modified']))

        self.terminal.writeln("")
        self.terminal.writeln("Total of {} files".format(len(files)))
    
    def cmd_set(self, args):
        if not args:
            self.terminal.writeln("%SET-E-TOOFEW, too few arguments")
            return

        params, qualifiers = self._parse_qualifiers(args)
        subcmd = params[0].upper() if params else ""

        # Match abbreviated SET subcommands
        set_cmds = ['DEFAULT', 'TERMINAL', 'PROCESS', 'PROMPT', 'VERIFY', 'FILE']
        matched_subcmd = self._match_command(subcmd, set_cmds)
        if matched_subcmd:
            subcmd = matched_subcmd

        # Debug: print what we're trying to match
        # self.terminal.writeln(f"DEBUG: subcmd={subcmd}, params={params}, qualifiers={qualifiers}")

        if subcmd == 'DEFAULT':
            if len(params) > 1:
                err = self.fs.change_directory(params[1])
                if err:
                    self.terminal.writeln(err)
            else:
                self.terminal.writeln(self.fs.get_current_path())
        elif subcmd == 'TERMINAL':
            self.terminal.writeln("%SET-I-TERMINAL, terminal characteristics set")
        elif subcmd == 'FILE':
            # SET FILE /PROTECTION=(...) filename
            # OR: SET FILE filename /PROTECTION=(...)
            # Handle both orders
            if 'PROTECTION' in qualifiers:
                # Find the filename (it's a non-qualifier parameter)
                filename = None
                for p in params[1:]:  # Skip 'FILE' itself
                    if not p.startswith('/'):
                        filename = p.upper()
                        break

                if not filename:
                    self.terminal.writeln("%SET-E-NOFILE, no file specified")
                    return

                if '.' not in filename:
                    filename = filename + '.DAT'
                protection = qualifiers['PROTECTION']
                if filename in self.fs.current.children:
                    self.fs.current.children[filename].protection = protection
                    self.terminal.writeln("%SET-I-MODIFIED, file protection modified")
                else:
                    self.terminal.writeln("%SET-E-FILENOTFOUND, file not found")
            else:
                self.terminal.writeln("%SET-E-INVQUAL, /PROTECTION qualifier required")
        else:
            self.terminal.writeln("%SET-W-NOTSET, item not set")
    
    def cmd_show(self, args):
        if not args:
            self.terminal.writeln("%SHOW-E-TOOFEW, too few arguments")
            return

        params, qualifiers = self._parse_qualifiers(args)
        subcmd = params[0].upper() if params else ""

        # Match abbreviated SHOW subcommands
        show_cmds = ['DEFAULT', 'TIME', 'SYSTEM', 'USERS', 'MEMORY', 'DEVICES',
                     'SYMBOL', 'LOGICAL', 'PROCESS', 'QUOTA']
        matched_subcmd = self._match_command(subcmd, show_cmds)
        if matched_subcmd:
            subcmd = matched_subcmd

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
            self.terminal.writeln("  OpenVMS V5.5-2 on node {}".format(self.node))
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
            self.terminal.writeln("  SYS$SYSDEVICE:   System Disk    Mounted")
        elif subcmd == 'SYMBOL' or subcmd == 'LOGICAL':
            if len(params) > 1:
                # Show specific symbol
                symbol = params[1].upper()
                if symbol in self.symbols:
                    self.terminal.writeln('  {} = "{}"'.format(symbol, self.symbols[symbol]))
                else:
                    self.terminal.writeln("%SHOW-W-NOTFOUND, {} is not a known symbol".format(symbol))
            else:
                # Show all symbols
                if self.symbols:
                    for name, value in self.symbols.items():
                        self.terminal.writeln('  {} = "{}"'.format(name, value))
                else:
                    self.terminal.writeln("  No symbols defined")
        elif subcmd == 'PROCESS':
            self.terminal.writeln("  Process name:    SYSTEM")
            self.terminal.writeln("  Process ID:      00000042")
            self.terminal.writeln("  Terminal:        TXA0:")
            self.terminal.writeln("  User name:       {}".format(self.current_user))
        elif subcmd == 'QUOTA':
            self.terminal.writeln("  Disk quotas for user {}".format(self.current_user))
            self.terminal.writeln("  Disk Usage:     Unlimited")
            self.terminal.writeln("  File Count:     Unlimited")
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
        self.terminal.writeln("Commands: INSERT (I), LIST (L), DELETE line# (D), REPLACE line# (R)")
        self.terminal.writeln("          FIND text (F), SUBSTITUTE/old/new/ (S), EXIT (E), QUIT (Q)")
        self.terminal.writeln("")

        buffer_lines = content.split('\n') if content else []
        editing = True

        while editing:
            self.terminal.write("*")
            cmd_input = self.terminal.read_line().strip()
            if not cmd_input:
                continue

            # For SUBSTITUTE command, don't split on spaces
            if cmd_input.upper().startswith('S/') or cmd_input.upper().startswith('SUBSTITUTE/'):
                cmd = 'SUBSTITUTE'
                cmd_parts = [cmd_input]
            else:
                cmd_parts = cmd_input.split()
                cmd = cmd_parts[0].upper()

            if cmd == 'INSERT' or cmd == 'I':
                self.terminal.writeln("Enter text (type . on a line to finish):")
                while True:
                    line = self.terminal.read_line()
                    if line == '.':
                        break
                    buffer_lines.append(line)
            elif cmd == 'LIST' or cmd == 'L':
                self.terminal.writeln("")
                for i, line in enumerate(buffer_lines, 1):
                    self.terminal.writeln(f"{i:4d}  {line}")
                self.terminal.writeln("")
            elif cmd == 'DELETE' or cmd == 'D':
                if len(cmd_parts) > 1:
                    try:
                        line_num = int(cmd_parts[1]) - 1
                        if 0 <= line_num < len(buffer_lines):
                            del buffer_lines[line_num]
                            self.terminal.writeln(f"%EDIT-I-DELETED, line {line_num + 1} deleted")
                        else:
                            self.terminal.writeln("%EDIT-E-INVLINE, invalid line number")
                    except ValueError:
                        self.terminal.writeln("%EDIT-E-INVNUM, invalid number")
                else:
                    self.terminal.writeln("%EDIT-E-NOLINENUM, line number required")
            elif cmd == 'REPLACE' or cmd == 'R':
                if len(cmd_parts) > 1:
                    try:
                        line_num = int(cmd_parts[1]) - 1
                        if 0 <= line_num < len(buffer_lines):
                            self.terminal.writeln(f"Current: {buffer_lines[line_num]}")
                            self.terminal.write("New: ")
                            new_line = self.terminal.read_line()
                            buffer_lines[line_num] = new_line
                            self.terminal.writeln("%EDIT-I-REPLACED, line replaced")
                        else:
                            self.terminal.writeln("%EDIT-E-INVLINE, invalid line number")
                    except ValueError:
                        self.terminal.writeln("%EDIT-E-INVNUM, invalid number")
                else:
                    self.terminal.writeln("%EDIT-E-NOLINENUM, line number required")
            elif cmd == 'FIND' or cmd == 'F':
                if len(cmd_parts) > 1:
                    search_text = ' '.join(cmd_parts[1:])
                    found = False
                    for i, line in enumerate(buffer_lines, 1):
                        if search_text in line:
                            self.terminal.writeln(f"{i:4d}  {line}")
                            found = True
                    if not found:
                        self.terminal.writeln("%EDIT-W-NOTFOUND, text not found")
                else:
                    self.terminal.writeln("%EDIT-E-NOTEXT, search text required")
            elif cmd == 'SUBSTITUTE' or cmd == 'S':
                # Format: SUBSTITUTE/old/new/
                if '/' in cmd_input:
                    parts = cmd_input.split('/')
                    if len(parts) >= 3:
                        old_text = parts[1]
                        new_text = parts[2]
                        count = 0
                        for i in range(len(buffer_lines)):
                            if old_text in buffer_lines[i]:
                                buffer_lines[i] = buffer_lines[i].replace(old_text, new_text)
                                count += 1
                        self.terminal.writeln(f"%EDIT-I-SUBSTITUTED, {count} substitution(s) made")
                    else:
                        self.terminal.writeln("%EDIT-E-INVFORMAT, format: SUBSTITUTE/old/new/")
                else:
                    self.terminal.writeln("%EDIT-E-INVFORMAT, format: SUBSTITUTE/old/new/")
            elif cmd == 'EXIT' or cmd == 'E':
                self.fs.write_file(filename, '\n'.join(buffer_lines))
                self.terminal.writeln("%EDIT-I-SAVED, file saved")
                editing = False
            elif cmd == 'QUIT' or cmd == 'Q':
                editing = False
            else:
                self.terminal.writeln("?Unknown command. Type one of: I, L, D, R, F, S, E, Q")
    
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
        params, qualifiers = self._parse_qualifiers(args)

        pattern = params[0] if params else '*.*'
        purged = self.fs.purge_files(pattern)

        if purged > 0:
            self.terminal.writeln("%PURGE-I-FILPURG, {} file version{} purged".format(
                purged, 's' if purged != 1 else ''))
        else:
            self.terminal.writeln("%PURGE-I-FILPURG, no old versions found")
    
    def cmd_help(self, args):
        self.terminal.writeln("")
        self.terminal.writeln("Available DCL commands:")
        self.terminal.writeln("")
        self.terminal.writeln("  File Operations:")
        self.terminal.writeln("    DIRECTORY (DIR)   - List files (supports wildcards, /FULL, /VERSIONS)")
        self.terminal.writeln("    TYPE              - Display file contents (supports version ;n)")
        self.terminal.writeln("    COPY              - Copy files")
        self.terminal.writeln("    RENAME            - Rename files")
        self.terminal.writeln("    DELETE            - Delete files")
        self.terminal.writeln("    CREATE/DIR        - Create directory")
        self.terminal.writeln("    PURGE             - Remove old file versions")
        self.terminal.writeln("    APPEND            - Append one file to another")
        self.terminal.writeln("    DIFFERENCES       - Compare two files")
        self.terminal.writeln("    DUMP              - Hex dump of file")
        self.terminal.writeln("    SORT              - Sort file contents")
        self.terminal.writeln("    BACKUP            - Backup files")
        self.terminal.writeln("")
        self.terminal.writeln("  System Commands:")
        self.terminal.writeln("    SET DEFAULT       - Change directory")
        self.terminal.writeln("    SET FILE file /PROTECTION=(...) - Set file protection")
        self.terminal.writeln("    SHOW DEFAULT      - Display current directory")
        self.terminal.writeln("    SHOW TIME         - Display date/time")
        self.terminal.writeln("    SHOW SYSTEM       - System information")
        self.terminal.writeln("    SHOW USERS        - Display logged in users")
        self.terminal.writeln("    SHOW PROCESS      - Display process information")
        self.terminal.writeln("    SHOW SYMBOL       - Display DCL symbols")
        self.terminal.writeln("    SHOW DEVICES      - Display devices")
        self.terminal.writeln("    SHOW MEMORY       - Display memory usage")
        self.terminal.writeln("    SHOW QUOTA        - Display disk quota")
        self.terminal.writeln("    ASSIGN            - Assign logical name")
        self.terminal.writeln("    DEASSIGN          - Deassign logical name")
        self.terminal.writeln("    MOUNT             - Mount device")
        self.terminal.writeln("    DISMOUNT          - Dismount device")
        self.terminal.writeln("    INITIALIZE        - Initialize device")
        self.terminal.writeln("")
        self.terminal.writeln("  Utilities:")
        self.terminal.writeln("    EDIT              - Enhanced EDT editor (I,L,D,R,F,S,E,Q)")
        self.terminal.writeln("    SEARCH            - Search in files")
        self.terminal.writeln("    ANALYZE           - Analyze disk/file")
        self.terminal.writeln("    MAIL              - Mail utility")
        self.terminal.writeln("    PHONE             - Phone facility")
        self.terminal.writeln("    SPAWN             - Create subprocess")
        self.terminal.writeln("")
        self.terminal.writeln("  Other:")
        self.terminal.writeln("    RUN               - Run program")
        self.terminal.writeln("    SUBMIT            - Submit batch job")
        self.terminal.writeln("    PRINT             - Queue file for printing")
        self.terminal.writeln("    LOGOUT (EXIT)     - Exit system")
        self.terminal.writeln("")
        self.terminal.writeln("  Note: Commands can be abbreviated to minimum unique characters")
        self.terminal.writeln("        Example: DIR, DIRE, DIREC, DIRECT all work for DIRECTORY")
        self.terminal.writeln("        File protection format: (S:RWED,O:RWED,G:RE,W:)")
        self.terminal.writeln("")
    
    def cmd_assign(self, args):
        """ASSIGN logical-name equivalence-name"""
        params, qualifiers = self._parse_qualifiers(args)

        if len(params) < 2:
            self.terminal.writeln("%ASSIGN-E-TOOFEW, too few arguments")
            return

        logical = params[0].upper()
        equiv = params[1].upper()
        self.symbols[logical] = equiv
        self.terminal.writeln("%DCL-I-SUPERSEDE, previous value has been superseded")

    def cmd_deassign(self, args):
        """DEASSIGN logical-name"""
        params, qualifiers = self._parse_qualifiers(args)

        if not params:
            self.terminal.writeln("%DEASSIGN-E-TOOFEW, too few arguments")
            return

        logical = params[0].upper()
        if logical in self.symbols:
            del self.symbols[logical]
        else:
            self.terminal.writeln("%DEASSIGN-W-NOTLOG, {} is not a known logical name".format(logical))

    def cmd_append(self, args):
        """APPEND file1 file2 - append file1 to file2"""
        params, qualifiers = self._parse_qualifiers(args)

        if len(params) < 2:
            self.terminal.writeln("%APPEND-E-TOOFEW, too few arguments")
            return

        content1, err1 = self.fs.read_file(params[0])
        if err1:
            self.terminal.writeln(err1)
            return

        content2, err2 = self.fs.read_file(params[1])
        if err2:
            content2 = ""

        self.fs.write_file(params[1], content2 + content1)
        self.terminal.writeln("%APPEND-I-APPENDED, {} appended to {}".format(
            params[0].upper(), params[1].upper()))

    def cmd_differences(self, args):
        """DIFFERENCES file1 file2"""
        params, qualifiers = self._parse_qualifiers(args)

        if len(params) < 2:
            self.terminal.writeln("%DIFFERENCES-E-TOOFEW, too few arguments")
            return

        content1, err1 = self.fs.read_file(params[0])
        if err1:
            self.terminal.writeln(err1)
            return

        content2, err2 = self.fs.read_file(params[1])
        if err2:
            self.terminal.writeln(err2)
            return

        if content1 == content2:
            self.terminal.writeln("Number of difference sections found: 0")
            self.terminal.writeln("Number of difference records found: 0")
        else:
            self.terminal.writeln("Number of difference sections found: 1")
            lines1 = content1.split('\n')
            lines2 = content2.split('\n')
            self.terminal.writeln("Number of difference records found: {}".format(
                abs(len(lines1) - len(lines2))))

    def cmd_dump(self, args):
        """DUMP file"""
        params, qualifiers = self._parse_qualifiers(args)

        if not params:
            self.terminal.writeln("%DUMP-E-TOOFEW, too few arguments")
            return

        content, err = self.fs.read_file(params[0])
        if err:
            self.terminal.writeln(err)
            return

        self.terminal.writeln("")
        self.terminal.writeln("File dump of {}".format(params[0].upper()))
        self.terminal.writeln("")

        # Hex dump format
        for i in range(0, len(content), 16):
            chunk = content[i:i+16]
            hex_part = ' '.join('{:02X}'.format(ord(c)) for c in chunk)
            ascii_part = ''.join(c if 32 <= ord(c) < 127 else '.' for c in chunk)
            self.terminal.writeln("{:08X}  {:48s}  {}".format(i, hex_part, ascii_part))

    def cmd_analyze(self, args):
        """ANALYZE command stub"""
        params, qualifiers = self._parse_qualifiers(args)

        if not params:
            self.terminal.writeln("%ANALYZE-E-TOOFEW, too few arguments")
            return

        self.terminal.writeln("%ANALYZE-I-STARTED, analysis started")
        self.terminal.writeln("%ANALYZE-I-COMPLETE, analysis complete")

    def cmd_sort(self, args):
        """SORT input-file output-file [/KEY=(POS:size,...))]"""
        params, qualifiers = self._parse_qualifiers(args)

        if len(params) < 2:
            self.terminal.writeln("%SORT-E-TOOFEW, too few arguments")
            self.terminal.writeln("  Usage: SORT input-file output-file")
            return

        input_file = params[0]
        output_file = params[1]

        content, err = self.fs.read_file(input_file)
        if err:
            self.terminal.writeln(err)
            return

        lines = content.split('\n')
        sorted_lines = sorted(lines)

        self.fs.write_file(output_file, '\n'.join(sorted_lines))
        self.terminal.writeln(f"%SORT-I-SORTED, {len(lines)} records sorted")
        self.terminal.writeln(f"%SORT-I-CREATED, {output_file.upper()} created")

    def cmd_spawn(self, args):
        """SPAWN - Create subprocess (limited functionality)"""
        params, qualifiers = self._parse_qualifiers(args)

        self.terminal.writeln("%SPAWN-I-RETURNED, subprocess returned to caller")
        # In real VMS, this would spawn a subprocess
        # For this emulator, we just acknowledge the command

    def cmd_backup(self, args):
        """BACKUP source destination"""
        params, qualifiers = self._parse_qualifiers(args)

        if len(params) < 2:
            self.terminal.writeln("%BACKUP-E-TOOFEW, too few arguments")
            return

        source = params[0]
        destination = params[1]

        # Simple implementation: just copy the file
        content, err = self.fs.read_file(source)
        if err:
            self.terminal.writeln(err)
            return

        # Create backup with .BCK extension if not specified
        if '.' not in destination:
            destination = destination + '.BCK'

        self.fs.write_file(destination, content)
        self.terminal.writeln("%BACKUP-I-COPIED, copied " + source.upper())
        self.terminal.writeln("%BACKUP-S-BACKUPCOMPLETE, backup operation completed successfully")

    def cmd_mount(self, args):
        """MOUNT device-name volume-label"""
        params, qualifiers = self._parse_qualifiers(args)

        if not params:
            self.terminal.writeln("%MOUNT-E-TOOFEW, too few arguments")
            return

        device = params[0].upper()
        volume = params[1].upper() if len(params) > 1 else "VOLUME1"

        self.terminal.writeln(f"%MOUNT-I-MOUNTED, {volume} mounted on {device}")

    def cmd_dismount(self, args):
        """DISMOUNT device-name"""
        params, qualifiers = self._parse_qualifiers(args)

        if not params:
            self.terminal.writeln("%DISMOUNT-E-TOOFEW, too few arguments")
            return

        device = params[0].upper()
        self.terminal.writeln(f"%DISMOUNT-I-DISMOUNTED, {device} dismounted")

    def cmd_initialize(self, args):
        """INITIALIZE device-name volume-label"""
        params, qualifiers = self._parse_qualifiers(args)

        if len(params) < 2:
            self.terminal.writeln("%INITIALIZE-E-TOOFEW, too few arguments")
            return

        device = params[0].upper()
        volume = params[1].upper()

        self.terminal.writeln(f"%INITIALIZE-I-INITIALIZED, {device} initialized as {volume}")

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
