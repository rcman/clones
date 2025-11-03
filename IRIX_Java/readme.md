# IRIX Desktop Environment - Enhanced Edition

## 🌟 Overview

A **fully enhanced** Java recreation of the legendary SGI IRIX desktop environment with authentic icons, improved functionality, and beautiful UI design.

## ✨ Key Features

### Authentic IRIX Experience
- 🎨 **Hand-crafted Icons**: 8 unique, detailed 48x48 icons
- 🖥️ **SGI Branding**: Authentic IRIX/SGI color scheme and styling
- 💎 **3D Beveled UI**: True IRIX-style raised/pressed buttons
- 🌈 **Gradient Desktop**: Beautiful blue gradient with SGI logo
- 🎯 **Pixel Perfect**: Every detail matches the original aesthetic

### Enhanced Functionality
- 📱 **Desktop Context Menu**: Right-click for new folders, arrange icons, properties
- 🖱️ **Interactive Workspace Manager**: Visual 2x2 grid with color coding
- 📊 **5-Tab System Manager**: System info, processes, performance, users, network
- ⚡ **Live Performance Monitoring**: Animated CPU, memory, and disk meters
- 🪟 **Taskbar Window Buttons**: See and switch between open windows
- 💬 **Help System**: Complete documentation built-in
- ⌨️ **Keyboard Shortcuts**: Ctrl+Alt+T (terminal), Ctrl+Alt+1-4 (workspaces)

### Applications
1. **System Manager** - 5 tabs of system information and monitoring
2. **Terminal** - Full command-line interface (ls, cd, pwd, etc.)
3. **File Manager** - Browse filesystem, preview text files
4. **Text Editor** - Edit files with menus (Ctrl+N/O/S/F shortcuts)
5. **Web Browser** - Basic HTML viewing
6. **Calculator** - Complete arithmetic operations
7. **Drawing Tool** - Lines, rectangles, ellipses with colors
8. **Media Player** - Playback simulation with visualization

## 🚀 Quick Start

### Compilation
```bash
./build.sh
```

### Run
```bash
./build.sh --run
```

Or manually:
```bash
javac *.java
java IRIXDesktop
```

## 🎮 How to Use

### Desktop
- **Double-click icons** to open applications
- **Right-click desktop** for context menu
- **Drag icons** to rearrange them
- **Use context menu** to auto-arrange icons

### Taskbar
- **Start Button** - Opens application menu
- **WS Button** - Opens workspace manager
- **Window Buttons** - Click to focus windows
- **System Tray** - Sound and network indicators
- **Clock** - Live time display
- **Shutdown** - Power options

### Workspaces
- **Visual Manager** - Click WS button or Ctrl+Alt+1-4
- **4 Workspaces** - Color-coded for easy identification
- **Window Persistence** - Windows stay in their workspace

### System Manager
- **System Info Tab** - View hardware and software details
- **Processes Tab** - See running processes, kill unwanted ones
- **Performance Tab** - Monitor CPU, memory, disk usage
- **Users Tab** - See logged-in users
- **Network Tab** - View network interfaces

## ⌨️ Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Alt+T` | Open Terminal |
| `Ctrl+Alt+1` | Switch to Workspace 1 |
| `Ctrl+Alt+2` | Switch to Workspace 2 |
| `Ctrl+Alt+3` | Switch to Workspace 3 |
| `Ctrl+Alt+4` | Switch to Workspace 4 |
| `ESC` | Close Workspace Manager |

### Text Editor Shortcuts
| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New File |
| `Ctrl+O` | Open File |
| `Ctrl+S` | Save File |
| `Ctrl+F` | Find Text |
| `Ctrl+X/C/V` | Cut/Copy/Paste |
| `Ctrl+A` | Select All |

## 🎨 Visual Design

### Icons
Each icon is hand-crafted at 48x48 resolution with:
- Anti-aliased graphics
- Proper depth and shading
- SGI color palette
- Professional appearance

### UI Elements
- **3D Beveled Buttons**: Raised when normal, inset when pressed
- **Gradient Backgrounds**: Smooth color transitions
- **Shadow Effects**: Text shadows for readability
- **Hover Feedback**: Visual response to mouse interaction
- **Selection Highlights**: Rounded boxes around selected items

### Color Scheme
Based on authentic SGI IRIX colors:
- Primary: IRIS Blue (`#0066CC`)
- UI Background: Light Gray (`#BDBDBD`)
- Shadows: Dark Gray (`#636363`)
- Desktop: Blue gradient (`#284678` to `#0050A0`)

## 📦 Files Included

### Main Application
- `IRIXDesktop.java` - Enhanced main desktop (1300+ lines)
- `IRIXWindow.java` - Window component
- `StartMenu.java` - Enhanced start menu with icons
- `WorkspaceManager.java` - Visual workspace switcher

### Applications
- `Calculator.java` - Full calculator
- `DrawingApp.java` - Drawing application
- `FileSystemBrowser.java` - File manager
- `MediaPlayer.java` - Media player
- `TerminalEmulator.java` - Terminal
- `TextEditor.java` - Text editor
- `WebBrowser.java` - Web browser

### Build Files
- `build.sh` - Linux/Mac build script
- `build.bat` - Windows build script

### Documentation
- `README.md` - This file
- `ENHANCEMENTS.md` - Detailed enhancement list
- `IMPORT_FIXES.md` - Import issue fixes
- `VERIFICATION.md` - Compilation checklist

## 🐛 Troubleshooting

### Compilation Errors
If you get compilation errors, make sure you have JDK 11 or later:
```bash
java -version
javac -version
```

### Icons Not Showing
The icons are generated programmatically, so they should always work. If you see blank spaces, the Graphics2D rendering might have an issue.

### Windows Not Appearing in Taskbar
This is a known issue if windows are created before the taskbar. Try closing and reopening the application.

## 💡 Tips & Tricks

1. **Organize Your Workspace**: Use right-click → "Arrange Icons" for a clean desktop
2. **Multiple Workspaces**: Organize projects across different workspaces
3. **Quick Terminal**: Use Ctrl+Alt+T to open terminal instantly
4. **Process Management**: Use System Manager to see and manage processes
5. **Performance Monitoring**: Watch live system metrics in Performance tab
6. **Help Anytime**: Click Help in Start Menu for complete documentation

## 📊 System Requirements

- **Java**: JDK 11 or later
- **OS**: Linux, macOS, or Windows
- **RAM**: 256MB minimum (512MB recommended)
- **Display**: 1024x768 or higher

## 🎯 What's Different from Original IRIX

This is a **visual and functional recreation**, not an emulation:
- Runs on Java (original IRIX was Unix-based)
- Simulated system information (not actual MIPS hardware)
- Modern window management (JInternalFrame instead of X11)
- Simplified networking (no actual network configuration)
- Java application launcher (not native IRIX binaries)

However, the **look, feel, and workflow** are authentically IRIX!

## 🏆 Credits

Inspired by Silicon Graphics IRIX 6.5 - one of the most beautiful desktop environments ever created.

**Enhanced Edition Features:**
- Hand-crafted authentic icons
- Professional 3D beveling
- Interactive workspace management
- Complete system monitoring
- Desktop context menus
- Comprehensive help system

## 📜 License

This is a recreation for educational and nostalgic purposes.
IRIX and Silicon Graphics are trademarks of their respective owners.

## 🚀 Version

**Enhanced Edition v2.0**
- Original release: Fixed compilation errors
- Enhanced release: Complete visual overhaul + new features

---

**Enjoy your authentic IRIX experience! 🎉**

For detailed information about all enhancements, see `ENHANCEMENTS.md`.
<BR>
<BR>
<img width="1911" height="1034" alt="Screenshot From 2025-10-29 14-51-26" src="https://github.com/user-attachments/assets/417a7657-99b5-4ea1-b9ae-267bb05eb043" />
