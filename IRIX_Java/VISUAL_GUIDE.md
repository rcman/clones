# Visual Walkthrough - IRIX Desktop Enhanced

## What You'll See When You Run It

### 🖥️ Main Desktop Screen

```
╔════════════════════════════════════════════════════════════════════╗
║                                                                    ║
║  🖥️  System                     ╔═══════════════════╗             ║
║      Manager                    ║                   ║             ║
║                                 ║       SGI         ║             ║
║  💻  Terminal                   ║ Silicon Graphics  ║             ║
║                                 ║                   ║             ║
║  📁  File                       ╚═══════════════════╝             ║
║      Manager                                                       ║
║                                   (Centered Logo)                  ║
║  📝  Text                                                          ║
║      Editor                                                        ║
║                                                                    ║
║  🌐  Web                                                           ║
║      Browser                                                       ║
║                                                                    ║
║  🔢  Calculator                                                    ║
║                                                                    ║
║  🎨  Drawing                                                       ║
║      Tool                                                          ║
║                                                                    ║
║  🎵  Media                                                         ║
║      Player                                                        ║
║                                                                    ║
╠════════════════════════════════════════════════════════════════════╣
║ [Start] [Workspace 1] [WS]  ...  🔊 🌐 [Wed 14:23:45] [Shutdown] ║
╚════════════════════════════════════════════════════════════════════╝

Background: Beautiful blue gradient (dark blue at top → lighter blue at bottom)
Grid: Subtle white grid pattern across entire desktop
Icons: Left side, vertically aligned with 85px spacing
Logo: Large semi-transparent "SGI Silicon Graphics" in center
Taskbar: 40px height at bottom with beveled 3D buttons
```

### 📋 Start Menu (Click "Start" Button)

```
┌─────────────────────────┐
│ IRIX                    │ ← Blue header (#0066CC)
│ Applications            │
├─────────────────────────┤
│ ⚙️  System Manager      │ ← Hover = dark blue highlight
│ 🖥️  Terminal            │
│ 📁  File Manager        │
│ 📝  Text Editor         │
│ 🌐  Web Browser         │
│ 🔢  Calculator          │
│ 🎨  Drawing Tool        │
│ 🎵  Media Player        │
├─────────────────────────┤ ← Separator
│ ⚡  System Settings     │
│ ❓  Help                │
├─────────────────────────┤ ← Separator
│ ⏻  Shutdown             │
└─────────────────────────┘

Size: 240x400 pixels
Position: Above Start button
Style: Gray background with 3D border
Hover Effect: Items turn dark blue with white text
```

### 🎮 Workspace Manager (Click "WS" Button)

```
╔════════════════════════════════════════════════════╗
║        Virtual Workspaces                          ║
║  Click a workspace to switch, or use Ctrl+Alt+1-4  ║
╠════════════════════════════════════════════════════╣
║                                                    ║
║  ┌─────────────────┐  ┌─────────────────┐        ║
║  │ ✓               │  │                 │        ║
║  │                 │  │                 │        ║
║  │        1        │  │        2        │        ║
║  │                 │  │                 │        ║
║  │  Workspace 1    │  │  Workspace 2    │        ║
║  └─────────────────┘  └─────────────────┘        ║
║    Light Blue            Light Pink                ║
║                                                    ║
║  ┌─────────────────┐  ┌─────────────────┐        ║
║  │                 │  │                 │        ║
║  │                 │  │                 │        ║
║  │        3        │  │        4        │        ║
║  │                 │  │                 │        ║
║  │  Workspace 3    │  │  Workspace 4    │        ║
║  └─────────────────┘  └─────────────────┘        ║
║    Light Green           Light Yellow              ║
║                                                    ║
║  💡 Each workspace maintains its own set of windows║
║                                       [Close]      ║
╚════════════════════════════════════════════════════╝

Size: 500x380 pixels
Active Workspace: Shows green checkmark (✓)
Hover Effect: Workspace brightens
Large Numbers: 48pt font size for clarity
```

### 🖥️ System Manager Window

```
╔═══════════════════════════════════════════════════════════════╗
║ ⚙ System Manager                              [_][□][×]        ║
╠═══════════════════════════════════════════════════════════════╣
║ [System Info] [Processes] [Performance] [Users] [Network]    ║
╠═══════════════════════════════════════════════════════════════╣
║                                                               ║
║  ═══════════════════════════════════════════                 ║
║      IRIX System Information                                 ║
║  ═══════════════════════════════════════════                 ║
║                                                               ║
║  Operating System                                            ║
║    OS Name:           IRIX 6.5                               ║
║    Host:              localhost                              ║
║    Machine Type:      SGI Indy                               ║
║    Processor:         MIPS R5000 @ 150MHz                    ║
║                                                               ║
║  Graphics System                                             ║
║    Graphics:          Newport (XL-8)                         ║
║    Resolution:        1920x1080                              ║
║    Color Depth:       24-bit                                 ║
║                                                               ║
║  Memory Information                                          ║
║    Physical RAM:      128 MB                                 ║
║    Java Max Memory:   4096 MB                                ║
║    Java Used Memory:  256 MB                                 ║
║    Java Free Memory:  3840 MB                                ║
║                                                               ║
║  ... (scrollable)                                            ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝

Title Bar: Blue (#0066CC) with white text
Tabs: 5 tabs for different categories
Font: Monospaced for data, makes it look technical
Window: Can be moved, resized, minimized, maximized, closed
```

### 📊 Performance Tab (in System Manager)

```
╔═══════════════════════════════════════════════════════════════╗
║ ⚙ System Manager                              [_][□][×]        ║
╠═══════════════════════════════════════════════════════════════╣
║ [System Info] [Processes] [Performance] [Users] [Network]    ║
╠═══════════════════════════════════════════════════════════════╣
║                                                               ║
║  ┌─ CPU Usage ─────────────────────────────────────────────┐ ║
║  │ [█████████████████░░░░░░░░░░] 25% - MIPS R5000         │ ║
║  └──────────────────────────────────────────────────────────┘ ║
║                                                               ║
║  ┌─ Memory Usage ───────────────────────────────────────────┐ ║
║  │ [████████████████████████████░░░░] 60% - 77/128 MB     │ ║
║  └──────────────────────────────────────────────────────────┘ ║
║                                                               ║
║  ┌─ Disk Usage ─────────────────────────────────────────────┐ ║
║  │ [████████████████████░░░░░░░░░░░] 45% - 4.5/10 GB      │ ║
║  └──────────────────────────────────────────────────────────┘ ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝

Progress Bars: Color-coded (green=CPU, blue=memory, orange=disk)
Animated: Values update every 2 seconds
Borders: Titled borders for each meter
Percentages: Show exact values
```

### 💻 Terminal Window

```
╔═══════════════════════════════════════════════════════════════╗
║ 💻 Terminal                                    [_][□][×]        ║
╠═══════════════════════════════════════════════════════════════╣
║ IRIX (tm) Version 6.5 IP32                                    ║
║ Copyright 1987-1996 Silicon Graphics, Inc.                    ║
║ All Rights Reserved.                                          ║
║                                                               ║
║ /home/user %                                                  ║
║ /home/user % ls                                               ║
║ d Documents                                                   ║
║ d Downloads                                                   ║
║ - readme.txt                                                  ║
║                                                               ║
║ /home/user % _                                                ║
║                                                               ║
║                                                               ║
║                                                               ║
║ ─────────────────────────────────────────────────────────────║
║ [                                                          ]  ║
╚═══════════════════════════════════════════════════════════════╝

Background: Black (#000000)
Text: Green (#00FF00) for authentic terminal look
Font: Monospaced, 14pt
Input Field: At bottom for command entry
Commands: ls, cd, pwd, clear, date, whoami, echo, help
History: Up/Down arrows for command history
```

### 🎨 Drawing Tool Window

```
╔═══════════════════════════════════════════════════════════════╗
║ 🎨 Drawing Tool                                [_][□][×]        ║
╠═══════════════════════════════════════════════════════════════╣
║ [Line] [Rectangle] [Ellipse] [Clear]                         ║
║ [█][█][█][█][█][█][█] ← Color palette                        ║
╠═══════════════════════════════════════════════════════════════╣
║                                                               ║
║                                                               ║
║                                                               ║
║                  (Drawing Canvas)                            ║
║                                                               ║
║                                                               ║
║                                                               ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝

Toolbar: Top of window with tool and color buttons
Canvas: White background for drawing
Tools: Line, Rectangle, Ellipse, Clear
Colors: Black, Red, Green, Blue, Yellow, Magenta, Cyan
Drawing: Click and drag to create shapes
Anti-aliasing: Smooth, professional-looking shapes
```

### 🎵 Media Player Window

```
╔═══════════════════════════════════════════════════════════════╗
║ 🎵 Media Player                                [_][□][×]        ║
╠═══════════════════════════════════════════════════════════════╣
║                    Stopped - 0%                               ║
║ ┌───────────────────────────────────────────────────────────┐ ║
║ │                                                           │ ║
║ │              ▂▄▆█▆▄▂  ▂▄█▄▂  ▄▆▂                        │ ║
║ │           ▂▄▆█▆▄▂  ▂▄▆█▆▄▂  ▂▄▆█▆▄▂                     │ ║
║ │        ▂▄▆█▆▄▂  ▄▆█▆▄  ▂▄▆█▆▄▂  ▄▆█▆                    │ ║
║ │                                    │                      │ ║
║ │              Audio Visualization   │← Progress Line       │ ║
║ │                                                           │ ║
║ └───────────────────────────────────────────────────────────┘ ║
║ [────────────────────────────────] ← Progress Slider        ║
║                                                               ║
║              [▶] [⏸⏸] [■] ← Playback Controls                ║
║                                                               ║
╚═══════════════════════════════════════════════════════════════╝

Visualization: Animated "audio waveform" (random bars)
Controls: Play, Pause, Stop buttons
Progress: Slider shows playback position (0-100%)
Status: Text shows state and position
Background: Black visualization area for contrast
Animated: Updates 10 times per second when playing
```

### 📝 Desktop Context Menu (Right-Click)

```
┌─────────────────────┐
│ New Folder          │
├─────────────────────┤
│ Refresh Desktop     │
│ Arrange Icons       │
├─────────────────────┤
│ Desktop Properties  │
└─────────────────────┘

Trigger: Right-click anywhere on desktop
Position: At mouse cursor
Background: Light gray (#BDBDBD)
Actions: All functional and working
```

### ⚠️ Shutdown Dialog

```
┌──────────────────────────────────────┐
│  Shutdown System                     │
├──────────────────────────────────────┤
│                                      │
│   ⚠️    Are you sure you want to     │
│         shut down the system?        │
│                                      │
├──────────────────────────────────────┤
│  [Shutdown] [Restart] [Cancel]       │
└──────────────────────────────────────┘

Size: 400x200 pixels
Icon: Warning triangle emoji
Buttons: Three options with authentic IRIX styling
Modal: Blocks interaction with other windows
Centered: Appears in center of screen
```

## Color Theme Throughout

### Desktop
- **Gradient Top**: #284678 (Dark Blue)
- **Gradient Bottom**: #0050A0 (Medium Blue)
- **Grid**: rgba(255,255,255,0.05) (Semi-transparent white)
- **Logo**: rgba(255,255,255,0.60) (Semi-transparent white)

### Windows
- **Title Bar**: #0066CC (IRIS Blue)
- **Title Text**: #FFFFFF (White)
- **Background**: #F0F0F0 (Light Gray)
- **Border**: #636363 (Dark Gray)

### Buttons
- **Normal Background**: #BDBDBD (Light Gray)
- **Highlight (top/left)**: #EEEEEE (Very Light Gray)
- **Shadow (bottom/right)**: #424242 (Very Dark Gray)
- **Pressed Background**: #636363 (Dark Gray)
- **Text**: #000000 (Black)

### Desktop Icons
- **Normal**: Full color, no effects
- **Hover**: White overlay (40% opacity) + border
- **Text**: White with black shadow
- **Selection Box**: Rounded, white, 100% opacity

## Typography

### Desktop
- **Icon Labels**: SansSerif Bold 11pt
- **Logo**: SansSerif Bold 72pt

### System
- **Window Titles**: SansSerif Bold 12pt
- **Menu Items**: SansSerif Plain 12pt
- **Buttons**: SansSerif Bold 10-11pt
- **System Info**: Monospaced Plain 12pt
- **Terminal**: Monospaced Plain 14pt

## Spacing & Layout

### Desktop
- **Icon Vertical Spacing**: 85px
- **Icon Horizontal Spacing**: 120px
- **Icon from Edge**: 20px
- **Taskbar Height**: 40px

### Windows
- **Title Bar Height**: 24px
- **Border Width**: 2px
- **Internal Padding**: 10-15px
- **Tab Height**: 28px

### Buttons
- **Start Button**: 80x28px
- **WS Button**: 45x28px
- **Shutdown Button**: 90x28px
- **System Tray Icons**: 30x28px

---

**Every pixel is designed to recreate the authentic SGI IRIX experience!**
