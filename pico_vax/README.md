# VAX/VMS Emulator for Raspberry Pi Pico

A complete, authentic VAX/VMS operating system emulator written in Python, designed to run on Raspberry Pi Pico or any Python environment. Experience the classic Digital Equipment Corporation (DEC) operating system with full DCL (Digital Command Language) command-line interface.

## Features

### Authentic VMS Experience

- **VMS 5.5-2 Boot Sequence**: Complete with copyright notice, login banner, and system information
- **DCL Command Processor**: Full-featured Digital Command Language interpreter
- **VMS Error Messages**: Authentic error message format with facility codes and severity levels
- **VT100 Terminal Support**: Terminal control sequences for screen clearing and cursor positioning
- **Case-Insensitive Commands**: All commands follow VMS conventions (converted to uppercase)

### File System Features

#### Hierarchical File System
- VMS-style directory structure: `SYS$SYSDEVICE:[000000]`
- Default directories: SYSMGR, SYSEXE, SYSMAINT, USER, SCRATCH
- Path syntax: `[dir1.dir2.dir3]` for subdirectories
- Parent directory navigation: `[-]`
- Root directory: `[000000]`
- Directories displayed as `NAME.DIR;1`

#### File Versioning System
- **Automatic Version Control**: Files automatically versioned (FILE.TXT;1, FILE.TXT;2, etc.)
- **Version History**: Old versions preserved when files are modified
- **Version-Specific Access**: Read any version with `TYPE FILE.TXT;1`
- **PURGE Command**: Remove old versions, keeping only the latest
- **DIR /VERSIONS**: Display all versions of files

#### File Operations
- Full RMS (Record Management Services) attributes
- Record format: Variable length
- Record attributes: Carriage return carriage control
- File protection codes: `(S:RWED,O:RWED,G:R,W:)`
- Owner and UIC (User Identification Code) tracking
- Timestamps in VMS format: `DD-MMM-YYYY HH:MM:SS.00`

### Command Features

#### Command Abbreviation
All commands support VMS minimum uniqueness abbreviation:
- `DIR`, `DIRE`, `DIREC`, `DIRECT` → `DIRECTORY`
- `SHO DEF` → `SHOW DEFAULT`
- `TY` → `TYPE`
- `LOG` → `LOGOUT`
- `COP` → `COPY`
- `REN` → `RENAME`

#### Qualifier Support
VMS-style qualifiers with `/` prefix:
- `/FULL` - Detailed file information with RMS attributes
- `/VERSIONS` - Show all file versions
- `/PROTECTION=(...)` - Set file protection codes
- `/DIR` - Create directory
- Position-independent qualifiers
- Value-based qualifiers: `/QUALIFIER=value`

#### Wildcard Pattern Matching
- `*` - Matches zero or more characters
- `%` - Matches exactly one character
- Examples: `*.TXT`, `TEST*.*`, `FILE%.DAT`, `*.*`

### Implemented Commands

#### File Management
- **DIRECTORY** (`DIR`) - List files with pattern matching
  - `/FULL` - Display detailed file information
  - `/VERSIONS` - Show all file versions
  - Supports wildcards: `DIR *.TXT`
- **TYPE** - Display file contents (supports version numbers: `TYPE FILE.TXT;1`)
- **COPY** - Copy files
- **RENAME** - Rename files
- **DELETE** - Delete files (preserves version history)
- **CREATE** - Create empty files
- **CREATE/DIR** - Create directories
- **PURGE** - Remove old file versions
- **APPEND** - Append one file to another
- **DIFFERENCES** - Compare two files
- **DUMP** - Hex dump of file contents
- **SEARCH** - Search for text in files
- **SORT** - Sort file contents
- **BACKUP** - Backup files

#### Directory Navigation
- **SET DEFAULT** - Change directory
  - `SET DEFAULT [USER]` - Navigate to directory
  - `SET DEFAULT [-]` - Go to parent directory
  - `SET DEFAULT [000000]` - Go to root
- **SHOW DEFAULT** - Display current directory path

#### System Information
- **SHOW TIME** - Current date and time in VMS format
- **SHOW SYSTEM** - System version and node name
- **SHOW USERS** - Display logged-in users
- **SHOW PROCESS** - Process information (PID, terminal, username)
- **SHOW MEMORY** - Memory statistics (uses Python's gc module)
- **SHOW DEVICES** - List available devices
- **SHOW QUOTA** - Display disk quotas

#### DCL Symbols and Logical Names
- **ASSIGN** - Create logical name assignments
  - `ASSIGN MYDIR SYS$SYSDEVICE:[USER.MYDIR]`
- **DEASSIGN** - Remove logical name assignments
- **SHOW SYMBOL** - Display DCL symbols
  - `SHOW SYMBOL` - Show all symbols
  - `SHOW SYMBOL name` - Show specific symbol
- Case-insensitive symbol storage

#### File Protection
- **SET FILE** - Modify file protection codes
  - `SET FILE filename /PROTECTION=(S:RWED,O:RWED,G:RE,W:)`
  - Protection format: (System:RWED, Owner:RWED, Group:RWED, World:RWED)
  - RWED = Read, Write, Execute, Delete permissions

#### Enhanced Text Editor (EDT Simulation)
- **EDIT** - Full-featured line editor
  - `INSERT` (I) - Enter insert mode (type `.` to finish)
  - `LIST` (L) - Display buffer with line numbers
  - `DELETE line#` (D) - Delete specific line
  - `REPLACE line#` (R) - Replace line content
  - `FIND text` (F) - Search for text in buffer
  - `SUBSTITUTE/old/new/` (S) - Global search and replace
  - `EXIT` (E) - Save file and exit
  - `QUIT` (Q) - Exit without saving

#### Utilities
- **ANALYZE** - Analyze disk/file (stub implementation)
- **MAIL** - VAX/VMS Mail utility (stub)
- **PHONE** - Phone facility for inter-user communication (stub)
- **RUN** - Execute programs (stub)
- **SUBMIT** - Submit batch jobs (stub)
- **PRINT** - Queue files for printing (stub)
- **SPAWN** - Create subprocess (stub)
- **MOUNT** - Mount device
- **DISMOUNT** - Dismount device
- **INITIALIZE** - Initialize device

#### Session Management
- **LOGOUT** / **EXIT** - Save filesystem state and exit
- **HELP** - Display comprehensive command reference

### Persistence

#### State Management
- Filesystem automatically saved to `vaxvms_state.json` on logout
- State automatically loaded on boot if file exists
- JSON format stores:
  - Complete directory tree
  - All file contents
  - File metadata (size, owner, protection, timestamps)
  - Version history for all files
  - DCL symbols and logical names

### Error Handling

#### Authentic VMS Error Messages
Format: `%FACILITY-SEVERITY-IDENT, description`

**Severity Levels:**
- `I` - Informational
- `W` - Warning
- `E` - Error
- `S` - Success

**Example Messages:**
- `%DCL-W-IVVERB, unrecognized command verb`
- `%DELETE-E-FILENOTFOUND, file not found`
- `%CREATE-I-CREATED, file created`
- `%DIRECT-W-NOFILES, no files found`
- `%PURGE-I-FILPURG, 5 file versions purged`
- `%SET-E-DIRNOTFOUND, directory not found`

**Facilities:**
- DCL, DIRECT, DELETE, CREATE, PURGE, TYPE, SHOW, SET, ASSIGN, DEASSIGN, EDIT, COPY, RENAME, SEARCH, APPEND, DIFFERENCES, DUMP, SORT, BACKUP, MOUNT, DISMOUNT, INITIALIZE, etc.

### Terminal Features

#### Line Editing
- **Backspace** (`\x7f`, `\x08`) - Delete last character
- **Ctrl-C** (`\x03`) - Cancel current line
- **Ctrl-D** (`\x04`) - EOF/Logout
- **Ctrl-U** (`\x15`) - Clear entire line
- **Enter** (`\r`, `\n`) - Submit command

#### Display
- Automatic `\n` to `\r\n` translation for proper terminal display
- VT100 screen clearing support
- Proper cursor positioning

## Installation and Usage

### Requirements
- Python 3.x (or MicroPython for Raspberry Pi Pico)
- No external dependencies (uses only standard library)

### Running on Desktop/Server
```bash
python vax_test.py
```

### Running on Raspberry Pi Pico
1. Install MicroPython on your Raspberry Pi Pico
2. Copy `vax_test.py` to the Pico
3. Connect via serial terminal (115200 baud)
4. Reset the Pico to boot into VMS

### First Time Boot
```
VAX/VMS Version V5.5-2

    Welcome to VAX/VMS (TM) on Raspberry Pi Pico

    Memory: 264192 bytes

 (c) Copyright 1991 Digital Equipment Corporation.

Welcome to VAX/VMS Operating System

VAX/VMS Version 5.5-2

For help, type HELP at the $ prompt.


Username: SYSTEM

%LOGIN-I-LOGGEDIN, logged in as SYSTEM
        on PICO::TXA0: at 15-NOV-2025 14:23:45

$
```

## Example Usage Sessions

### Basic File Operations
```
$ DIR
Directory SYS$SYSDEVICE:[000000]

SYSMGR.DIR;1
SYSEXE.DIR;1
SYSMAINT.DIR;1
USER.DIR;1
SCRATCH.DIR;1

Total of 5 files

$ SET DEFAULT [SYSMGR]
$ DIR

Directory SYS$SYSDEVICE:[SYSMGR]

WELCOME.TXT;1        75  15-NOV-2025 14:23:45.00

Total of 1 files

$ TYPE WELCOME.TXT

Welcome to VAX/VMS Operating System

VAX/VMS Version 5.5-2

For help, type HELP at the $ prompt.
```

### File Versioning
```
$ CREATE MEMO.TXT
%CREATE-I-CREATED, MEMO.TXT created

$ EDIT MEMO.TXT
EDT Editor
Commands: INSERT (I), LIST (L), DELETE line# (D), REPLACE line# (R)
          FIND text (F), SUBSTITUTE/old/new/ (S), EXIT (E), QUIT (Q)

*I
Enter text (type . on a line to finish):
This is the first version of the memo.
.
*E
%EDIT-I-SAVED, file saved

$ EDIT MEMO.TXT
*I
Enter text (type . on a line to finish):
This is the second version with more information.
.
*E
%EDIT-I-SAVED, file saved

$ DIR MEMO.TXT /VERSIONS

Directory SYS$SYSDEVICE:[SYSMGR]

MEMO.TXT;2       45  15-NOV-2025 14:25:12.00
MEMO.TXT;1       42  15-NOV-2025 14:24:30.00

Total of 2 files

$ TYPE MEMO.TXT;1

This is the first version of the memo.

$ TYPE MEMO.TXT

This is the second version with more information.

$ PURGE MEMO.TXT
%PURGE-I-FILPURG, 1 file version purged

$ DIR MEMO.TXT /VERSIONS

Directory SYS$SYSDEVICE:[SYSMGR]

MEMO.TXT;2       45  15-NOV-2025 14:25:12.00

Total of 1 files
```

### Using Wildcards
```
$ DIR *.TXT

Directory SYS$SYSDEVICE:[SYSMGR]

WELCOME.TXT;1        75  15-NOV-2025 14:23:45.00
MEMO.TXT;2           45  15-NOV-2025 14:25:12.00

Total of 2 files

$ DIR WELC%.TXT

Directory SYS$SYSDEVICE:[SYSMGR]

WELCOME.TXT;1        75  15-NOV-2025 14:23:45.00

Total of 1 files
```

### Directory Navigation
```
$ SHOW DEFAULT
  SYS$SYSDEVICE:[000000]

$ SET DEFAULT [USER]
$ SHOW DEFAULT
  SYS$SYSDEVICE:[USER]

$ SET DEFAULT [SYSTEM]
$ SHOW DEFAULT
  SYS$SYSDEVICE:[USER.SYSTEM]

$ SET DEFAULT [-]
$ SHOW DEFAULT
  SYS$SYSDEVICE:[USER]

$ SET DEFAULT [000000]
$ SHOW DEFAULT
  SYS$SYSDEVICE:[000000]
```

### DCL Symbols and Logical Names
```
$ ASSIGN MYDIR SYS$SYSDEVICE:[USER.SYSTEM]
%DCL-I-SUPERSEDE, previous value has been superseded

$ SHOW SYMBOL MYDIR
  MYDIR = "SYS$SYSDEVICE:[USER.SYSTEM]"

$ SHOW SYMBOL
  MYDIR = "SYS$SYSDEVICE:[USER.SYSTEM]"

$ DEASSIGN MYDIR
```

### File Protection
```
$ DIR /FULL

Directory SYS$SYSDEVICE:[SYSMGR]

WELCOME.TXT;1
  Size:                 75
  Owner:           [SYSTEM,[1,4]]
  Created:         15-NOV-2025 14:23:45.00
  Revised:         15-NOV-2025 14:23:45.00 (1)
  File protection: (S:RWED,O:RWED,G:R,W:)
  File organization: Sequential
  Record format:   Variable
  Record attributes: Carriage return carriage control

Total of 1 files

$ SET FILE WELCOME.TXT /PROTECTION=(S:RWED,O:RWE,G:R,W:)
%SET-I-MODIFIED, file protection modified
```

### Enhanced Editor Features
```
$ EDIT TEST.TXT
EDT Editor
Commands: INSERT (I), LIST (L), DELETE line# (D), REPLACE line# (R)
          FIND text (F), SUBSTITUTE/old/new/ (S), EXIT (E), QUIT (Q)

*I
Enter text (type . on a line to finish):
Line one with error
Line two is correct
Line three with error
.
*L

   1  Line one with error
   2  Line two is correct
   3  Line three with error

*F error
   1  Line one with error
   3  Line three with error

*S/error/mistake/
%EDIT-I-SUBSTITUTED, 2 substitution(s) made

*L

   1  Line one with mistake
   2  Line two is correct
   3  Line three with mistake

*D 3
%EDIT-I-DELETED, line 3 deleted

*R 1
Current: Line one with mistake
New: Line one is now fixed
%EDIT-I-REPLACED, line replaced

*L

   1  Line one is now fixed
   2  Line two is correct

*E
%EDIT-I-SAVED, file saved
```

### File Utilities
```
$ COPY WELCOME.TXT BACKUP.TXT

$ APPEND WELCOME.TXT BACKUP.TXT
%APPEND-I-APPENDED, WELCOME.TXT appended to BACKUP.TXT

$ SEARCH BACKUP.TXT "VMS"
VAX/VMS Version 5.5-2

$ DIFFERENCES WELCOME.TXT BACKUP.TXT
Number of difference sections found: 1
Number of difference records found: 5

$ DUMP WELCOME.TXT

File dump of WELCOME.TXT

00000000  57 65 6C 63 6F 6D 65 20 74 6F 20 56 41 58 2F 56  Welcome to VAX/V
00000010  4D 53 20 4F 70 65 72 61 74 69 6E 67 20 53 79 73  MS Operating Sys
```

### System Commands
```
$ SHOW TIME
  15-NOV-2025 14:30:25

$ SHOW SYSTEM
  VAX/VMS V5.5-2 on node PICO
  OpenVMS V5.5-2 on node PICO

$ SHOW USERS
    VAX/VMS Interactive Users

  Username  Process Name      PID     Terminal
  SYSTEM    SYSTEM          00000042  TXA0:

$ SHOW PROCESS
  Process name:    SYSTEM
  Process ID:      00000042
  Terminal:        TXA0:
  User name:       SYSTEM

$ SHOW MEMORY
  Total Physical Memory:       264192 bytes
  Free Memory:                 128456 bytes
  Used Memory:                 135736 bytes

$ SHOW DEVICES
  Device  Type           Status
  TXA0:   Terminal       Online
  SYS$SYSDEVICE:   System Disk    Mounted

$ SHOW QUOTA
  Disk quotas for user SYSTEM
  Disk Usage:     Unlimited
  File Count:     Unlimited
```

### Sorting Files
```
$ TYPE NAMES.TXT

Charlie
Alice
Bob

$ SORT NAMES.TXT SORTED.TXT
%SORT-I-SORTED, 3 records sorted
%SORT-I-CREATED, SORTED.TXT created

$ TYPE SORTED.TXT

Alice
Bob
Charlie
```

### Backup Operations
```
$ BACKUP IMPORTANT.TXT SAFE
%BACKUP-I-COPIED, copied IMPORTANT.TXT
%BACKUP-S-BACKUPCOMPLETE, backup operation completed successfully

$ DIR SAFE.BCK

Directory SYS$SYSDEVICE:[USER]

SAFE.BCK;1          156  15-NOV-2025 14:35:20.00

Total of 1 files
```

### Command Abbreviations
```
$ SHO DEF
  SYS$SYSDEVICE:[000000]

$ SHO TI
  15-NOV-2025 14:40:15

$ DIR *.TXT
...

$ TY WELCOME.TXT
...

$ COP FILE1.TXT FILE2.TXT

$ REN OLD.TXT NEW.TXT

$ DEL TEMP.DAT
```

## Technical Details

### Architecture

#### Class Structure
- **FileNode**: Represents files and directories
  - Stores metadata, content, versions
  - Serialization support for JSON persistence
- **FileSystem**: Manages directory tree
  - VMS path syntax handling
  - Pattern matching (wildcards)
  - Version control management
- **SerialTerminal**: Terminal I/O handling
  - Line editing capabilities
  - VT100 control sequences
- **VAXVMS**: Main system orchestrator
  - DCL command processor
  - Command abbreviation matching
  - Qualifier parsing
  - Boot sequence management

#### File Extensions
- Files without extensions automatically get `.DAT` extension
- Directories always display as `.DIR;1`
- Version numbers appended with semicolon (`;`)

#### Memory Management
- On Raspberry Pi Pico: Uses MicroPython's `gc` module
- Automatic garbage collection
- Memory statistics via `SHOW MEMORY`

### Python Compatibility

#### Standard Library Only
- `sys` - I/O operations
- `time` - Timestamps
- `json` - Filesystem persistence
- `os` - File operations (for state file)
- `re` - Pattern matching

#### MicroPython Compatible
- No external dependencies
- Uses `gc.mem_free()` and `gc.mem_alloc()` on Pico
- Handles `OSError` instead of `FileNotFoundError`
- Compatible with MicroPython's limited `time` module

## Version History

### Current Version (1.0)

**Core Features:**
- Full DCL command processor with abbreviation support
- VMS-style hierarchical filesystem
- Automatic file versioning
- Qualifier parsing system
- Wildcard pattern matching
- JSON persistence

**File Operations:**
- DIRECTORY with /FULL and /VERSIONS qualifiers
- TYPE with version number support
- COPY, RENAME, DELETE, CREATE
- PURGE for version management
- APPEND, DIFFERENCES, DUMP
- SEARCH in files
- SORT file contents
- BACKUP files

**Enhanced Editor:**
- INSERT, LIST commands
- DELETE, REPLACE line operations
- FIND text search
- SUBSTITUTE global replace
- Line-numbered display

**System Commands:**
- SET DEFAULT, SET FILE, SET TERMINAL
- SHOW DEFAULT, TIME, SYSTEM, USERS, PROCESS, MEMORY, DEVICES, QUOTA
- ASSIGN/DEASSIGN logical names
- SHOW SYMBOL for DCL symbols

**Utilities:**
- MOUNT, DISMOUNT, INITIALIZE
- SPAWN subprocess
- MAIL, PHONE (stubs)
- RUN, SUBMIT, PRINT (stubs)
- ANALYZE (stub)

**Terminal Features:**
- VT100 support
- Line editing (backspace, Ctrl-C, Ctrl-D, Ctrl-U)
- Proper CR/LF handling

**Error Handling:**
- Authentic VMS error message format
- Facility codes and severity levels
- Comprehensive error coverage

## Future Enhancements

Possible additions for future versions:
- IF/THEN/ELSE conditional commands
- GOTO and label support
- Command procedures (.COM files)
- Batch job execution
- Full MAIL implementation
- Phone facility implementation
- Process spawning
- Volume mounting
- ACL (Access Control List) support
- DECnet networking (stub)
- Cluster support (stub)

## License

This is an educational project recreating the classic VAX/VMS experience. VAX and VMS are trademarks of Digital Equipment Corporation (now part of HP/HPE). This emulator is not affiliated with or endorsed by Digital Equipment Corporation or its successors.

## Author

Franco - VAX/VMS Enthusiast

## Acknowledgments

Inspired by the classic Digital Equipment Corporation VAX/VMS operating system, which powered enterprise computing from 1977 through the 2000s. This emulator aims to preserve the experience of this influential operating system for educational purposes and nostalgic exploration.
