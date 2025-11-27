// IRIXDesktop.java - Enhanced Version with Authentic IRIX Look & Feel
// Improvements: Better resource management, cleaner architecture, proper timer cleanup,
// constants extraction, better error handling, and documentation

import javax.swing.*;
import javax.swing.Timer;
import javax.sound.sampled.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.awt.image.BufferedImage;
import java.io.*;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.Date;
import java.util.List;
import java.text.SimpleDateFormat;

/**
 * IRIX Desktop Environment - A faithful recreation of Silicon Graphics' IRIX 6.5 desktop.
 * This class serves as the main application frame and manages all desktop components.
 */
public class IRIXDesktop extends JFrame {
    
    // ========== Color Scheme Constants ==========
    // Authentic IRIX/SGI color scheme - centralized for easy theming
    public static final Color IRIS_BLUE = new Color(0, 102, 204);
    public static final Color IRIS_INDIGO = new Color(50, 78, 133);
    public static final Color LIGHT_GRAY = new Color(189, 189, 189);
    public static final Color DARK_GRAY = new Color(99, 99, 99);
    public static final Color HIGHLIGHT = new Color(238, 238, 238);
    public static final Color SHADOW = new Color(66, 66, 66);
    public static final Color DESKTOP_TOP = new Color(40, 70, 120);
    public static final Color DESKTOP_BOTTOM = new Color(0, 80, 160);
    public static final Color MENU_HIGHLIGHT = new Color(0, 0, 128);
    
    // ========== UI Constants ==========
    private static final int TASKBAR_HEIGHT = 40;
    private static final int ICON_SPACING = 85;
    private static final int ICON_SIZE = 48;
    private static final int GRID_SIZE = 50;
    private static final int NUM_WORKSPACES = 4;
    private static final int DOUBLE_CLICK_DELAY = 400;
    
    // ========== Instance Variables ==========
    private JDesktopPane desktop;
    private JPanel taskbar;
    private JButton startButton;
    private final List<IRIXWindow> windows;
    private StartMenu startMenu;
    private WorkspaceManager workspaceManager;
    private final Map<String, ImageIcon> icons;
    private final Clipboard clipboard;
    private int currentWorkspace = 0;
    private JLabel workspaceLabel;
    private JPanel windowButtonPanel;
    
    // Track timers for proper cleanup
    private final List<Timer> activeTimers = new ArrayList<>();
    
    public IRIXDesktop() {
        windows = new ArrayList<>();
        icons = new HashMap<>();
        clipboard = Toolkit.getDefaultToolkit().getSystemClipboard();
        loadIcons();
        initializeUI();
        workspaceManager = new WorkspaceManager(NUM_WORKSPACES, this);
        
        // Ensure proper cleanup on close
        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                cleanup();
            }
        });
    }
    
    /**
     * Clean up resources when closing the application
     */
    private void cleanup() {
        for (Timer timer : activeTimers) {
            timer.stop();
        }
        activeTimers.clear();
    }
    
    /**
     * Register a timer for cleanup tracking
     */
    private void registerTimer(Timer timer) {
        activeTimers.add(timer);
    }
    
    private void initializeUI() {
        setTitle("IRIX Desktop - Silicon Graphics Inc.");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setExtendedState(JFrame.MAXIMIZED_BOTH);
        setUndecorated(true);
        
        // Create desktop with authentic IRIX background
        desktop = createDesktopPane();
        desktop.setDragMode(JDesktopPane.LIVE_DRAG_MODE);
        
        createTaskbar();
        createDesktopIcons();
        createStartMenu();
        setupKeyboardShortcuts();
        addDesktopContextMenu();
        
        add(desktop, BorderLayout.CENTER);
        add(taskbar, BorderLayout.SOUTH);
    }
    
    /**
     * Creates the main desktop pane with IRIX-style gradient background
     */
    private JDesktopPane createDesktopPane() {
        return new JDesktopPane() {
            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                Graphics2D g2d = (Graphics2D) g;
                g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, 
                                     RenderingHints.VALUE_ANTIALIAS_ON);
                
                // IRIX-style gradient background
                GradientPaint gradient = new GradientPaint(
                    0, 0, DESKTOP_TOP,
                    0, getHeight(), DESKTOP_BOTTOM
                );
                g2d.setPaint(gradient);
                g2d.fillRect(0, 0, getWidth(), getHeight());
                
                drawSGILogo(g2d, getWidth(), getHeight());
                drawGridPattern(g2d, getWidth(), getHeight());
            }
        };
    }
    
    private void drawSGILogo(Graphics2D g2d, int width, int height) {
        int centerX = width / 2;
        int centerY = height / 2;
        int size = 200;
        
        // Semi-transparent background
        g2d.setColor(new Color(255, 255, 255, 20));
        g2d.fillRoundRect(centerX - size/2, centerY - size/2, size, size, 30, 30);
        
        // Draw "SGI" text in authentic style
        g2d.setColor(new Color(255, 255, 255, 60));
        Font sgiFont = new Font("SansSerif", Font.BOLD, 72);
        g2d.setFont(sgiFont);
        FontMetrics fm = g2d.getFontMetrics();
        String text = "SGI";
        int textWidth = fm.stringWidth(text);
        g2d.drawString(text, centerX - textWidth/2, centerY + 20);
        
        // Draw subtitle
        g2d.setFont(new Font("SansSerif", Font.PLAIN, 16));
        fm = g2d.getFontMetrics();
        String subtitle = "Silicon Graphics";
        textWidth = fm.stringWidth(subtitle);
        g2d.drawString(subtitle, centerX - textWidth/2, centerY + 50);
    }
    
    private void drawGridPattern(Graphics2D g2d, int width, int height) {
        g2d.setColor(new Color(255, 255, 255, 5));
        for (int x = 0; x < width; x += GRID_SIZE) {
            g2d.drawLine(x, 0, x, height);
        }
        for (int y = 0; y < height; y += GRID_SIZE) {
            g2d.drawLine(0, y, width, y);
        }
    }
    
    private void addDesktopContextMenu() {
        JPopupMenu contextMenu = new JPopupMenu();
        contextMenu.setBackground(LIGHT_GRAY);
        
        JMenuItem newFolder = createMenuItem("New Folder", e -> createNewFolder());
        JMenuItem refresh = createMenuItem("Refresh Desktop", e -> desktop.repaint());
        JMenuItem arrange = createMenuItem("Arrange Icons", e -> arrangeIcons());
        JMenuItem properties = createMenuItem("Desktop Properties", e -> showDesktopProperties());
        
        contextMenu.add(newFolder);
        contextMenu.addSeparator();
        contextMenu.add(refresh);
        contextMenu.add(arrange);
        contextMenu.addSeparator();
        contextMenu.add(properties);
        
        desktop.addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                showContextMenuIfTriggered(e, contextMenu);
            }
            @Override
            public void mouseReleased(MouseEvent e) {
                showContextMenuIfTriggered(e, contextMenu);
            }
        });
    }
    
    private JMenuItem createMenuItem(String text, ActionListener action) {
        JMenuItem item = new JMenuItem(text);
        item.addActionListener(action);
        return item;
    }
    
    private void showContextMenuIfTriggered(MouseEvent e, JPopupMenu menu) {
        if (e.isPopupTrigger()) {
            menu.show(e.getComponent(), e.getX(), e.getY());
        }
    }
    
    private void createNewFolder() {
        String name = JOptionPane.showInputDialog(this, "Folder Name:", "New Folder");
        if (name != null && !name.trim().isEmpty()) {
            JOptionPane.showMessageDialog(this, "Folder '" + name + "' created on desktop");
        }
    }
    
    private void arrangeIcons() {
        Component[] components = desktop.getComponents();
        int x = 20, y = 20;
        
        for (Component comp : components) {
            if (comp instanceof JButton) {
                comp.setLocation(x, y);
                y += ICON_SPACING;
                if (y > desktop.getHeight() - 100) {
                    y = 20;
                    x += 120;
                }
            }
        }
    }
    
    private void showDesktopProperties() {
        JDialog propsDialog = new JDialog(this, "Desktop Properties", true);
        propsDialog.setSize(450, 350);
        propsDialog.setLocationRelativeTo(this);
        
        JTabbedPane tabs = new JTabbedPane();
        tabs.addTab("Background", createBackgroundSettingsPanel());
        tabs.addTab("Screen Saver", createScreenSaverPanel());
        tabs.addTab("Appearance", createAppearancePanel());
        
        propsDialog.add(tabs);
        propsDialog.setVisible(true);
    }
    
    private JPanel createBackgroundSettingsPanel() {
        JPanel panel = new JPanel(new BorderLayout(10, 10));
        panel.setBorder(BorderFactory.createEmptyBorder(15, 15, 15, 15));
        panel.add(new JLabel("Background settings - select wallpaper or gradient"), 
                  BorderLayout.NORTH);
        return panel;
    }
    
    private JPanel createScreenSaverPanel() {
        JPanel panel = new JPanel(new BorderLayout(10, 10));
        panel.setBorder(BorderFactory.createEmptyBorder(15, 15, 15, 15));
        panel.add(new JLabel("Screen saver settings - timeout and effects"), 
                  BorderLayout.NORTH);
        return panel;
    }
    
    private JPanel createAppearancePanel() {
        JPanel panel = new JPanel(new BorderLayout(10, 10));
        panel.setBorder(BorderFactory.createEmptyBorder(15, 15, 15, 15));
        panel.add(new JLabel("Appearance settings - colors and fonts"), 
                  BorderLayout.NORTH);
        return panel;
    }
    
    private void createTaskbar() {
        taskbar = new JPanel(new BorderLayout());
        taskbar.setBackground(LIGHT_GRAY);
        taskbar.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createMatteBorder(2, 0, 0, 0, HIGHLIGHT),
            BorderFactory.createMatteBorder(1, 0, 0, 0, SHADOW)
        ));
        taskbar.setPreferredSize(new Dimension(getWidth(), TASKBAR_HEIGHT));
        
        // Left panel - Start button and workspace controls
        JPanel leftPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 5, 5));
        leftPanel.setBackground(LIGHT_GRAY);
        
        startButton = createIRIXButton("Start", new Dimension(80, 28));
        startButton.setFont(new Font("SansSerif", Font.BOLD, 11));
        startButton.addActionListener(e -> {
            playClickSound();
            toggleStartMenu();
        });
        
        workspaceLabel = new JLabel("  Workspace 1  ");
        workspaceLabel.setFont(new Font("Monospaced", Font.BOLD, 11));
        workspaceLabel.setOpaque(true);
        workspaceLabel.setBackground(new Color(230, 230, 230));
        workspaceLabel.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(SHADOW, 1),
            BorderFactory.createEmptyBorder(3, 5, 3, 5)
        ));
        
        JButton workspaceButton = createIRIXButton("WS", new Dimension(45, 28));
        workspaceButton.addActionListener(e -> showWorkspaceManager());
        
        leftPanel.add(startButton);
        leftPanel.add(workspaceLabel);
        leftPanel.add(workspaceButton);
        
        // Center panel for window buttons
        windowButtonPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 3, 5));
        windowButtonPanel.setBackground(LIGHT_GRAY);
        
        // Right panel - System tray and clock
        JPanel rightPanel = createSystemTray();
        
        taskbar.add(leftPanel, BorderLayout.WEST);
        taskbar.add(windowButtonPanel, BorderLayout.CENTER);
        taskbar.add(rightPanel, BorderLayout.EAST);
    }
    
    private JPanel createSystemTray() {
        JPanel rightPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT, 5, 5));
        rightPanel.setBackground(LIGHT_GRAY);
        
        JLabel clock = new JLabel();
        clock.setFont(new Font("Monospaced", Font.BOLD, 11));
        clock.setOpaque(true);
        clock.setBackground(new Color(230, 230, 230));
        clock.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(SHADOW, 1),
            BorderFactory.createEmptyBorder(3, 8, 3, 8)
        ));
        startClock(clock);
        
        JButton soundIcon = createSystemTrayIcon("\uD83D\uDD0A", "Volume Control");
        JButton networkIcon = createSystemTrayIcon("\uD83C\uDF10", "Network Status");
        
        JButton shutdownButton = createIRIXButton("Shutdown", new Dimension(90, 28));
        shutdownButton.addActionListener(e -> showShutdownDialog());
        
        rightPanel.add(soundIcon);
        rightPanel.add(networkIcon);
        rightPanel.add(clock);
        rightPanel.add(shutdownButton);
        
        return rightPanel;
    }
    
    private JButton createSystemTrayIcon(String emoji, String tooltip) {
        JButton btn = new JButton(emoji);
        btn.setPreferredSize(new Dimension(30, 28));
        btn.setBorder(BorderFactory.createEmptyBorder());
        btn.setContentAreaFilled(false);
        btn.setFocusPainted(false);
        btn.setToolTipText(tooltip);
        return btn;
    }
    
    /**
     * Creates an IRIX-style 3D beveled button
     */
    public JButton createIRIXButton(String text, Dimension size) {
        JButton button = new JButton(text) {
            @Override
            protected void paintComponent(Graphics g) {
                Graphics2D g2d = (Graphics2D) g;
                g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, 
                                     RenderingHints.VALUE_ANTIALIAS_ON);
                
                if (getModel().isPressed()) {
                    paintPressedState(g2d);
                } else {
                    paintRaisedState(g2d);
                }
                
                // Text
                g2d.setColor(Color.BLACK);
                g2d.setFont(getFont());
                FontMetrics fm = g2d.getFontMetrics();
                int x = (getWidth() - fm.stringWidth(getText())) / 2;
                int y = (getHeight() - fm.getHeight()) / 2 + fm.getAscent();
                g2d.drawString(getText(), x, y);
            }
            
            private void paintPressedState(Graphics2D g2d) {
                g2d.setColor(DARK_GRAY);
                g2d.fillRect(0, 0, getWidth(), getHeight());
                g2d.setColor(SHADOW);
                g2d.drawLine(0, 0, getWidth()-1, 0);
                g2d.drawLine(0, 0, 0, getHeight()-1);
                g2d.setColor(HIGHLIGHT);
                g2d.drawLine(getWidth()-1, 1, getWidth()-1, getHeight()-1);
                g2d.drawLine(1, getHeight()-1, getWidth()-1, getHeight()-1);
            }
            
            private void paintRaisedState(Graphics2D g2d) {
                g2d.setColor(LIGHT_GRAY);
                g2d.fillRect(0, 0, getWidth(), getHeight());
                
                // Highlight
                g2d.setColor(HIGHLIGHT);
                g2d.drawLine(0, 0, getWidth()-2, 0);
                g2d.drawLine(0, 0, 0, getHeight()-2);
                g2d.drawLine(1, 1, getWidth()-3, 1);
                g2d.drawLine(1, 1, 1, getHeight()-3);
                
                // Shadow
                g2d.setColor(SHADOW);
                g2d.drawLine(0, getHeight()-1, getWidth()-1, getHeight()-1);
                g2d.drawLine(getWidth()-1, 0, getWidth()-1, getHeight()-1);
                g2d.setColor(DARK_GRAY);
                g2d.drawLine(1, getHeight()-2, getWidth()-2, getHeight()-2);
                g2d.drawLine(getWidth()-2, 1, getWidth()-2, getHeight()-2);
            }
        };
        
        button.setPreferredSize(size);
        button.setBorder(BorderFactory.createEmptyBorder());
        button.setContentAreaFilled(false);
        button.setFocusPainted(false);
        button.setFont(new Font("SansSerif", Font.BOLD, 10));
        
        return button;
    }
    
    private void startClock(JLabel clock) {
        SimpleDateFormat sdf = new SimpleDateFormat("EEE HH:mm:ss");
        clock.setText(" " + sdf.format(new Date()) + " ");
        
        Timer timer = new Timer(1000, e -> 
            clock.setText(" " + sdf.format(new Date()) + " ")
        );
        timer.start();
        registerTimer(timer);
    }
    
    private void createDesktopIcons() {
        // Define desktop icons configuration
        Object[][] iconConfig = {
            {"System\nManager", "system", (Runnable) this::openSystemManager},
            {"Terminal", "terminal", (Runnable) this::openTerminal},
            {"File\nManager", "files", (Runnable) this::openFileManager},
            {"Text\nEditor", "editor", (Runnable) this::openTextEditor},
            {"Web\nBrowser", "browser", (Runnable) this::openWebBrowser},
            {"Calculator", "calculator", (Runnable) this::openCalculator},
            {"Drawing\nTool", "draw", (Runnable) this::openDrawingApp},
            {"Media\nPlayer", "media", (Runnable) this::openMediaPlayer}
        };
        
        int y = 20;
        for (Object[] config : iconConfig) {
            String name = (String) config[0];
            String iconKey = (String) config[1];
            Runnable action = (Runnable) config[2];
            createDesktopIcon(name, iconKey, 20, y, e -> action.run());
            y += ICON_SPACING;
        }
    }
    
    private void createDesktopIcon(String name, String iconKey, int x, int y, 
                                   ActionListener action) {
        JButton icon = new JButton() {
            @Override
            protected void paintComponent(Graphics g) {
                Graphics2D g2d = (Graphics2D) g;
                g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, 
                                     RenderingHints.VALUE_ANTIALIAS_ON);
                
                // Selection/hover background
                if (getModel().isRollover()) {
                    g2d.setColor(new Color(255, 255, 255, 40));
                    g2d.fillRoundRect(2, 2, getWidth()-4, getHeight()-4, 8, 8);
                    g2d.setColor(new Color(255, 255, 255, 100));
                    g2d.drawRoundRect(2, 2, getWidth()-4, getHeight()-4, 8, 8);
                }
                
                // Draw icon image
                ImageIcon iconImg = icons.get(iconKey);
                if (iconImg != null) {
                    int iconX = (getWidth() - iconImg.getIconWidth()) / 2;
                    g2d.drawImage(iconImg.getImage(), iconX, 8, this);
                }
                
                // Draw text with shadow
                paintIconText(g2d, name);
            }
            
            private void paintIconText(Graphics2D g2d, String name) {
                g2d.setFont(getFont());
                FontMetrics fm = g2d.getFontMetrics();
                String[] lines = name.split("\n");
                int textY = 52;
                
                for (String line : lines) {
                    int textWidth = fm.stringWidth(line);
                    int textX = (getWidth() - textWidth) / 2;
                    
                    // Shadow
                    g2d.setColor(new Color(0, 0, 0, 100));
                    g2d.drawString(line, textX + 1, textY + 1);
                    
                    // Text
                    g2d.setColor(Color.WHITE);
                    g2d.drawString(line, textX, textY);
                    
                    textY += fm.getHeight();
                }
            }
        };
        
        icon.setBounds(x, y, 90, 75);
        icon.setBackground(new Color(0, 0, 0, 0));
        icon.setBorder(BorderFactory.createEmptyBorder());
        icon.setFocusPainted(false);
        icon.setContentAreaFilled(false);
        icon.setForeground(Color.WHITE);
        icon.setFont(new Font("SansSerif", Font.BOLD, 11));
        
        // Double-click support with proper timer management
        icon.addMouseListener(new DoubleClickHandler(action));
        
        desktop.add(icon);
    }
    
    /**
     * Handler for double-click detection on desktop icons
     */
    private class DoubleClickHandler extends MouseAdapter {
        private int clickCount = 0;
        private final Timer clickTimer;
        private final ActionListener action;
        
        DoubleClickHandler(ActionListener action) {
            this.action = action;
            this.clickTimer = new Timer(DOUBLE_CLICK_DELAY, e -> clickCount = 0);
            clickTimer.setRepeats(false);
            registerTimer(clickTimer);
        }
        
        @Override
        public void mouseClicked(MouseEvent e) {
            clickCount++;
            if (clickCount == 1) {
                clickTimer.restart();
            } else if (clickCount >= 2) {
                clickTimer.stop();
                clickCount = 0;
                action.actionPerformed(new ActionEvent(e.getSource(), 
                    ActionEvent.ACTION_PERFORMED, null));
            }
        }
    }
    
    private void createStartMenu() {
        startMenu = new StartMenu(this);
    }
    
    private void toggleStartMenu() {
        if (startMenu.isVisible()) {
            startMenu.setVisible(false);
        } else {
            Point location = startButton.getLocationOnScreen();
            startMenu.setLocation(location.x, location.y - startMenu.getHeight());
            startMenu.setVisible(true);
            startMenu.toFront();
        }
    }
    
    private void setupKeyboardShortcuts() {
        // Terminal shortcut (Ctrl+Alt+T)
        registerKeyboardShortcut(KeyEvent.VK_T, 
            InputEvent.CTRL_DOWN_MASK | InputEvent.ALT_DOWN_MASK,
            "openTerminal", this::openTerminal);
        
        // Workspace shortcuts (Ctrl+Alt+1-4)
        for (int i = 0; i < NUM_WORKSPACES; i++) {
            final int workspace = i;
            registerKeyboardShortcut(KeyEvent.VK_1 + i,
                InputEvent.CTRL_DOWN_MASK | InputEvent.ALT_DOWN_MASK,
                "workspace" + i, () -> switchWorkspace(workspace));
        }
    }
    
    private void registerKeyboardShortcut(int keyCode, int modifiers, 
                                          String actionName, Runnable action) {
        KeyStroke key = KeyStroke.getKeyStroke(keyCode, modifiers);
        desktop.getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW).put(key, actionName);
        desktop.getActionMap().put(actionName, new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                action.run();
            }
        });
    }
    
    private void loadIcons() {
        icons.put("system", IconFactory.createSystemIcon());
        icons.put("terminal", IconFactory.createTerminalIcon());
        icons.put("files", IconFactory.createFileManagerIcon());
        icons.put("editor", IconFactory.createTextEditorIcon());
        icons.put("browser", IconFactory.createBrowserIcon());
        icons.put("calculator", IconFactory.createCalculatorIcon());
        icons.put("draw", IconFactory.createDrawingIcon());
        icons.put("media", IconFactory.createMediaIcon());
    }
    
    // ========== Sound Methods ==========
    
    public void playClickSound() {
        Toolkit.getDefaultToolkit().beep();
    }
    
    private void playWindowOpenSound() {
        // Could implement custom sound here
    }
    
    private void playWindowCloseSound() {
        // Could implement custom sound here
    }
    
    // ========== Application Launchers ==========
    
    public void openSystemManager() {
        playWindowOpenSound();
        IRIXWindow window = new IRIXWindow("System Manager", 650, 550, this);
        
        JTabbedPane tabs = new JTabbedPane();
        tabs.setFont(new Font("SansSerif", Font.PLAIN, 11));
        tabs.addTab("System Info", createSystemInfoPanel());
        tabs.addTab("Processes", createProcessMonitor());
        tabs.addTab("Performance", createPerformancePanel());
        tabs.addTab("Users", createUsersPanel());
        tabs.addTab("Network", createNetworkPanel());
        
        window.getContentPane().add(tabs);
        showWindow(window);
    }
    
    public void openTerminal() {
        playWindowOpenSound();
        IRIXWindow window = new IRIXWindow("Terminal", 650, 450, this);
        window.getContentPane().add(new TerminalEmulator());
        showWindow(window);
    }
    
    public void openFileManager() {
        playWindowOpenSound();
        IRIXWindow window = new IRIXWindow("File Manager", 700, 500, this);
        window.getContentPane().add(new FileSystemBrowser());
        showWindow(window);
    }
    
    public void openTextEditor() {
        playWindowOpenSound();
        IRIXWindow window = new IRIXWindow("Text Editor", 700, 550, this);
        window.getContentPane().setLayout(new BorderLayout());
        
        TextEditor editor = new TextEditor();
        JMenuBar menuBar = editor.getMenuBar();
        if (menuBar != null) {
            window.setJMenuBar(menuBar);
        }
        
        window.getContentPane().add(new JScrollPane(editor), BorderLayout.CENTER);
        showWindow(window);
        editor.requestFocus();
    }
    
    public void openWebBrowser() {
        playWindowOpenSound();
        IRIXWindow window = new IRIXWindow("Web Browser", 850, 650, this);
        window.getContentPane().add(new WebBrowser());
        showWindow(window);
    }
    
    public void openCalculator() {
        playWindowOpenSound();
        IRIXWindow window = new IRIXWindow("Calculator", 320, 420, this);
        window.getContentPane().add(new Calculator());
        showWindow(window);
    }
    
    public void openDrawingApp() {
        playWindowOpenSound();
        IRIXWindow window = new IRIXWindow("Drawing Tool", 700, 550, this);
        window.getContentPane().setLayout(new BorderLayout());
        window.getContentPane().add(new DrawingApp(), BorderLayout.CENTER);
        showWindow(window);
    }
    
    public void openMediaPlayer() {
        playWindowOpenSound();
        IRIXWindow window = new IRIXWindow("Media Player", 450, 350, this);
        window.getContentPane().add(new MediaPlayer());
        showWindow(window);
    }
    
    // ========== System Manager Panels ==========
    
    private JPanel createSystemInfoPanel() {
        JPanel panel = new JPanel(new BorderLayout());
        JTextArea info = new JTextArea();
        info.setEditable(false);
        info.setFont(new Font("Monospaced", Font.PLAIN, 12));
        
        Runtime runtime = Runtime.getRuntime();
        long maxMemory = runtime.maxMemory() / 1024 / 1024;
        long totalMemory = runtime.totalMemory() / 1024 / 1024;
        long freeMemory = runtime.freeMemory() / 1024 / 1024;
        long usedMemory = totalMemory - freeMemory;
        
        Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
        
        String systemInfo = String.format(
            "═══════════════════════════════════════════%n" +
            "    IRIX System Information%n" +
            "═══════════════════════════════════════════%n%n" +
            "Operating System%n" +
            "  OS Name:           IRIX 6.5%n" +
            "  Host:              %s%n" +
            "  Machine Type:      SGI Indy%n" +
            "  Processor:         MIPS R5000 @ 150MHz%n%n" +
            "Graphics System%n" +
            "  Graphics:          Newport (XL-8)%n" +
            "  Resolution:        %dx%d%n" +
            "  Color Depth:       24-bit%n%n" +
            "Memory Information%n" +
            "  Physical RAM:      128 MB%n" +
            "  Java Max Memory:   %d MB%n" +
            "  Java Used Memory:  %d MB%n" +
            "  Java Free Memory:  %d MB%n%n" +
            "System Status%n" +
            "  Uptime:            %d minutes%n" +
            "  Java Version:      %s%n" +
            "  Java Vendor:       %s%n%n" +
            "User Information%n" +
            "  User:              %s%n" +
            "  Home:              %s%n" +
            "  Working Dir:       %s%n",
            getHostName(),
            screenSize.width, screenSize.height,
            maxMemory, usedMemory, freeMemory,
            System.currentTimeMillis() / 1000 / 60,
            System.getProperty("java.version"),
            System.getProperty("java.vendor"),
            System.getProperty("user.name"),
            System.getProperty("user.home"),
            System.getProperty("user.dir")
        );
        
        info.setText(systemInfo);
        panel.add(new JScrollPane(info), BorderLayout.CENTER);
        return panel;
    }
    
    private String getHostName() {
        try {
            return java.net.InetAddress.getLocalHost().getHostName();
        } catch (Exception e) {
            return "localhost";
        }
    }
    
    private JPanel createProcessMonitor() {
        JPanel panel = new JPanel(new BorderLayout());
        
        String[] columnNames = {"PID", "Process Name", "CPU %", "Memory", "Status"};
        Object[][] data = {
            {"1", "init", "0.0", "2.1 MB", "Running"},
            {"2", "kthreadd", "0.0", "0 KB", "Sleeping"},
            {"3", "winman", "1.2", "15.4 MB", "Running"},
            {"4", "desktop", "2.5", "45.2 MB", "Running"},
            {"5", "terminal", "0.5", "8.3 MB", "Running"},
            {"6", "filesys", "0.1", "12.1 MB", "Sleeping"},
            {"7", "netman", "0.3", "6.8 MB", "Running"},
            {"8", "soundd", "0.0", "3.2 MB", "Sleeping"}
        };
        
        JTable table = new JTable(data, columnNames);
        table.setFont(new Font("Monospaced", Font.PLAIN, 11));
        table.getTableHeader().setFont(new Font("SansSerif", Font.BOLD, 11));
        
        panel.add(new JScrollPane(table), BorderLayout.CENTER);
        
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        JButton refreshBtn = new JButton("Refresh");
        JButton killBtn = new JButton("Kill Process");
        
        refreshBtn.addActionListener(e -> 
            JOptionPane.showMessageDialog(panel, "Process list refreshed"));
        
        killBtn.addActionListener(e -> killSelectedProcess(table, panel));
        
        buttonPanel.add(refreshBtn);
        buttonPanel.add(killBtn);
        panel.add(buttonPanel, BorderLayout.SOUTH);
        
        return panel;
    }
    
    private void killSelectedProcess(JTable table, JPanel panel) {
        int row = table.getSelectedRow();
        if (row >= 0) {
            String pid = (String) table.getValueAt(row, 0);
            String name = (String) table.getValueAt(row, 1);
            int confirm = JOptionPane.showConfirmDialog(panel,
                "Kill process " + name + " (PID: " + pid + ")?",
                "Confirm Kill",
                JOptionPane.YES_NO_OPTION);
            if (confirm == JOptionPane.YES_OPTION) {
                JOptionPane.showMessageDialog(panel, "Process terminated");
            }
        } else {
            JOptionPane.showMessageDialog(panel, "Please select a process");
        }
    }
    
    private JPanel createPerformancePanel() {
        JPanel panel = new JPanel(new GridLayout(3, 1, 10, 10));
        panel.setBorder(BorderFactory.createEmptyBorder(15, 15, 15, 15));
        
        JProgressBar cpuBar = createPerformanceBar("CPU Usage", "25% - MIPS R5000", 
                                                    25, new Color(0, 150, 0));
        JProgressBar memBar = createPerformanceBar("Memory Usage", "60% - 77/128 MB", 
                                                    60, new Color(0, 120, 200));
        JProgressBar diskBar = createPerformanceBar("Disk Usage", "45% - 4.5/10 GB", 
                                                     45, new Color(200, 100, 0));
        
        panel.add(wrapInTitledPanel(cpuBar, "CPU Usage"));
        panel.add(wrapInTitledPanel(memBar, "Memory Usage"));
        panel.add(wrapInTitledPanel(diskBar, "Disk Usage"));
        
        // Animate CPU and memory
        Timer timer = new Timer(2000, e -> {
            int cpu = 20 + (int)(Math.random() * 30);
            int mem = 50 + (int)(Math.random() * 25);
            cpuBar.setValue(cpu);
            cpuBar.setString(cpu + "% - MIPS R5000");
            memBar.setValue(mem);
            memBar.setString(mem + "% - " + (mem * 128 / 100) + "/128 MB");
        });
        timer.start();
        registerTimer(timer);
        
        return panel;
    }
    
    private JProgressBar createPerformanceBar(String name, String text, 
                                              int value, Color color) {
        JProgressBar bar = new JProgressBar(0, 100);
        bar.setString(text);
        bar.setStringPainted(true);
        bar.setValue(value);
        bar.setForeground(color);
        return bar;
    }
    
    private JPanel wrapInTitledPanel(JComponent component, String title) {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBorder(BorderFactory.createTitledBorder(title));
        panel.add(component, BorderLayout.CENTER);
        return panel;
    }
    
    private JPanel createUsersPanel() {
        JPanel panel = new JPanel(new BorderLayout());
        JTextArea usersInfo = new JTextArea();
        usersInfo.setEditable(false);
        usersInfo.setFont(new Font("Monospaced", Font.PLAIN, 12));
        usersInfo.setText("Logged in users:\n\n" +
                         System.getProperty("user.name") + "\t\ttty1\t\t" + new Date() + "\n");
        panel.add(new JScrollPane(usersInfo), BorderLayout.CENTER);
        return panel;
    }
    
    private JPanel createNetworkPanel() {
        JPanel panel = new JPanel(new BorderLayout());
        JTextArea networkInfo = new JTextArea();
        networkInfo.setEditable(false);
        networkInfo.setFont(new Font("Monospaced", Font.PLAIN, 12));
        networkInfo.setText("Network Interfaces:\n\n" +
                           "eth0: 192.168.1.100\n" +
                           "lo: 127.0.0.1\n\n" +
                           "Status: Connected\n");
        panel.add(new JScrollPane(networkInfo), BorderLayout.CENTER);
        return panel;
    }
    
    // ========== Window Management ==========
    
    private void showWindow(IRIXWindow window) {
        desktop.add(window);
        window.setVisible(true);
        windows.add(window);
        
        // Center window
        Dimension desktopSize = desktop.getSize();
        Dimension windowSize = window.getSize();
        window.setLocation(
            (desktopSize.width - windowSize.width) / 2,
            (desktopSize.height - windowSize.height) / 2
        );
        
        window.toFront();
        try {
            window.setSelected(true);
        } catch (java.beans.PropertyVetoException e) {
            // Ignore
        }
        
        addWindowButton(window);
    }
    
    private void addWindowButton(IRIXWindow window) {
        JButton winButton = createIRIXButton(window.getTitle(), new Dimension(120, 28));
        winButton.setFont(new Font("SansSerif", Font.PLAIN, 10));
        winButton.addActionListener(e -> {
            try {
                window.setIcon(false);
                window.setSelected(true);
                window.toFront();
            } catch (Exception ex) {
                // Ignore
            }
        });
        
        windowButtonPanel.add(winButton);
        windowButtonPanel.revalidate();
        windowButtonPanel.repaint();
        
        window.putClientProperty("taskbarButton", winButton);
    }
    
    public void removeWindow(IRIXWindow window) {
        windows.remove(window);
        playWindowCloseSound();
        
        JButton btn = (JButton) window.getClientProperty("taskbarButton");
        if (btn != null) {
            windowButtonPanel.remove(btn);
            windowButtonPanel.revalidate();
            windowButtonPanel.repaint();
        }
    }
    
    private void showWorkspaceManager() {
        playClickSound();
        workspaceManager.setVisible(true);
        workspaceManager.setLocationRelativeTo(this);
    }
    
    public void switchWorkspace(int newWorkspace) {
        currentWorkspace = newWorkspace;
        workspaceLabel.setText("  Workspace " + (newWorkspace + 1) + "  ");
        workspaceManager.setCurrentWorkspace(newWorkspace);
        
        for (IRIXWindow window : windows) {
            boolean visible = (window.getWorkspace() == newWorkspace);
            window.setVisible(visible);
        }
    }
    
    public void showShutdownDialog() {
        playClickSound();
        
        JDialog shutdownDialog = new JDialog(this, "Shutdown System", true);
        shutdownDialog.setLayout(new BorderLayout(10, 10));
        shutdownDialog.setSize(400, 200);
        shutdownDialog.setLocationRelativeTo(this);
        
        JPanel messagePanel = new JPanel(new BorderLayout());
        messagePanel.setBorder(BorderFactory.createEmptyBorder(20, 20, 10, 20));
        
        JLabel icon = new JLabel("\u26A0\uFE0F");
        icon.setFont(new Font("SansSerif", Font.PLAIN, 48));
        icon.setHorizontalAlignment(SwingConstants.CENTER);
        
        JLabel message = new JLabel(
            "<html><center>Are you sure you want to<br>shut down the system?</center></html>");
        message.setFont(new Font("SansSerif", Font.PLAIN, 14));
        message.setHorizontalAlignment(SwingConstants.CENTER);
        
        messagePanel.add(icon, BorderLayout.WEST);
        messagePanel.add(message, BorderLayout.CENTER);
        
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.CENTER, 10, 10));
        
        JButton shutdownBtn = createIRIXButton("Shutdown", new Dimension(100, 30));
        JButton restartBtn = createIRIXButton("Restart", new Dimension(100, 30));
        JButton cancelBtn = createIRIXButton("Cancel", new Dimension(100, 30));
        
        shutdownBtn.addActionListener(e -> {
            shutdownDialog.dispose();
            JOptionPane.showMessageDialog(this, "System shutting down...");
            System.exit(0);
        });
        
        restartBtn.addActionListener(e -> {
            shutdownDialog.dispose();
            JOptionPane.showMessageDialog(this, 
                "System restarting...\n(Would restart in real system)");
        });
        
        cancelBtn.addActionListener(e -> shutdownDialog.dispose());
        
        buttonPanel.add(shutdownBtn);
        buttonPanel.add(restartBtn);
        buttonPanel.add(cancelBtn);
        
        shutdownDialog.add(messagePanel, BorderLayout.CENTER);
        shutdownDialog.add(buttonPanel, BorderLayout.SOUTH);
        shutdownDialog.setVisible(true);
    }
    
    public Clipboard getClipboard() {
        return clipboard;
    }
    
    public int getCurrentWorkspace() {
        return currentWorkspace;
    }
    
    // ========== Main Entry Point ==========
    
    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            try {
                UIManager.setLookAndFeel(UIManager.getCrossPlatformLookAndFeelClassName());
            } catch (Exception e) {
                e.printStackTrace();
            }
            
            IRIXDesktop desktop = new IRIXDesktop();
            desktop.setVisible(true);
        });
    }
}

/**
 * Factory class for creating IRIX-style icons programmatically
 */
class IconFactory {
    private static final int ICON_SIZE = 48;
    
    public static ImageIcon createSystemIcon() {
        BufferedImage img = new BufferedImage(ICON_SIZE, ICON_SIZE, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = createGraphics(img);
        
        // Computer monitor
        g.setColor(new Color(100, 100, 120));
        g.fillRoundRect(4, 4, 40, 30, 4, 4);
        g.setColor(new Color(0, 180, 255));
        g.fillRect(7, 7, 34, 24);
        
        // Screen reflection
        g.setColor(new Color(255, 255, 255, 100));
        g.fillRect(9, 9, 15, 10);
        
        // Base
        g.setColor(new Color(80, 80, 100));
        g.fillRect(18, 34, 12, 4);
        g.fillRoundRect(12, 38, 24, 6, 3, 3);
        
        // Settings symbol
        g.setColor(Color.WHITE);
        g.setFont(new Font("SansSerif", Font.BOLD, 16));
        g.drawString("\u2699", 18, 24);
        
        g.dispose();
        return new ImageIcon(img);
    }
    
    public static ImageIcon createTerminalIcon() {
        BufferedImage img = new BufferedImage(ICON_SIZE, ICON_SIZE, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = createGraphics(img);
        
        // Window
        g.setColor(new Color(40, 40, 40));
        g.fillRoundRect(4, 4, 40, 40, 6, 6);
        
        // Screen
        g.setColor(Color.BLACK);
        g.fillRect(7, 10, 34, 31);
        
        // Terminal prompt
        g.setColor(Color.GREEN);
        g.setFont(new Font("Monospaced", Font.BOLD, 10));
        g.drawString(">_", 12, 22);
        g.drawString(">", 12, 32);
        
        // Title bar
        g.setColor(IRIXDesktop.IRIS_BLUE);
        g.fillRect(7, 7, 34, 3);
        
        g.dispose();
        return new ImageIcon(img);
    }
    
    public static ImageIcon createFileManagerIcon() {
        BufferedImage img = new BufferedImage(ICON_SIZE, ICON_SIZE, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = createGraphics(img);
        
        // Folder back
        g.setColor(new Color(200, 160, 80));
        int[] xPoints = {8, 20, 40, 40, 8};
        int[] yPoints = {18, 18, 20, 40, 40};
        g.fillPolygon(xPoints, yPoints, 5);
        
        // Folder front
        g.setColor(new Color(240, 200, 100));
        int[] xFront = {8, 18, 18, 36, 36, 8};
        int[] yFront = {20, 20, 16, 16, 38, 38};
        g.fillPolygon(xFront, yFront, 6);
        
        // Tab
        g.setColor(new Color(220, 180, 90));
        g.fillRect(8, 16, 10, 4);
        
        g.dispose();
        return new ImageIcon(img);
    }
    
    public static ImageIcon createTextEditorIcon() {
        BufferedImage img = new BufferedImage(ICON_SIZE, ICON_SIZE, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = createGraphics(img);
        
        // Document
        g.setColor(Color.WHITE);
        g.fillRoundRect(10, 6, 28, 36, 4, 4);
        
        // Lines of text
        g.setColor(IRIXDesktop.IRIS_BLUE);
        g.setStroke(new BasicStroke(2));
        g.drawLine(15, 14, 30, 14);
        g.drawLine(15, 20, 33, 20);
        g.drawLine(15, 26, 28, 26);
        g.drawLine(15, 32, 32, 32);
        
        // Border
        g.setColor(new Color(150, 150, 150));
        g.setStroke(new BasicStroke(1));
        g.drawRoundRect(10, 6, 28, 36, 4, 4);
        
        g.dispose();
        return new ImageIcon(img);
    }
    
    public static ImageIcon createBrowserIcon() {
        BufferedImage img = new BufferedImage(ICON_SIZE, ICON_SIZE, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = createGraphics(img);
        
        // Globe
        g.setColor(new Color(0, 120, 200));
        g.fillOval(8, 8, 32, 32);
        
        // Continents
        g.setColor(new Color(0, 150, 80));
        g.fillOval(12, 15, 10, 8);
        g.fillOval(20, 12, 12, 10);
        g.fillOval(14, 26, 8, 10);
        g.fillOval(26, 24, 10, 12);
        
        // Meridians
        g.setColor(new Color(255, 255, 255, 100));
        g.setStroke(new BasicStroke(1.5f));
        g.drawOval(16, 8, 16, 32);
        g.drawLine(8, 24, 40, 24);
        
        g.dispose();
        return new ImageIcon(img);
    }
    
    public static ImageIcon createCalculatorIcon() {
        BufferedImage img = new BufferedImage(ICON_SIZE, ICON_SIZE, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = createGraphics(img);
        
        // Calculator body
        g.setColor(new Color(80, 80, 90));
        g.fillRoundRect(8, 6, 32, 36, 4, 4);
        
        // Display
        g.setColor(new Color(180, 220, 180));
        g.fillRect(12, 10, 24, 8);
        g.setColor(Color.BLACK);
        g.setFont(new Font("Monospaced", Font.BOLD, 8));
        g.drawString("1234", 14, 16);
        
        // Buttons
        g.setColor(new Color(120, 120, 130));
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                g.fillRoundRect(12 + col * 8, 22 + row * 6, 6, 5, 2, 2);
            }
        }
        
        g.dispose();
        return new ImageIcon(img);
    }
    
    public static ImageIcon createDrawingIcon() {
        BufferedImage img = new BufferedImage(ICON_SIZE, ICON_SIZE, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = createGraphics(img);
        
        // Canvas
        g.setColor(Color.WHITE);
        g.fillRect(6, 10, 36, 32);
        
        // Brush strokes
        g.setStroke(new BasicStroke(3, BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND));
        g.setColor(Color.RED);
        g.drawLine(12, 18, 20, 26);
        g.setColor(Color.BLUE);
        g.drawOval(22, 16, 12, 12);
        g.setColor(Color.GREEN);
        g.drawRect(14, 28, 10, 8);
        
        // Palette
        g.setColor(new Color(139, 90, 43));
        g.fillOval(28, 28, 12, 10);
        g.setColor(Color.RED);
        g.fillOval(30, 30, 3, 3);
        g.setColor(Color.BLUE);
        g.fillOval(34, 30, 3, 3);
        g.setColor(Color.GREEN);
        g.fillOval(32, 34, 3, 3);
        
        g.dispose();
        return new ImageIcon(img);
    }
    
    public static ImageIcon createMediaIcon() {
        BufferedImage img = new BufferedImage(ICON_SIZE, ICON_SIZE, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g = createGraphics(img);
        
        // CD/DVD disc
        g.setColor(new Color(200, 200, 220));
        g.fillOval(8, 8, 32, 32);
        
        // Reflections
        GradientPaint gradient = new GradientPaint(
            12, 12, new Color(255, 200, 255, 100),
            28, 28, new Color(200, 255, 255, 100)
        );
        g.setPaint(gradient);
        g.fillOval(10, 10, 28, 28);
        
        // Center hole
        g.setColor(Color.BLACK);
        g.fillOval(20, 20, 8, 8);
        g.setColor(new Color(100, 100, 100));
        g.fillOval(21, 21, 6, 6);
        
        // Play button
        g.setColor(new Color(0, 180, 0));
        int[] xPlay = {28, 38, 28};
        int[] yPlay = {30, 36, 42};
        g.fillPolygon(xPlay, yPlay, 3);
        
        g.dispose();
        return new ImageIcon(img);
    }
    
    private static Graphics2D createGraphics(BufferedImage img) {
        Graphics2D g = img.createGraphics();
        g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        return g;
    }
}
