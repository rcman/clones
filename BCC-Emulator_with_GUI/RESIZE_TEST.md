# Resize Testing Guide

## Testing the Dynamic Resize Feature

The GUI emulator now supports dynamic terminal resizing. Here's how to test it:

### Test 1: Starting with Minimum Size
1. Resize your terminal to exactly 80x24 (or close to it)
2. Run: `./gui-emulator`
3. The GUI should display properly
4. Try gradually making the terminal larger
5. The GUI should automatically resize and scale all windows

### Test 2: Starting Large and Going Small
1. Maximize your terminal window
2. Run: `./gui-emulator`
3. The GUI should display with larger windows
4. Gradually resize the terminal smaller
5. When you go below 80x24, you should see the "Terminal too small!" message
6. Resize back above 80x24
7. The GUI should restore and continue working

### Test 3: Live Resizing During Use
1. Start the emulator at any size ≥ 80x24
2. Load a program (F1)
3. While the program is loaded, resize your terminal
4. The GUI should adapt immediately
5. All panels should scale proportionally

### Expected Behavior

**Minimum Size (80x24):**
- Menu bar displays all options
- Four panels visible: Terminal Display, Processor State, Memory View
- Status bar shows messages
- All content fits within window borders

**Larger Sizes:**
- All panels scale proportionally
- More memory addresses visible in Memory View
- More terminal lines visible in Terminal Display
- Content adapts to available space

**Below Minimum:**
- Error message displayed with current and required size
- Instructions to resize or quit
- Can exit with ESC or 'q'

### Troubleshooting

**If resize doesn't work:**
1. Check your terminal emulator supports SIGWINCH signals
2. Try pressing a key after resizing (some terminals need this)
3. Verify ncurses is properly installed

**Terminal emulators known to work well:**
- GNOME Terminal
- xterm
- kitty
- Alacritty
- iTerm2 (macOS)

**May have issues:**
- Some older terminals
- Screen/tmux (requires special configuration)
- Windows Command Prompt (use WSL terminal instead)

### Key Changes from Previous Version

- **No longer requires fullscreen** - works at 80x24 minimum
- **Live resize support** - windows recreate automatically on resize
- **Graceful degradation** - clear error message if too small
- **Exit changed** - Now uses ESC or 'q' instead of F10
- **Auto-scaling** - all panels grow/shrink proportionally
