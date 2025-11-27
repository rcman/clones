"""
VAX/VMS Emulator for Raspberry Pi Pico
Optimized for MicroPython with memory efficiency and authentic VMS features.

Features:
- DCL command procedures (.COM files)
- Lexical functions (F$TIME, F$LENGTH, etc.)
- File versioning
- Logical names (ASSIGN/DEFINE)
- SET PROTECTION with full parsing
- Memory-optimized with __slots__
"""

import sys
import time
import json
import re

# Constants to reduce string allocations
_MONTHS = ("JAN", "FEB", "MAR", "APR", "MAY", "JUN",
           "JUL", "AUG", "SEP", "OCT", "NOV", "DEC")
_DIR_PROT = "(S:RWED,O:RWED,G:RE,W:)"
_FILE_PROT = "(S:RWED,O:RWED,G:R,W:)"


def _get_timestamp():
    """Get VMS-style timestamp."""
    try:
        t = time.localtime()
        return "{}-{}-{} {:02d}:{:02d}:{:02d}.00".format(
            t[2], _MONTHS[t[1]-1], t[0], t[3], t[4], t[5])
    except (IndexError, ValueError, OSError):
        return "1-JAN-1989 00:00:00.00"


def _match_wildcard(pattern, text):
    """VMS wildcard matching (* and %) with proper escaping."""
    # Escape regex special chars first, then convert VMS wildcards
    pattern = re.escape(pattern)
    pattern = pattern.replace(r'\*', '.*').replace(r'\%', '.')
    return re.match('^' + pattern + '$', text, re.IGNORECASE) is not None


class FileNode:
    """Memory-efficient file/directory node using __slots__."""
    __slots__ = ('name', 'is_directory', 'content', 'protection', 'owner',
                 'uic', 'size', 'version', 'modified', 'record_format',
                 'record_attributes', 'children', 'versions')

    def __init__(self, name, is_directory=False, uic="[1,4]"):
        self.name = name
        self.is_directory = is_directory
        self.content = ""
        self.protection = _DIR_PROT if is_directory else _FILE_PROT
        self.owner = "SYSTEM"
        self.uic = uic
        self.size = 0
        self.version = 1
        self.modified = _get_timestamp()
        self.record_format = None if is_directory else "Variable"
        self.record_attributes = None if is_directory else "Carriage return carriage control"
        self.children = {} if is_directory else None
        self.versions = None if is_directory else {}

    def to_dict(self):
        """Serialize to dictionary for JSON storage."""
        d = {
            'n': self.name,  # Shortened keys save memory
            'd': self.is_directory,
            'c': self.content,
            'p': self.protection,
            'o': self.owner,
            'u': self.uic,
            's': self.size,
            'v': self.version,
            'm': self.modified,
        }
        if self.is_directory:
            d['ch'] = {k: v.to_dict() for k, v in self.children.items()}
        elif self.versions:
            d['vs'] = self.versions
        return d

    @staticmethod
    def from_dict(d):
        """Deserialize from dictionary."""
        # Handle both old and new key formats
        name = d.get('n', d.get('name', ''))
        is_dir = d.get('d', d.get('is_directory', False))
        node = FileNode(name, is_dir, d.get('u', d.get('uic', '[1,4]')))
        node.content = d.get('c', d.get('content', ''))
        node.protection = d.get('p', d.get('protection',
            d.get('permissions', _DIR_PROT if is_dir else _FILE_PROT)))
        node.owner = d.get('o', d.get('owner', 'SYSTEM'))
        node.size = d.get('s', d.get('size', 0))
        node.version = d.get('v', d.get('version', 1))
        node.modified = d.get('m', d.get('modified', '1-JAN-1989 00:00:00.00'))

        if is_dir:
            children = d.get('ch', d.get('children', {}))
            node.children = {k: FileNode.from_dict(v) for k, v in children.items()}
        else:
            node.versions = d.get('vs', d.get('versions', {}))
        return node


class FileSystem:
    """VMS-style filesystem with versioning and protection."""
    __slots__ = ('root', 'current', 'path_stack')

    def __init__(self):
        self.root = FileNode("SYS$SYSDEVICE:[000000]", True)
        self.current = self.root
        self.path_stack = ["SYS$SYSDEVICE:[000000]"]
        self._create_initial_structure()

    def _create_initial_structure(self):
        """Create default VMS directory structure."""
        for name in ('SYSMGR', 'SYSEXE', 'SYSMAINT', 'USER', 'SCRATCH'):
            self.root.children[name] = FileNode(name, True)

        self.root.children['USER'].children['SYSTEM'] = FileNode('SYSTEM', True)

        # Welcome message
        welcome = FileNode('WELCOME.TXT', False)
        welcome.content = ("Welcome to VAX/VMS Operating System\n\n"
                          "VAX/VMS Version 5.5-2\n\n"
                          "For help, type HELP at the $ prompt.\n")
        welcome.size = len(welcome.content)
        self.root.children['SYSMGR'].children['WELCOME.TXT'] = welcome

        # Sample command procedure
        login_com = FileNode('LOGIN.COM', False)
        login_com.content = ("$! LOGIN.COM - System login procedure\n"
                            "$!\n"
                            "$ WRITE SYS$OUTPUT \"Welcome, ''F$USER()'!\"\n"
                            "$ SHOW TIME\n"
                            "$ EXIT\n")
        login_com.size = len(login_com.content)
        self.root.children['SYSMGR'].children['LOGIN.COM'] = login_com

    def get_current_path(self):
        """Return VMS-style path string."""
        if len(self.path_stack) == 1:
            return self.path_stack[0]
        return "SYS$SYSDEVICE:[" + '.'.join(self.path_stack[1:]) + "]"

    def list_files(self, pattern='*.*'):
        """List files matching pattern."""
        pattern = pattern.upper()
        if ';' in pattern:
            pattern = pattern.split(';')[0]
        if '.' not in pattern and pattern != '*':
            pattern += '.DAT'

        results = []
        for name, node in self.current.children.items():
            if pattern in ('*', '*.*') or _match_wildcard(pattern, name):
                results.append(node)
        return results

    def change_directory(self, path):
        """Change current directory."""
        path = path.upper()

        if path == '[-]':
            if len(self.path_stack) > 1:
                self.path_stack.pop()
                self.current = self._navigate_to_path(self.path_stack)
            return None

        if path.startswith('[') and path.endswith(']'):
            inner = path[1:-1]
            if inner == '000000':
                self.path_stack = ["SYS$SYSDEVICE:[000000]"]
                self.current = self.root
                return None

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
            if path in self.current.children and self.current.children[path].is_directory:
                self.path_stack.append(path)
                self.current = self.current.children[path]
            else:
                return "%SET-E-DIRNOTFOUND, directory not found"
        return None

    def _navigate_to_path(self, path):
        """Navigate to path stack location."""
        node = self.root
        for i in range(1, len(path)):
            if path[i] in node.children:
                node = node.children[path[i]]
            else:
                return self.root
        return node

    def resolve_path(self, path):
        """Resolve a VMS path to a node. Returns (node, parent, name) or (None, None, error)."""
        path = path.upper()

        # Check for device/directory prefix
        if ':' in path or path.startswith('['):
            # Full path - parse it
            if ':' in path:
                # Remove device prefix
                path = path.split(':', 1)[1] if ':' in path else path

            if path.startswith('[') and ']' in path:
                dir_part = path[1:path.index(']')]
                file_part = path[path.index(']')+1:] if ']' in path else ''

                # Navigate to directory
                if dir_part == '000000':
                    target_dir = self.root
                else:
                    parts = dir_part.split('.')
                    target_dir = self.root
                    for part in parts:
                        if part in target_dir.children and target_dir.children[part].is_directory:
                            target_dir = target_dir.children[part]
                        else:
                            return None, None, "%RMS-E-DNF, directory not found"

                if file_part:
                    if '.' not in file_part:
                        file_part += '.DAT'
                    if file_part in target_dir.children:
                        return target_dir.children[file_part], target_dir, file_part
                    return None, target_dir, file_part
                return target_dir, None, dir_part
            else:
                # Just a filename in current directory
                if '.' not in path:
                    path += '.DAT'
                if path in self.current.children:
                    return self.current.children[path], self.current, path
                return None, self.current, path
        else:
            # Simple filename
            if '.' not in path:
                path += '.DAT'
            if path in self.current.children:
                return self.current.children[path], self.current, path
            return None, self.current, path

    def create_directory(self, name):
        """Create a new directory."""
        name = name.upper()
        if name.startswith('[') and name.endswith(']'):
            name = name[1:-1].split('.')[-1]

        if name in self.current.children:
            return "%CREATE-E-EXISTS, directory already exists"
        self.current.children[name] = FileNode(name, True)
        return None

    def delete_directory(self, name):
        """Delete an empty directory."""
        name = name.upper()
        if name not in self.current.children:
            return "%DELETE-E-DIRNOTFOUND, directory not found"

        node = self.current.children[name]
        if not node.is_directory:
            return "%DELETE-E-NOTDIR, not a directory"
        if node.children:
            return "%DELETE-E-DIRNOTEMPTY, directory not empty"

        del self.current.children[name]
        return None

    def read_file(self, name, version=None):
        """Read file content, optionally specific version."""
        name = name.upper()

        # Parse version from filename
        if ';' in name:
            name, ver_str = name.rsplit(';', 1)
            try:
                version = int(ver_str)
            except ValueError:
                pass

        if '.' not in name:
            name += '.DAT'

        node, parent, fname = self.resolve_path(name)
        if node is None:
            return None, "%TYPE-E-FILENOTFOUND, file not found"
        if node.is_directory:
            return None, "%TYPE-E-ISDIR, file is a directory"

        if version is not None and version != node.version:
            if node.versions and version in node.versions:
                return node.versions[version]['content'], None
            return None, "%TYPE-E-FILENOTFOUND, version not found"

        return node.content, None

    def write_file(self, name, content):
        """Write content to file, creating new version."""
        name = name.upper()
        if '.' not in name:
            name += '.DAT'

        if name in self.current.children:
            node = self.current.children[name]
            if not node.is_directory:
                # Store old version
                if node.versions is None:
                    node.versions = {}
                node.versions[node.version] = {
                    'content': node.content,
                    'size': node.size,
                    'modified': node.modified
                }
                node.version += 1
        else:
            node = FileNode(name, False)
            self.current.children[name] = node

        if not node.is_directory:
            node.content = content
            node.size = len(content)
            node.modified = _get_timestamp()
        return None

    def delete_file(self, name, version=None):
        """Delete a file or specific version."""
        name = name.upper()

        if ';' in name:
            name, ver_str = name.rsplit(';', 1)
            try:
                version = int(ver_str)
            except ValueError:
                pass

        if '.' not in name:
            name += '.DAT'

        if name not in self.current.children:
            return "%DELETE-E-FILENOTFOUND, file not found"

        node = self.current.children[name]
        if node.is_directory:
            return "%DELETE-E-ISDIR, use DELETE/DIR for directories"

        if version is not None:
            if version == node.version:
                # Delete current version, promote older if exists
                if node.versions:
                    newest = max(node.versions.keys())
                    v = node.versions.pop(newest)
                    node.content = v['content']
                    node.size = v['size']
                    node.modified = v['modified']
                    node.version = newest
                else:
                    del self.current.children[name]
            elif node.versions and version in node.versions:
                del node.versions[version]
            else:
                return "%DELETE-E-FILENOTFOUND, version not found"
        else:
            del self.current.children[name]
        return None

    def copy_file(self, src, dest):
        """Copy a file."""
        src = src.upper()
        dest = dest.upper()
        if '.' not in src:
            src += '.DAT'
        if '.' not in dest:
            dest += '.DAT'

        if src not in self.current.children:
            return "%COPY-E-FILENOTFOUND, file not found"
        if self.current.children[src].is_directory:
            return "%COPY-E-ISDIR, cannot copy directory"

        source = self.current.children[src]
        copy = FileNode(dest, False)
        copy.content = source.content
        copy.size = source.size
        self.current.children[dest] = copy
        return None

    def rename_file(self, src, dest):
        """Rename a file."""
        src = src.upper()
        dest = dest.upper()
        if '.' not in src:
            src += '.DAT'
        if '.' not in dest:
            dest += '.DAT'

        if src not in self.current.children:
            return "%RENAME-E-FILENOTFOUND, file not found"

        node = self.current.children.pop(src)
        node.name = dest
        self.current.children[dest] = node
        return None

    def purge_files(self, pattern='*.*', keep=1):
        """Purge old versions of files."""
        purged = 0
        pattern = pattern.upper()
        if '.' not in pattern and pattern != '*':
            pattern += '.DAT'

        for name, node in self.current.children.items():
            if node.is_directory:
                continue
            if _match_wildcard(pattern, name):
                if node.versions:
                    purged += len(node.versions)
                    node.versions = {}
        return purged

    def save(self, filename):
        """Save filesystem to JSON file."""
        try:
            with open(filename, 'w') as f:
                json.dump(self.root.to_dict(), f)
            return None
        except Exception as e:
            return "%SAVE-E-ERROR, " + str(e)

    def load(self, filename):
        """Load filesystem from JSON file."""
        try:
            with open(filename, 'r') as f:
                data = json.load(f)
            self.root = FileNode.from_dict(data)
            self.current = self.root
            self.path_stack = ["SYS$SYSDEVICE:[000000]"]
            return None
        except:
            return None


class Terminal:
    """Serial terminal handler optimized for Pico."""
    __slots__ = ('running',)

    def __init__(self):
        self.running = True

    def write(self, text):
        """Write text to terminal."""
        sys.stdout.write(text.replace('\n', '\r\n'))

    def writeln(self, text=""):
        """Write line to terminal."""
        self.write(text + '\n')

    def read_line(self):
        """Read a line with editing support."""
        line = ""
        while True:
            char = sys.stdin.read(1)

            if not char or char == '\x04':  # EOF
                if not line:
                    self.writeln("LOGOUT")
                    return "LOGOUT"
                self.writeln()
                return line

            if char in ('\r', '\n'):
                self.writeln()
                return line
            elif char in ('\x7f', '\x08'):  # Backspace/Delete
                if line:
                    line = line[:-1]
                    self.write('\b \b')
            elif char == '\x03':  # Ctrl-C
                self.writeln('^C')
                return ""
            elif char == '\x15':  # Ctrl-U (clear line)
                self.write('\b \b' * len(line))
                line = ""
            elif 32 <= ord(char) < 127:
                line += char
                self.write(char)

    def clear_screen(self):
        """Clear terminal screen."""
        self.write('\x1b[2J\x1b[H')


class LexicalFunctions:
    """VMS DCL lexical functions."""

    def __init__(self, vms):
        self.vms = vms
        self.functions = {
            'F$TIME': self.f_time,
            'F$LENGTH': self.f_length,
            'F$EXTRACT': self.f_extract,
            'F$ELEMENT': self.f_element,
            'F$INTEGER': self.f_integer,
            'F$STRING': self.f_string,
            'F$LOCATE': self.f_locate,
            'F$EDIT': self.f_edit,
            'F$USER': self.f_user,
            'F$DIRECTORY': self.f_directory,
            'F$ENVIRONMENT': self.f_environment,
            'F$GETENV': self.f_getenv,
            'F$LOGICAL': self.f_logical,
            'F$SEARCH': self.f_search,
            'F$TYPE': self.f_type,
            'F$VERIFY': self.f_verify,
            'F$MESSAGE': self.f_message,
        }

    def evaluate(self, expr):
        """Evaluate expression containing lexical functions."""
        # Find and evaluate all F$ functions
        while 'F$' in expr:
            # Find function call
            match = re.search(r"F\$(\w+)\(([^)]*)\)", expr)
            if not match:
                break

            func_name = 'F$' + match.group(1).upper()
            args_str = match.group(2)

            # Parse arguments
            args = self._parse_args(args_str)

            # Call function
            if func_name in self.functions:
                try:
                    result = self.functions[func_name](args)
                except Exception:
                    result = ""
            else:
                result = ""

            # Replace in expression
            expr = expr[:match.start()] + str(result) + expr[match.end():]

        return expr

    def _parse_args(self, args_str):
        """Parse function arguments."""
        if not args_str.strip():
            return []

        args = []
        current = ""
        depth = 0
        in_string = False

        for char in args_str:
            if char == '"' and depth == 0:
                in_string = not in_string
            elif char == '(' and not in_string:
                depth += 1
                current += char
            elif char == ')' and not in_string:
                depth -= 1
                current += char
            elif char == ',' and depth == 0 and not in_string:
                args.append(current.strip().strip('"'))
                current = ""
            else:
                current += char

        if current.strip():
            args.append(current.strip().strip('"'))

        return args

    def f_time(self, args):
        """Return current time."""
        return _get_timestamp()

    def f_length(self, args):
        """Return string length."""
        if args:
            return len(args[0])
        return 0

    def f_extract(self, args):
        """Extract substring: F$EXTRACT(start, length, string)"""
        if len(args) >= 3:
            try:
                start = int(args[0])
                length = int(args[1])
                return args[2][start:start+length]
            except:
                pass
        return ""

    def f_element(self, args):
        """Get element from delimited string: F$ELEMENT(index, delimiter, string)"""
        if len(args) >= 3:
            try:
                index = int(args[0])
                delimiter = args[1]
                parts = args[2].split(delimiter)
                if 0 <= index < len(parts):
                    return parts[index]
            except:
                pass
        return ""

    def f_integer(self, args):
        """Convert to integer."""
        if args:
            try:
                return int(args[0])
            except:
                pass
        return 0

    def f_string(self, args):
        """Convert to string."""
        if args:
            return str(args[0])
        return ""

    def f_locate(self, args):
        """Find substring: F$LOCATE(substring, string)"""
        if len(args) >= 2:
            pos = args[1].find(args[0])
            return pos if pos >= 0 else len(args[1])
        return 0

    def f_edit(self, args):
        """Edit string: F$EDIT(string, edit_list)"""
        if len(args) >= 2:
            s = args[0]
            edits = args[1].upper()
            if 'UPCASE' in edits:
                s = s.upper()
            if 'LOWERCASE' in edits:
                s = s.lower()
            if 'TRIM' in edits:
                s = s.strip()
            if 'COMPRESS' in edits:
                s = ' '.join(s.split())
            if 'COLLAPSE' in edits:
                s = s.replace(' ', '')
            return s
        return args[0] if args else ""

    def f_user(self, args):
        """Return current username."""
        return self.vms.current_user

    def f_directory(self, args):
        """Return current directory."""
        return self.vms.fs.get_current_path()

    def f_environment(self, args):
        """Return environment info."""
        if args:
            item = args[0].upper()
            if item == 'DEFAULT':
                return self.vms.fs.get_current_path()
            if item == 'PROCEDURE':
                return self.vms.current_procedure or ""
        return ""

    def f_getenv(self, args):
        """Get environment variable (alias for logical)."""
        return self.f_logical(args)

    def f_logical(self, args):
        """Translate logical name."""
        if args:
            name = args[0].upper()
            return self.vms.symbols.get(name, "")
        return ""

    def f_search(self, args):
        """Search for file: F$SEARCH(filespec)"""
        if args:
            files = self.vms.fs.list_files(args[0])
            if files:
                return self.vms.fs.get_current_path() + files[0].name
        return ""

    def f_type(self, args):
        """Return symbol type."""
        if args:
            name = args[0].upper()
            if name in self.vms.symbols:
                return "STRING"
        return ""

    def f_verify(self, args):
        """Return verify status."""
        return "1" if self.vms.verify_mode else "0"

    def f_message(self, args):
        """Return message text for status code."""
        return "%SYSTEM-S-NORMAL, normal successful completion"


class VAXVMS:
    """Main VAX/VMS emulator class."""

    # Valid DCL commands
    COMMANDS = (
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
    )

    def __init__(self):
        self.fs = FileSystem()
        self.terminal = Terminal()
        self.lexical = LexicalFunctions(self)
        self.current_user = "SYSTEM"
        self.node = "PICO"
        self.running = True
        self.save_file = "vaxvms_state.json"
        self.symbols = {}  # DCL symbols and logical names
        self.verify_mode = False
        self.current_procedure = None
        self.procedure_stack = []
        self.open_files = {}  # For OPEN/READ/WRITE/CLOSE

        self.fs.load(self.save_file)

    def _match_command(self, cmd, commands=None):
        """Match abbreviated command to full name."""
        if commands is None:
            commands = self.COMMANDS
        cmd = cmd.upper()
        matches = [c for c in commands if c.startswith(cmd)]

        if len(matches) == 1:
            return matches[0]
        if cmd in matches:
            return cmd
        return None

    def _parse_line(self, line):
        """Parse DCL command line into parts and qualifiers."""
        parts = []
        qualifiers = {}
        current = ""
        in_string = False

        for char in line:
            if char == '"':
                in_string = not in_string
                current += char
            elif char == ' ' and not in_string:
                if current:
                    if current.startswith('/'):
                        qual = current[1:].upper()
                        if '=' in qual:
                            k, v = qual.split('=', 1)
                            qualifiers[k] = v.strip('"')
                        else:
                            qualifiers[qual] = True
                    else:
                        parts.append(current.strip('"'))
                    current = ""
            else:
                current += char

        if current:
            if current.startswith('/'):
                qual = current[1:].upper()
                if '=' in qual:
                    k, v = qual.split('=', 1)
                    qualifiers[k] = v.strip('"')
                else:
                    qualifiers[qual] = True
            else:
                parts.append(current.strip('"'))

        return parts, qualifiers

    def _substitute_symbols(self, line):
        """Substitute DCL symbols and lexical functions."""
        # First evaluate lexical functions (they need to be done before symbol substitution)
        line = self.lexical.evaluate(line)

        # Symbol substitution (double single-quotes ''name'')
        # This is VMS-style - ''symbol'' means substitute the value
        for name, value in self.symbols.items():
            line = line.replace("''" + name + "''", str(value))

        # Also handle 'symbol' (less common but used)
        for name, value in self.symbols.items():
            line = line.replace("'" + name + "'", str(value))

        return line

    def _parse_protection(self, prot_str):
        """Parse VMS protection string like (S:RWED,O:RWE,G:RE,W:R)"""
        prot_str = prot_str.strip('()')
        result = {}

        for part in prot_str.split(','):
            if ':' in part:
                cat, perms = part.split(':', 1)
                result[cat.upper()] = perms.upper()
            else:
                # Just category with no perms
                result[part.upper()] = ''

        # Build normalized protection string
        cats = ['S', 'O', 'G', 'W']
        parts = []
        for c in cats:
            parts.append("{}:{}".format(c, result.get(c, '')))

        return "(" + ','.join(parts) + ")"

    def boot(self):
        """Boot the VMS system."""
        self.terminal.clear_screen()
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
            self.terminal.writeln("    Memory: {} bytes free of {}".format(free, total))
        except:
            pass

        self.terminal.writeln("")
        self.terminal.writeln(" (c) Copyright 1991 Digital Equipment Corporation.")
        self.terminal.writeln("")

        content, _ = self.fs.read_file('[SYSMGR]WELCOME.TXT')
        if content:
            self.terminal.writeln(content)

        self.terminal.writeln("")
        self.terminal.writeln("Username: " + self.current_user)
        self.terminal.writeln("")
        self.terminal.writeln("%LOGIN-I-LOGGEDIN, logged in as " + self.current_user)
        self.terminal.writeln("        on {}::TXA0: at {}".format(self.node, _get_timestamp()))
        self.terminal.writeln("")

        # Run login.com if it exists
        self._execute_procedure('[SYSMGR]LOGIN.COM', silent=True)

        while self.running:
            self.terminal.write("$ ")
            cmd_line = self.terminal.read_line()
            if cmd_line:
                self.execute_command(cmd_line)

    def execute_command(self, cmd_line):
        """Execute a DCL command."""
        # Handle @ command procedure
        if cmd_line.startswith('@'):
            self._execute_procedure(cmd_line[1:].strip())
            return

        # Symbol substitution
        cmd_line = self._substitute_symbols(cmd_line)

        parts, qualifiers = self._parse_line(cmd_line)
        if not parts:
            return

        cmd = parts[0].upper()
        args = parts[1:]

        # Handle command/qualifier (e.g., CREATE/DIR)
        if '/' in cmd:
            cmd_part, qual_part = cmd.split('/', 1)
            cmd = cmd_part
            # Add the qualifier
            if '=' in qual_part:
                k, v = qual_part.split('=', 1)
                qualifiers[k] = v.strip('"')
            else:
                qualifiers[qual_part] = True

        # Handle DIR alias
        if cmd == 'DIR':
            cmd = 'DIRECTORY'

        matched = self._match_command(cmd)

        if not matched:
            self.terminal.writeln("%DCL-W-IVVERB, unrecognized command verb - check validity and spelling")
            self.terminal.writeln("\\{}\\".format(cmd))
            return

        # Get handler method
        handler = getattr(self, 'cmd_' + matched.lower(), None)
        if handler:
            handler(args, qualifiers)
        else:
            self.terminal.writeln("%DCL-W-NOTINSTALLED, command not installed")
            self.terminal.writeln("\\{}\\".format(matched))

    def _execute_procedure(self, filename, silent=False):
        """Execute a .COM command procedure."""
        content, err = self.fs.read_file(filename)
        if err:
            if not silent:
                self.terminal.writeln(err)
            return

        old_procedure = self.current_procedure
        self.current_procedure = filename.upper()
        self.procedure_stack.append(filename.upper())

        labels = {}  # For GOTO support
        lines = content.split('\n')

        # First pass: find labels
        for i, line in enumerate(lines):
            line = line.strip()
            if line.startswith('$') and ':' in line and not line.startswith('$!'):
                label = line[1:line.index(':')].strip().upper()
                labels[label] = i

        # Second pass: execute
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            i += 1

            if not line or line.startswith('$!'):
                continue

            if line.startswith('$'):
                line = line[1:].strip()

                # Check for label
                if ':' in line and not line.startswith('!'):
                    # Skip label definition
                    colon_pos = line.index(':')
                    line = line[colon_pos+1:].strip()
                    if not line:
                        continue

                if self.verify_mode and not silent:
                    self.terminal.writeln("$ " + line)

                # Handle GOTO
                if line.upper().startswith('GOTO '):
                    target = line[5:].strip().upper()
                    if target in labels:
                        i = labels[target]
                    continue

                # Handle EXIT
                if line.upper() == 'EXIT':
                    break

                # Handle IF...THEN
                if line.upper().startswith('IF '):
                    # Simple IF handling
                    match = re.match(r'IF\s+(.+?)\s+THEN\s+(.+)', line, re.IGNORECASE)
                    if match:
                        condition = match.group(1)
                        action = match.group(2)
                        # Evaluate simple conditions
                        condition = self._substitute_symbols(condition)
                        # Very basic evaluation
                        try:
                            if eval(condition.replace('.EQS.', '==').replace('.NES.', '!=')):
                                self.execute_command(action)
                        except:
                            pass
                    continue

                self.execute_command(line)

        self.procedure_stack.pop()
        self.current_procedure = old_procedure

    # ==================== DCL Commands ====================

    def cmd_directory(self, args, quals):
        """DIRECTORY command."""
        pattern = args[0] if args else '*.*'
        files = self.fs.list_files(pattern)

        if not files:
            self.terminal.writeln("")
            self.terminal.writeln("%DIRECT-W-NOFILES, no files found")
            return

        self.terminal.writeln("")
        self.terminal.writeln("Directory " + self.fs.get_current_path())
        self.terminal.writeln("")

        total_size = 0

        if quals.get('FULL'):
            for f in files:
                self.terminal.writeln("")
                if f.is_directory:
                    self.terminal.writeln("{}.DIR;1".format(f.name))
                else:
                    self.terminal.writeln("{};{}".format(f.name, f.version))

                self.terminal.writeln("  Size:            {:>8}".format(f.size if not f.is_directory else ""))
                self.terminal.writeln("  Owner:           [{}]".format(f.uic))
                self.terminal.writeln("  Created:         {}".format(f.modified))
                self.terminal.writeln("  Revised:         {} ({})".format(f.modified, f.version))
                self.terminal.writeln("  File protection: {}".format(f.protection))
                if f.record_format:
                    self.terminal.writeln("  Record format:   {}".format(f.record_format))
                total_size += f.size
        else:
            for f in files:
                if f.is_directory:
                    self.terminal.writeln("{}.DIR;1".format(f.name))
                else:
                    self.terminal.writeln("{};{}    {:>8}  {}".format(
                        f.name, f.version, f.size, f.modified))
                    total_size += f.size

                    if quals.get('VERSIONS') and f.versions:
                        for ver in sorted(f.versions.keys(), reverse=True):
                            v = f.versions[ver]
                            self.terminal.writeln("{};{}    {:>8}  {}".format(
                                f.name, ver, v['size'], v['modified']))

        self.terminal.writeln("")
        self.terminal.writeln("Total of {} file{}, {} bytes".format(
            len(files), 's' if len(files) != 1 else '', total_size))

    def cmd_set(self, args, quals):
        """SET command."""
        if not args:
            self.terminal.writeln("%SET-E-TOOFEW, too few arguments")
            return

        subcmd = self._match_command(args[0], ('DEFAULT', 'TERMINAL', 'PROCESS',
                                                'PROMPT', 'VERIFY', 'FILE', 'PROTECTION'))
        if not subcmd:
            self.terminal.writeln("%SET-W-NOTSET, item not set")
            return

        if subcmd == 'DEFAULT':
            if len(args) > 1:
                err = self.fs.change_directory(args[1])
                if err:
                    self.terminal.writeln(err)
            else:
                self.terminal.writeln(self.fs.get_current_path())

        elif subcmd == 'VERIFY':
            self.verify_mode = not self.verify_mode
            self.terminal.writeln("%SET-I-VERIFY, verify mode {}".format(
                "enabled" if self.verify_mode else "disabled"))

        elif subcmd == 'FILE':
            # SET FILE filename /PROTECTION=(...)
            filename = None
            for a in args[1:]:
                if not a.startswith('/'):
                    filename = a.upper()
                    break

            if not filename:
                self.terminal.writeln("%SET-E-NOFILE, no file specified")
                return

            if 'PROTECTION' not in quals:
                self.terminal.writeln("%SET-E-INVQUAL, /PROTECTION qualifier required")
                return

            if '.' not in filename:
                filename += '.DAT'

            if filename in self.fs.current.children:
                prot = self._parse_protection(quals['PROTECTION'])
                self.fs.current.children[filename].protection = prot
                self.terminal.writeln("%SET-I-MODIFIED, file protection modified")
            else:
                self.terminal.writeln("%SET-E-FILENOTFOUND, file not found")

        elif subcmd == 'PROTECTION':
            # SET PROTECTION=(prot)/DEFAULT
            if 'DEFAULT' in quals:
                self.terminal.writeln("%SET-I-DEFPROT, default protection set")

        elif subcmd == 'TERMINAL':
            self.terminal.writeln("%SET-I-TERMINAL, terminal characteristics set")

        elif subcmd == 'PROCESS':
            self.terminal.writeln("%SET-I-PROCESS, process parameters set")

    def cmd_show(self, args, quals):
        """SHOW command."""
        if not args:
            self.terminal.writeln("%SHOW-E-TOOFEW, too few arguments")
            return

        subcmd = self._match_command(args[0], ('DEFAULT', 'TIME', 'SYSTEM', 'USERS',
                                                'MEMORY', 'DEVICES', 'SYMBOL', 'LOGICAL',
                                                'PROCESS', 'QUOTA', 'STATUS', 'PROTECTION'))
        if not subcmd:
            self.terminal.writeln("%SHOW-W-NOTFOUND, item not found")
            return

        if subcmd == 'DEFAULT':
            self.terminal.writeln("  " + self.fs.get_current_path())

        elif subcmd == 'TIME':
            self.terminal.writeln("  " + _get_timestamp())

        elif subcmd == 'SYSTEM':
            self.terminal.writeln("  VAX/VMS V5.5-2 on node {}".format(self.node))
            self.terminal.writeln("  Uptime: {} seconds".format(time.time()))

        elif subcmd == 'USERS':
            self.terminal.writeln("      VAX/VMS Interactive Users")
            self.terminal.writeln("")
            self.terminal.writeln("  Username  Process Name      PID     Terminal")
            self.terminal.writeln("  {}  {:16}  {:08X}  TXA0:".format(
                self.current_user.ljust(8), "SYSTEM", 0x42))

        elif subcmd == 'MEMORY':
            try:
                import gc
                gc.collect()
                free = gc.mem_free()
                total = gc.mem_alloc() + free
                used = gc.mem_alloc()
                self.terminal.writeln("  System Memory Resources on {}".format(_get_timestamp()))
                self.terminal.writeln("")
                self.terminal.writeln("  Physical Memory:     {:>10} bytes".format(total))
                self.terminal.writeln("  Free Memory:         {:>10} bytes".format(free))
                self.terminal.writeln("  In Use:              {:>10} bytes".format(used))
                self.terminal.writeln("  Utilization:         {:>10.1f}%".format(used*100/total))
            except:
                self.terminal.writeln("  Memory information unavailable")

        elif subcmd == 'DEVICES':
            self.terminal.writeln("  Device          Device           Error")
            self.terminal.writeln("  Name            Status           Count")
            self.terminal.writeln("  TXA0:           Online               0")
            self.terminal.writeln("  SYS$SYSDEVICE:  Mounted              0")

        elif subcmd in ('SYMBOL', 'LOGICAL'):
            if len(args) > 1:
                name = args[1].upper()
                if name in self.symbols:
                    self.terminal.writeln('  {} = "{}"'.format(name, self.symbols[name]))
                else:
                    self.terminal.writeln("%SHOW-W-NOTFOUND, {} not found".format(name))
            else:
                if self.symbols:
                    for name, value in sorted(self.symbols.items()):
                        self.terminal.writeln('  {} = "{}"'.format(name, value))
                else:
                    self.terminal.writeln("  No symbols defined")

        elif subcmd == 'PROCESS':
            self.terminal.writeln("  {} at {}".format(_get_timestamp(), self.node))
            self.terminal.writeln("  Pid: 00000042  Proc. name: {}".format(self.current_user))
            self.terminal.writeln("  Priority: 4    Default file spec: {}".format(
                self.fs.get_current_path()))

        elif subcmd == 'QUOTA':
            self.terminal.writeln("  User [{}] has {}".format(self.current_user, "unlimited quota"))

        elif subcmd == 'STATUS':
            self.terminal.writeln("  Status: %X00000001")
            self.terminal.writeln("  Message: %SYSTEM-S-NORMAL, normal successful completion")

        elif subcmd == 'PROTECTION':
            self.terminal.writeln("  System default protection: {}".format(_FILE_PROT))

    def cmd_create(self, args, quals):
        """CREATE command."""
        if 'DIRECTORY' in quals or 'DIR' in quals:
            # CREATE/DIRECTORY name
            if not args:
                self.terminal.writeln("%CREATE-E-NOSPEC, no directory specified")
                return
            name = args[0]
            err = self.fs.create_directory(name)
            if err:
                self.terminal.writeln(err)
            else:
                self.terminal.writeln("%CREATE-I-CREATED, {}.DIR;1 created".format(name.upper()))
        else:
            # Create file
            name = args[0].upper()
            if '.' not in name:
                name += '.DAT'

            if name not in self.fs.current.children:
                self.fs.current.children[name] = FileNode(name, False)

            self.terminal.writeln("%CREATE-I-CREATED, {} created".format(name))
            self.terminal.writeln("Enter text (Ctrl-Z or empty line to finish):")

            lines = []
            while True:
                line = self.terminal.read_line()
                if not line:
                    break
                lines.append(line)

            if lines:
                self.fs.write_file(name, '\n'.join(lines))

    def cmd_delete(self, args, quals):
        """DELETE command."""
        if not args:
            self.terminal.writeln("%DELETE-E-TOOFEW, too few arguments")
            return

        name = args[0]
        if name.upper().endswith('.DIR') or 'DIRECTORY' in quals:
            name = name.upper().replace('.DIR', '')
            err = self.fs.delete_directory(name)
        else:
            err = self.fs.delete_file(name)

        if err:
            self.terminal.writeln(err)

    def cmd_type(self, args, quals):
        """TYPE command."""
        if not args:
            self.terminal.writeln("%TYPE-E-TOOFEW, too few arguments")
            return

        content, err = self.fs.read_file(args[0])
        if err:
            self.terminal.writeln(err)
        else:
            self.terminal.writeln("")
            if content:
                # Handle /PAGE qualifier
                if quals.get('PAGE'):
                    lines = content.split('\n')
                    for i, line in enumerate(lines):
                        self.terminal.writeln(line)
                        if (i + 1) % 20 == 0:
                            self.terminal.write("Press RETURN to continue...")
                            self.terminal.read_line()
                else:
                    self.terminal.writeln(content)

    def cmd_copy(self, args, quals):
        """COPY command."""
        if len(args) < 2:
            self.terminal.writeln("%COPY-E-TOOFEW, too few arguments")
            return
        err = self.fs.copy_file(args[0], args[1])
        if err:
            self.terminal.writeln(err)

    def cmd_rename(self, args, quals):
        """RENAME command."""
        if len(args) < 2:
            self.terminal.writeln("%RENAME-E-TOOFEW, too few arguments")
            return
        err = self.fs.rename_file(args[0], args[1])
        if err:
            self.terminal.writeln(err)

    def cmd_search(self, args, quals):
        """SEARCH command."""
        if len(args) < 2:
            self.terminal.writeln("%SEARCH-E-TOOFEW, too few arguments")
            return

        filename = args[0]
        pattern = args[1]
        content, err = self.fs.read_file(filename)

        if err:
            self.terminal.writeln(err)
            return

        found = False
        for i, line in enumerate(content.split('\n'), 1):
            if pattern.upper() in line.upper():
                self.terminal.writeln("{} Line {}: {}".format(filename.upper(), i, line))
                found = True

        if not found:
            self.terminal.writeln("%SEARCH-I-NOMATCHES, no matches found")

    def cmd_purge(self, args, quals):
        """PURGE command."""
        pattern = args[0] if args else '*.*'
        purged = self.fs.purge_files(pattern)

        if purged > 0:
            self.terminal.writeln("%PURGE-I-FILPURG, {} version{} purged".format(
                purged, 's' if purged != 1 else ''))
        else:
            self.terminal.writeln("%PURGE-I-FILPURG, no old versions found")

    def cmd_print(self, args, quals):
        """PRINT command."""
        if not args:
            self.terminal.writeln("%PRINT-E-TOOFEW, too few arguments")
            return
        self.terminal.writeln("%PRINT-I-QUEUED, {} queued to SYS$PRINT".format(args[0].upper()))

    def cmd_assign(self, args, quals):
        """ASSIGN logical-name equivalence-name."""
        if len(args) < 2:
            self.terminal.writeln("%ASSIGN-E-TOOFEW, too few arguments")
            return

        equiv = args[0].upper()
        logical = args[1].upper()
        self.symbols[logical] = equiv
        self.terminal.writeln("%ASSIGN-I-ASSIGNED, {} assigned to {}".format(logical, equiv))

    def cmd_deassign(self, args, quals):
        """DEASSIGN logical-name."""
        if not args:
            self.terminal.writeln("%DEASSIGN-E-TOOFEW, too few arguments")
            return

        logical = args[0].upper()
        if logical in self.symbols:
            del self.symbols[logical]
        else:
            self.terminal.writeln("%DEASSIGN-W-NOTLOG, {} is not a logical name".format(logical))

    def cmd_define(self, args, quals):
        """DEFINE logical-name equivalence-name (same as ASSIGN but different syntax)."""
        if len(args) < 2:
            self.terminal.writeln("%DEFINE-E-TOOFEW, too few arguments")
            return

        logical = args[0].upper()
        equiv = args[1].upper()
        self.symbols[logical] = equiv
        self.terminal.writeln("%DEFINE-I-DEFINED, {} defined as {}".format(logical, equiv))

    def cmd_append(self, args, quals):
        """APPEND file1 file2."""
        if len(args) < 2:
            self.terminal.writeln("%APPEND-E-TOOFEW, too few arguments")
            return

        content1, err1 = self.fs.read_file(args[0])
        if err1:
            self.terminal.writeln(err1)
            return

        content2, _ = self.fs.read_file(args[1])
        content2 = content2 or ""

        self.fs.write_file(args[1], content2 + content1)
        self.terminal.writeln("%APPEND-I-APPENDED, {} appended to {}".format(
            args[0].upper(), args[1].upper()))

    def cmd_differences(self, args, quals):
        """DIFFERENCES file1 file2."""
        if len(args) < 2:
            self.terminal.writeln("%DIFFERENCES-E-TOOFEW, too few arguments")
            return

        c1, e1 = self.fs.read_file(args[0])
        if e1:
            self.terminal.writeln(e1)
            return

        c2, e2 = self.fs.read_file(args[1])
        if e2:
            self.terminal.writeln(e2)
            return

        lines1 = c1.split('\n')
        lines2 = c2.split('\n')
        diffs = 0

        self.terminal.writeln("")
        for i, (l1, l2) in enumerate(zip(lines1, lines2), 1):
            if l1 != l2:
                self.terminal.writeln("************")
                self.terminal.writeln("File 1 ({}), line {}:".format(args[0].upper(), i))
                self.terminal.writeln("  " + l1)
                self.terminal.writeln("File 2 ({}), line {}:".format(args[1].upper(), i))
                self.terminal.writeln("  " + l2)
                diffs += 1

        # Handle different lengths
        if len(lines1) != len(lines2):
            self.terminal.writeln("************")
            self.terminal.writeln("Files have different number of lines ({} vs {})".format(
                len(lines1), len(lines2)))
            diffs += abs(len(lines1) - len(lines2))

        self.terminal.writeln("")
        self.terminal.writeln("Number of difference sections found: {}".format(diffs))

    def cmd_dump(self, args, quals):
        """DUMP file - hex dump."""
        if not args:
            self.terminal.writeln("%DUMP-E-TOOFEW, too few arguments")
            return

        content, err = self.fs.read_file(args[0])
        if err:
            self.terminal.writeln(err)
            return

        self.terminal.writeln("")
        self.terminal.writeln("Dump of file {}".format(args[0].upper()))
        self.terminal.writeln("")

        for i in range(0, len(content), 16):
            chunk = content[i:i+16]
            hex_part = ' '.join('{:02X}'.format(ord(c)) for c in chunk)
            ascii_part = ''.join(c if 32 <= ord(c) < 127 else '.' for c in chunk)
            self.terminal.writeln("{:08X}  {:48s}  {}".format(i, hex_part, ascii_part))

    def cmd_sort(self, args, quals):
        """SORT input output."""
        if len(args) < 2:
            self.terminal.writeln("%SORT-E-TOOFEW, too few arguments")
            return

        content, err = self.fs.read_file(args[0])
        if err:
            self.terminal.writeln(err)
            return

        lines = content.split('\n')
        lines.sort()

        self.fs.write_file(args[1], '\n'.join(lines))
        self.terminal.writeln("%SORT-I-SORTED, {} records sorted to {}".format(
            len(lines), args[1].upper()))

    def cmd_backup(self, args, quals):
        """BACKUP source dest."""
        if len(args) < 2:
            self.terminal.writeln("%BACKUP-E-TOOFEW, too few arguments")
            return

        content, err = self.fs.read_file(args[0])
        if err:
            self.terminal.writeln(err)
            return

        dest = args[1]
        if '.' not in dest:
            dest += '.BCK'

        self.fs.write_file(dest, content)
        self.terminal.writeln("%BACKUP-S-COPIED, {} copied to {}".format(
            args[0].upper(), dest.upper()))

    def cmd_edit(self, args, quals):
        """EDT-style editor."""
        if not args:
            self.terminal.writeln("%EDIT-E-TOOFEW, too few arguments")
            return

        filename = args[0]
        content, _ = self.fs.read_file(filename)
        buffer_lines = content.split('\n') if content else []

        self.terminal.writeln("")
        self.terminal.writeln("EDT Editor - {} lines in buffer".format(len(buffer_lines)))
        self.terminal.writeln("Commands: I(nsert) L(ist) D(elete) R(eplace) F(ind) S/old/new/ E(xit) Q(uit)")
        self.terminal.writeln("")

        while True:
            self.terminal.write("*")
            cmd_input = self.terminal.read_line().strip()
            if not cmd_input:
                continue

            cmd = cmd_input[0].upper()
            arg = cmd_input[1:].strip()

            if cmd == 'I':
                self.terminal.writeln("Enter text (. on line to finish):")
                while True:
                    line = self.terminal.read_line()
                    if line == '.':
                        break
                    buffer_lines.append(line)

            elif cmd == 'L':
                self.terminal.writeln("")
                for i, line in enumerate(buffer_lines, 1):
                    self.terminal.writeln("{:4d}  {}".format(i, line))
                self.terminal.writeln("")

            elif cmd == 'D':
                try:
                    n = int(arg) - 1
                    if 0 <= n < len(buffer_lines):
                        del buffer_lines[n]
                        self.terminal.writeln("%EDIT-I-DELETED, line deleted")
                    else:
                        self.terminal.writeln("%EDIT-E-INVLINE, invalid line number")
                except:
                    self.terminal.writeln("%EDIT-E-INVNUM, specify line number")

            elif cmd == 'R':
                try:
                    n = int(arg) - 1
                    if 0 <= n < len(buffer_lines):
                        self.terminal.writeln("Current: {}".format(buffer_lines[n]))
                        self.terminal.write("New: ")
                        buffer_lines[n] = self.terminal.read_line()
                        self.terminal.writeln("%EDIT-I-REPLACED")
                    else:
                        self.terminal.writeln("%EDIT-E-INVLINE, invalid line number")
                except:
                    self.terminal.writeln("%EDIT-E-INVNUM, specify line number")

            elif cmd == 'F':
                found = False
                for i, line in enumerate(buffer_lines, 1):
                    if arg.upper() in line.upper():
                        self.terminal.writeln("{:4d}  {}".format(i, line))
                        found = True
                if not found:
                    self.terminal.writeln("%EDIT-W-NOTFOUND, text not found")

            elif cmd == 'S' and '/' in cmd_input:
                parts = cmd_input.split('/')
                if len(parts) >= 3:
                    old, new = parts[1], parts[2]
                    count = 0
                    for i in range(len(buffer_lines)):
                        if old in buffer_lines[i]:
                            buffer_lines[i] = buffer_lines[i].replace(old, new)
                            count += 1
                    self.terminal.writeln("%EDIT-I-SUBSTITUTED, {} replacement{}".format(
                        count, 's' if count != 1 else ''))
                else:
                    self.terminal.writeln("%EDIT-E-INVFORMAT, use S/old/new/")

            elif cmd == 'E':
                self.fs.write_file(filename, '\n'.join(buffer_lines))
                self.terminal.writeln("%EDIT-I-SAVED, {} saved".format(filename.upper()))
                break

            elif cmd == 'Q':
                break

            else:
                self.terminal.writeln("?Unknown command")

    def cmd_open(self, args, quals):
        """OPEN channel filename."""
        if len(args) < 2:
            self.terminal.writeln("%OPEN-E-TOOFEW, too few arguments")
            return

        channel = args[0].upper()
        filename = args[1]

        content, err = self.fs.read_file(filename)
        if 'WRITE' in quals:
            content = "" if err else content
            self.open_files[channel] = {'name': filename, 'content': [], 'mode': 'write'}
        else:
            if err:
                self.terminal.writeln(err)
                return
            self.open_files[channel] = {'name': filename, 'lines': content.split('\n'),
                                        'pos': 0, 'mode': 'read'}

    def cmd_close(self, args, quals):
        """CLOSE channel."""
        if not args:
            self.terminal.writeln("%CLOSE-E-TOOFEW, too few arguments")
            return

        channel = args[0].upper()
        if channel not in self.open_files:
            self.terminal.writeln("%CLOSE-E-NOTOPEN, channel not open")
            return

        finfo = self.open_files[channel]
        if finfo['mode'] == 'write':
            self.fs.write_file(finfo['name'], '\n'.join(finfo['content']))

        del self.open_files[channel]

    def cmd_read(self, args, quals):
        """READ channel symbol."""
        if len(args) < 2:
            self.terminal.writeln("%READ-E-TOOFEW, too few arguments")
            return

        channel = args[0].upper()
        symbol = args[1].upper()

        if channel not in self.open_files:
            self.terminal.writeln("%READ-E-NOTOPEN, channel not open")
            return

        finfo = self.open_files[channel]
        if finfo['pos'] < len(finfo['lines']):
            self.symbols[symbol] = finfo['lines'][finfo['pos']]
            finfo['pos'] += 1
        else:
            self.terminal.writeln("%READ-E-EOF, end of file")

    def cmd_write(self, args, quals):
        """WRITE channel text."""
        if len(args) < 2:
            self.terminal.writeln("%WRITE-E-TOOFEW, too few arguments")
            return

        channel = args[0].upper()
        text = ' '.join(args[1:])
        text = self._substitute_symbols(text)

        # Handle SYS$OUTPUT specially - it's always available
        if channel in ('SYS$OUTPUT', 'SYS$ERROR'):
            self.terminal.writeln(text)
        elif channel in self.open_files:
            self.open_files[channel]['content'].append(text)
        else:
            self.terminal.writeln("%WRITE-E-NOTOPEN, channel not open")

    def cmd_run(self, args, quals):
        """RUN command."""
        if not args:
            self.terminal.writeln("%RUN-E-TOOFEW, too few arguments")
            return
        self.terminal.writeln("%RUN-I-PROC, process {} created".format(args[0].upper()))

    def cmd_submit(self, args, quals):
        """SUBMIT command."""
        if not args:
            self.terminal.writeln("%SUBMIT-E-TOOFEW, too few arguments")
            return
        self.terminal.writeln("%SUBMIT-I-QUEUED, {} queued to SYS$BATCH".format(args[0].upper()))

    def cmd_mail(self, args, quals):
        """MAIL utility stub."""
        self.terminal.writeln("VAX/VMS MAIL V5.5")
        self.terminal.writeln("You have no new messages.")
        self.terminal.writeln("MAIL> (Type EXIT to return)")

    def cmd_phone(self, args, quals):
        """PHONE utility stub."""
        self.terminal.writeln("VAX/VMS Phone Facility V5.5")
        self.terminal.writeln("%PHONE-I-NOUSER, no other users available")

    def cmd_spawn(self, args, quals):
        """SPAWN subprocess."""
        self.terminal.writeln("%SPAWN-I-SPAWNED, subprocess created")
        self.terminal.writeln("%SPAWN-I-RETURNED, control returned to parent")

    def cmd_mount(self, args, quals):
        """MOUNT device."""
        if not args:
            self.terminal.writeln("%MOUNT-E-TOOFEW, too few arguments")
            return
        device = args[0].upper()
        volume = args[1].upper() if len(args) > 1 else "VOLUME1"
        self.terminal.writeln("%MOUNT-I-MOUNTED, {} mounted on {}".format(volume, device))

    def cmd_dismount(self, args, quals):
        """DISMOUNT device."""
        if not args:
            self.terminal.writeln("%DISMOUNT-E-TOOFEW, too few arguments")
            return
        self.terminal.writeln("%DISMOUNT-I-DISMOUNTED, {} dismounted".format(args[0].upper()))

    def cmd_initialize(self, args, quals):
        """INITIALIZE device."""
        if len(args) < 2:
            self.terminal.writeln("%INITIALIZE-E-TOOFEW, too few arguments")
            return
        self.terminal.writeln("%INITIALIZE-I-INIT, {} initialized as {}".format(
            args[0].upper(), args[1].upper()))

    def cmd_analyze(self, args, quals):
        """ANALYZE command stub."""
        if not args:
            self.terminal.writeln("%ANALYZE-E-TOOFEW, too few arguments")
            return
        self.terminal.writeln("%ANALYZE-I-COMPLETE, analysis of {} complete".format(args[0].upper()))

    def cmd_help(self, args, quals):
        """HELP command."""
        if args:
            topic = args[0].upper()
            help_topics = {
                'DIRECTORY': "DIRECTORY [filespec] [/FULL] [/VERSIONS]\n  Lists files in current directory",
                'SET': "SET DEFAULT dir - Change directory\nSET FILE file /PROTECTION=(...) - Set protection\nSET VERIFY - Toggle command echo",
                'SHOW': "SHOW DEFAULT|TIME|SYSTEM|USERS|MEMORY|DEVICES|SYMBOL|PROCESS|QUOTA",
                'CREATE': "CREATE filename - Create and edit file\nCREATE/DIRECTORY name - Create directory",
                'DELETE': "DELETE filename - Delete file\nDELETE name.DIR - Delete directory",
                'TYPE': "TYPE filename [/PAGE] - Display file contents",
                'EDIT': "EDIT filename - EDT editor (I L D R F S E Q commands)",
                'LEXICAL': "F$TIME() F$LENGTH(s) F$EXTRACT(start,len,s) F$ELEMENT(n,delim,s)\nF$USER() F$DIRECTORY() F$LOGICAL(name) F$SEARCH(spec)",
                '@': "@ command executes a .COM procedure file",
            }
            if topic in help_topics:
                self.terminal.writeln("")
                self.terminal.writeln(help_topics[topic])
                self.terminal.writeln("")
                return

        self.terminal.writeln("")
        self.terminal.writeln("VAX/VMS DCL Help - Type HELP topic for details")
        self.terminal.writeln("")
        self.terminal.writeln("File Commands:     DIRECTORY TYPE COPY RENAME DELETE CREATE PURGE")
        self.terminal.writeln("                   APPEND DIFFERENCES DUMP SORT BACKUP SEARCH")
        self.terminal.writeln("System Commands:   SET SHOW ASSIGN DEASSIGN DEFINE")
        self.terminal.writeln("                   MOUNT DISMOUNT INITIALIZE")
        self.terminal.writeln("Utilities:         EDIT MAIL PHONE RUN SUBMIT SPAWN")
        self.terminal.writeln("File I/O:          OPEN CLOSE READ WRITE")
        self.terminal.writeln("Procedures:        @ (execute .COM file)")
        self.terminal.writeln("Session:           LOGOUT EXIT HELP")
        self.terminal.writeln("")
        self.terminal.writeln("Lexical Functions: F$TIME F$LENGTH F$USER F$DIRECTORY ...")
        self.terminal.writeln("                   Type HELP LEXICAL for full list")
        self.terminal.writeln("")
        self.terminal.writeln("Commands can be abbreviated (DIR for DIRECTORY)")
        self.terminal.writeln("")

    def cmd_logout(self, args, quals):
        """LOGOUT/EXIT command."""
        self.terminal.writeln("")
        self.terminal.writeln("%SYSTEM-I-SAVING, saving filesystem...")
        err = self.fs.save(self.save_file)
        if err:
            self.terminal.writeln(err)
        else:
            self.terminal.writeln("%SYSTEM-I-SAVED, filesystem saved")

        self.terminal.writeln("")
        self.terminal.writeln("  {} logged out at {}".format(self.current_user, _get_timestamp()))
        self.terminal.writeln("")
        self.running = False

    def cmd_exit(self, args, quals):
        """EXIT command (alias for LOGOUT)."""
        self.cmd_logout(args, quals)

    def cmd_wait(self, args, quals):
        """WAIT command."""
        if args:
            try:
                # Parse VMS time format or seconds
                secs = int(args[0]) if args[0].isdigit() else 1
                time.sleep(secs)
            except:
                pass

    def cmd_if(self, args, quals):
        """IF command - handled in procedure execution."""
        self.terminal.writeln("%DCL-I-DIRECT, IF only valid in command procedures")

    def cmd_goto(self, args, quals):
        """GOTO command - handled in procedure execution."""
        self.terminal.writeln("%DCL-I-DIRECT, GOTO only valid in command procedures")

    def cmd_on(self, args, quals):
        """ON command stub."""
        self.terminal.writeln("%DCL-I-ONSET, ON condition handler set")


def main():
    """Main entry point."""
    vms = VAXVMS()
    vms.boot()


if __name__ == '__main__':
    main()