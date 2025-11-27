# IRIX Desktop Environment - Enhanced Version

A faithful recreation of the Silicon Graphics IRIX 6.5 desktop environment, written in pure Java Swing.

## Improvements Over Original

### Architecture & Code Quality

1. **Better Resource Management**
   - All Swing Timers are tracked and properly cleaned up on application close
   - `registerTimer()` pattern prevents memory leaks
   - Proper cleanup in `WindowListener.windowClosing()`

2. **Centralized Constants**
   - All colors extracted to public static final constants in `IRIXDesktop`
   - UI dimensions (taskbar height, icon size, etc.) are now constants
   - Easy theming by modifying constant values

3. **Separated Concerns**
   - `IconFactory` class handles all icon creation (extracted from main class)
   - Each component has clear responsibilities
   - Reduced coupling between classes

4. **Improved Code Organization**
   - Consistent code formatting and naming conventions
   - Comprehensive JavaDoc comments
   - Logical grouping of related methods
   - Reduced code duplication

### Component Improvements

#### IRIXDesktop.java
- Cleaner initialization flow
- Desktop icons defined via configuration array (easier to add/remove)
- Proper keyboard shortcut registration helper method
- Better window centering logic

#### IRIXWindow.java
- Extracted button painting into separate methods
- Added tooltips to window controls
- Improved drag handling reliability
- Added `moveToWorkspace()` convenience method

#### Calculator.java
- **Full keyboard support** - type numbers and operators directly
- **Memory functions** (MC, MR, M+, M-) with indicator
- **Backspace support** for corrections
- **Percentage calculations**
- Better error handling (division by zero, overflow)
- Cleaner number formatting

#### TerminalEmulator.java
- **Many more commands**: `cat`, `head`, `tail`, `wc`, `mkdir`, `touch`, `rm`, `cp`, `mv`, `env`, `export`, `history`, `uname`, `hostname`, `uptime`, `df`, `man`
- **Colored output** (errors in red, info in blue, prompts in yellow)
- **Environment variables** with `$VAR` expansion in echo
- **Command chaining** with semicolons
- **Ctrl+C** to cancel, **Ctrl+L** to clear
- Path shortening in prompt (~ for home)
- Better tab completion for both commands and files

#### DrawingApp.java
- **Undo/Redo support** with stack-based history
- **Fill mode toggle** for solid shapes
- **Stroke width adjustment** (1-20px)
- **Round rectangle** tool added
- **Custom color chooser**
- **Status bar** showing tool, color, and position
- Shape properties stored with each drawn shape

#### FileSystemBrowser.java
- **Three-pane layout**: tree, file list, and preview
- **Breadcrumb navigation** with path display
- **File table** with Name, Size, Type, and Modified columns
- **Context menu** (Open, Copy Path, Properties)
- **Back/Forward navigation** with history
- **Better file type detection**
- **Smart preview** (text files only, with size limits)

#### TextEditor.java
- **Full Undo/Redo** with UndoManager
- **Find and Replace** dialogs
- **Go to Line** feature
- **Modified indicator** in status bar
- **Line/Column display** in status bar
- **Font size selection**
- **Proper save confirmation** on close/new

#### MediaPlayer.java
- **Playlist support** with sample tracks
- **Previous/Next track** navigation
- **Repeat and Shuffle modes**
- **Volume slider**
- **Better visualization** with color gradient bars
- **Time display** (current/total)

#### WebBrowser.java
- **History navigation** (back/forward)
- **Bookmarks** with add/view functionality
- **Special pages**: about:home, about:help, about:bookmarks
- **Styled home page** with gradient background
- **Better error pages**
- **Local file browsing**

#### StartMenu.java
- **Section headers** for organization
- **Improved visual design**
- **All applications accessible**

#### WorkspaceManager.java
- **Cleaner visual design**
- **Active workspace indicator** (green checkmark)
- **Hover effects**
- **ESC to close**

### Build System

#### build.sh (Linux/Mac)
- Command-line options: `--run`, `--clean`, `--jar`, `--help`
- Colored output for better readability
- Exit on first error with `set -e`
- JAR creation support
- Progress indication

#### build.bat (Windows)
- Same options as Linux version
- Proper error handling
- JAR creation support

## File Structure

```
irix-improved/
├── IRIXDesktop.java      # Main application & IconFactory
├── IRIXWindow.java       # Custom window component
├── Calculator.java       # Calculator with keyboard & memory
├── DrawingApp.java       # Drawing app with undo/redo
├── FileSystemBrowser.java # File manager
├── MediaPlayer.java      # Media player with playlist
├── TerminalEmulator.java # Terminal with many commands
├── TextEditor.java       # Text editor with undo/redo
├── WebBrowser.java       # Browser with history
├── StartMenu.java        # Application launcher
├── WorkspaceManager.java # Virtual desktop manager
├── build.sh              # Linux/Mac build script
├── build.bat             # Windows build script
└── README.md             # This file
```

## Building and Running

### Linux/Mac
```bash
chmod +x build.sh
./build.sh --run
```

### Windows
```batch
build.bat --run
```

### Manual Compilation
```bash
javac *.java
java IRIXDesktop
```

### Creating a JAR
```bash
./build.sh --jar
java -jar IRIXDesktop.jar
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+Alt+T | Open Terminal |
| Ctrl+Alt+1-4 | Switch Workspace |
| (In Terminal) Ctrl+C | Cancel input |
| (In Terminal) Ctrl+L | Clear screen |
| (In Text Editor) Ctrl+Z | Undo |
| (In Text Editor) Ctrl+Y | Redo |
| (In Text Editor) Ctrl+F | Find |
| (In Text Editor) Ctrl+H | Replace |
| (In Text Editor) Ctrl+G | Go to Line |

## Requirements

- Java 8 or higher
- No external dependencies (pure Java Swing)

## Credits

Inspired by Silicon Graphics IRIX 6.5 desktop environment.
