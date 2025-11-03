# IRIX Desktop Icons - Visual Reference

## Icon Gallery

All icons are programmatically generated at 48x48 pixels with anti-aliasing and authentic IRIX styling.

### 🖥️ System Manager Icon
```
┌──────────────────────┐
│  ╔═══════════════╗   │
│  ║ ▓▓▓▓▓▓▓▓▓▓▓▓▓ ║   │  - Computer monitor shape
│  ║ ▓▓▓▓▓▓▓▓▓▓▓▓▓ ║   │  - Blue screen with reflection
│  ║ ▓▓░░░░▓▓▓▓▓▓▓ ║   │  - Settings gear symbol
│  ║ ▓▓▓▓▓▓▓▓▓▓▓▓▓ ║   │  - Gray monitor frame
│  ╚═══════════════╝   │  - Stand base
│      ╔═══╗           │
│      ╚═══╝           │
└──────────────────────┘
Colors: Gray frame (#646478), Blue screen (#00B4FF), White gear
```

### 💻 Terminal Icon
```
┌──────────────────────┐
│ ╔═══════════════════╗│
│ ║▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓║│  - Black terminal window
│ ║                   ║│  - Green text on black
│ ║  >_               ║│  - Command prompt
│ ║  >                ║│  - Blue title bar
│ ║                   ║│  - Rounded corners
│ ╚═══════════════════╝│
└──────────────────────┘
Colors: Black screen, Green text (#00FF00), Blue bar (#0066CC)
```

### 📁 File Manager Icon
```
┌──────────────────────┐
│     ╔══╗              │
│    ╔╝  ╚══════╗       │  - Yellow folder
│   ╔╝           ║      │  - Tab on top
│  ╔╝ ░░░░░░░░░░ ║      │  - Lighter front face
│  ║  ░░░░░░░░░░ ║      │  - 3D depth effect
│  ║  ░░░░░░░░░░ ║      │  - Shadow underneath
│  ╚═════════════╝      │
└──────────────────────┘
Colors: Yellow-orange (#F0C864), Tab (#C8A050), Darker back (#C8A050)
```

### 📝 Text Editor Icon
```
┌──────────────────────┐
│   ╔═════════════╗    │
│   ║             ║    │  - White document
│   ║ ──────      ║    │  - Blue text lines
│   ║ ────────    ║    │  - Gray border
│   ║ ─────       ║    │  - Drop shadow
│   ║ ───────     ║    │  - Rounded corners
│   ║             ║    │
│   ╚═════════════╝    │
└──────────────────────┘
Colors: White paper, Blue lines (#0066CC), Gray border (#969696)
```

### 🌐 Web Browser Icon
```
┌──────────────────────┐
│      ╭─────╮         │
│    ╱         ╲       │  - Blue globe
│   │  ▓▓  ▓▓  │      │  - Green continents
│   │ ▓▓▓  ▓▓▓ │      │  - White meridians
│   │  ▓▓  ▓▓  │      │  - Horizontal equator
│    ╲         ╱       │  - Circular shape
│      ╰─────╯         │
└──────────────────────┘
Colors: Blue ocean (#0078C8), Green land (#009650), White lines
```

### 🔢 Calculator Icon
```
┌──────────────────────┐
│  ╔═══════════════╗   │
│  ║┌─────────────┐║   │  - Gray calculator body
│  ║│ 1234        │║   │  - Green LCD display
│  ║└─────────────┘║   │  - Black text
│  ║ ▓▓ ▓▓ ▓▓     ║   │  - Gray buttons (3x3)
│  ║ ▓▓ ▓▓ ▓▓     ║   │  - Rounded corners
│  ║ ▓▓ ▓▓ ▓▓     ║   │  - Beveled edges
│  ╚═══════════════╝   │
└──────────────────────┘
Colors: Gray body (#50505A), Green LCD (#B4DCB4), Dark gray buttons
```

### 🎨 Drawing Tool Icon
```
┌──────────────────────┐
│ ╔═══════════════════╗│
│ ║░░░░░░░░░░░░░░░░░░░║│  - White canvas
│ ║░  ╱╲    ╭─╮     ░║│  - Red brush stroke
│ ║░ ╱  ╲   │ │     ░║│  - Blue circle
│ ║░╱    ╲  ╰─╯     ░║│  - Green rectangle
│ ║░   ▄▄          ░║│  - Brown palette
│ ║░  █▓▓█  ●●●    ░║│  - RGB color dots
│ ╚═══════════════════╝│
└──────────────────────┘
Colors: Red/Blue/Green strokes, Brown palette (#8B5A2B), RGB dots
```

### 🎵 Media Player Icon
```
┌──────────────────────┐
│     ╭───────╮        │
│    ╱  ░░░░░  ╲       │  - Silver/gray disc
│   │ ░▓▓▓▓▓▓░ │      │  - Rainbow reflection
│   │░▓█████▓░│      │  - Black center hole
│   │ ░▓▓▓▓▓▓░ │      │  - Gray inner circle
│    ╲  ░░░░░  ╱       │  - Green play button
│     ╰───────╯  ▶    │
└──────────────────────┘
Colors: Gray disc (#C8C8DC), Rainbow gradient, Black hole, Green play
```

## Icon Design Principles

### Authentic IRIX Style
1. **Simple Geometry**: Clean shapes, no excessive detail
2. **Flat Design**: Subtle 3D, not over-rendered
3. **Limited Colors**: 3-5 colors per icon maximum
4. **Clear Silhouettes**: Recognizable at small sizes
5. **Professional**: Business-appropriate, not cartoonish

### Technical Implementation
- **Resolution**: 48x48 pixels
- **Format**: BufferedImage TYPE_INT_ARGB
- **Rendering**: Graphics2D with anti-aliasing
- **Scaling**: Properly sized for desktop icons
- **Transparency**: Alpha channel support

### Color Palette Reference
```
Primary Colors (IRIX Standard):
- IRIS Blue:       #0066CC (0, 102, 204)
- SGI Indigo:      #324E85 (50, 78, 133)
- Light Gray:      #BDBDBD (189, 189, 189)
- Dark Gray:       #636363 (99, 99, 99)

Icon-Specific Colors:
- Monitor Gray:    #646478 (100, 100, 120)
- Screen Blue:     #00B4FF (0, 180, 255)
- Terminal Green:  #00FF00 (0, 255, 0)
- Folder Yellow:   #F0C864 (240, 200, 100)
- Ocean Blue:      #0078C8 (0, 120, 200)
- Land Green:      #009650 (0, 150, 80)
- Calculator LCD:  #B4DCB4 (180, 220, 180)
- Palette Brown:   #8B5A2B (139, 90, 43)
```

## Icon States

### Normal State
- Full color
- Standard rendering
- No effects

### Hover State (Selected)
- Semi-transparent white overlay (40% opacity)
- Rounded border (100% opacity)
- Maintains icon visibility

### Pressed State
- Not implemented for desktop icons
- (Reserved for buttons)

## Implementation Details

All icons are created in the `loadIcons()` method using:
```java
private ImageIcon createXxxIcon() {
    BufferedImage img = new BufferedImage(48, 48, TYPE_INT_ARGB);
    Graphics2D g = img.createGraphics();
    g.setRenderingHint(KEY_ANTIALIASING, VALUE_ANTIALIAS_ON);
    
    // Draw icon elements
    
    g.dispose();
    return new ImageIcon(img);
}
```

## Desktop Display

Icons appear on desktop with:
- **Icon**: 48x48 centered at top
- **Label**: 1-2 lines below icon
- **Shadow**: Black text shadow for contrast
- **Selection Box**: Rounded rectangle when selected
- **Spacing**: 85 pixels vertical, 120 pixels horizontal

## Future Enhancements

Potential improvements:
- [ ] Animated icons (spinning disc for media player)
- [ ] Icon drag-and-drop
- [ ] Custom icon upload
- [ ] Icon size options (32x32, 48x48, 64x64)
- [ ] Alternative icon styles
- [ ] Icon themes

---

**All icons are original creations inspired by SGI IRIX aesthetic!**
