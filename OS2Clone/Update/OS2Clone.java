import javax.swing.*;
import javax.swing.border.*;
import javax.swing.event.*;
import javax.swing.plaf.basic.*;
import javax.swing.tree.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.*;
import java.io.*;
import java.util.*;
import java.util.List;

/**
 * OS/2 Warp Clone - A nostalgic recreation of the OS/2 Workplace Shell
 */
public class OS2Clone {
    private JFrame desktop;
    private JDesktopPane desktopPane;
    private WorkplaceShell workplaceShell;
    private JPanel taskbar;
    
    // OS/2 Color Palette
    public static final Color OS2_DESKTOP = new Color(0x008080);  // Teal desktop
    public static final Color OS2_GRAY = new Color(0xC0C0C0);
    public static final Color OS2_DARK_GRAY = new Color(0x808080);
    public static final Color OS2_BLUE = new Color(0x000080);
    public static final Color OS2_LIGHT = new Color(0xDFDFDF);
    public static final Color OS2_SHADOW = new Color(0x404040);
    
    public static void main(String[] args) {
        // Set OS/2 look
        try {
            UIManager.setLookAndFeel(UIManager.getCrossPlatformLookAndFeelClassName());
            configureUIDefaults();
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        SwingUtilities.invokeLater(() -> new OS2Clone().start());
    }
    
    private static void configureUIDefaults() {
        UIManager.put("Panel.background", OS2_GRAY);
        UIManager.put("Button.background", OS2_GRAY);
        UIManager.put("InternalFrame.activeTitleBackground", OS2_BLUE);
        UIManager.put("InternalFrame.activeTitleForeground", Color.WHITE);
        UIManager.put("InternalFrame.inactiveTitleBackground", OS2_DARK_GRAY);
        UIManager.put("InternalFrame.inactiveTitleForeground", OS2_GRAY);
        UIManager.put("Desktop.background", OS2_DESKTOP);
    }
    
    public void start() {
        createDesktopEnvironment();
    }
    
    private void createDesktopEnvironment() {
        desktop = new JFrame("OS/2 Warp Clone");
        desktop.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        desktop.setExtendedState(JFrame.MAXIMIZED_BOTH);
        desktop.setLayout(new BorderLayout());
        
        // Create desktop pane
        desktopPane = new JDesktopPane();
        desktopPane.setBackground(OS2_DESKTOP);
        desktopPane.setDragMode(JDesktopPane.OUTLINE_DRAG_MODE);
        
        // Initialize Workplace Shell (icons)
        workplaceShell = new WorkplaceShell(desktopPane, this);
        
        // Create taskbar
        taskbar = createTaskbar();
        
        desktop.add(desktopPane, BorderLayout.CENTER);
        desktop.add(taskbar, BorderLayout.SOUTH);
        
        desktop.setVisible(true);
    }
    
    private JPanel createTaskbar() {
        JPanel bar = new JPanel(new BorderLayout());
        bar.setPreferredSize(new Dimension(0, 32));
        bar.setBackground(OS2_GRAY);
        bar.setBorder(new OS2Border(true));
        
        // Start button (LaunchPad style)
        JButton startBtn = createOS2Button("OS/2 Warp");
        startBtn.setPreferredSize(new Dimension(100, 28));
        startBtn.addActionListener(e -> showStartMenu(startBtn));
        
        // Window list area
        JPanel windowList = new JPanel(new FlowLayout(FlowLayout.LEFT, 2, 2));
        windowList.setOpaque(false);
        
        // Clock
        JLabel clock = new JLabel();
        clock.setPreferredSize(new Dimension(60, 28));
        clock.setHorizontalAlignment(SwingConstants.CENTER);
        clock.setBorder(new OS2Border(false));
        updateClock(clock);
        
        // Timer for clock
        Timer clockTimer = new Timer(1000, e -> updateClock(clock));
        clockTimer.start();
        
        JPanel leftPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 2));
        leftPanel.setOpaque(false);
        leftPanel.add(startBtn);
        
        bar.add(leftPanel, BorderLayout.WEST);
        bar.add(windowList, BorderLayout.CENTER);
        bar.add(clock, BorderLayout.EAST);
        
        return bar;
    }
    
    private void updateClock(JLabel clock) {
        java.text.SimpleDateFormat sdf = new java.text.SimpleDateFormat("HH:mm");
        clock.setText(sdf.format(new java.util.Date()));
    }
    
    private void showStartMenu(Component anchor) {
        JPopupMenu menu = new JPopupMenu();
        menu.setBackground(OS2_GRAY);
        
        JMenuItem fileManager = new JMenuItem("File Manager");
        fileManager.addActionListener(e -> openFileManager());
        
        JMenuItem settings = new JMenuItem("System Setup");
        settings.addActionListener(e -> openSystemSettings());
        
        JMenuItem cmdPrompt = new JMenuItem("OS/2 Command Prompt");
        cmdPrompt.addActionListener(e -> openCommandPrompt());
        
        JMenuItem about = new JMenuItem("About OS/2");
        about.addActionListener(e -> showAbout());
        
        menu.add(fileManager);
        menu.add(settings);
        menu.add(cmdPrompt);
        menu.addSeparator();
        menu.add(about);
        
        menu.show(anchor, 0, -menu.getPreferredSize().height);
    }
    
    public void openFileManager() {
        OS2FileManager fm = new OS2FileManager();
        desktopPane.add(fm);
        fm.setVisible(true);
        try { fm.setSelected(true); } catch (Exception e) {}
    }
    
    public void openSystemSettings() {
        OS2SystemSettings ss = new OS2SystemSettings();
        desktopPane.add(ss);
        ss.setVisible(true);
        try { ss.setSelected(true); } catch (Exception e) {}
    }
    
    public void openCommandPrompt() {
        OS2CommandPrompt cmd = new OS2CommandPrompt();
        desktopPane.add(cmd);
        cmd.setVisible(true);
        try { cmd.setSelected(true); } catch (Exception e) {}
    }
    
    private void showAbout() {
        JOptionPane.showMessageDialog(desktop,
            "OS/2 Warp Clone\nVersion 1.0\n\nA nostalgic recreation of the\nOS/2 Workplace Shell",
            "About OS/2",
            JOptionPane.INFORMATION_MESSAGE);
    }
    
    public static JButton createOS2Button(String text) {
        JButton btn = new JButton(text);
        btn.setBackground(OS2_GRAY);
        btn.setFocusPainted(false);
        btn.setBorder(new OS2Border(true));
        btn.addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent e) {
                btn.setBorder(new OS2Border(false));
            }
            public void mouseReleased(MouseEvent e) {
                btn.setBorder(new OS2Border(true));
            }
        });
        return btn;
    }
}

/**
 * OS/2 Style 3D Border
 */
class OS2Border extends AbstractBorder {
    private boolean raised;
    
    public OS2Border(boolean raised) {
        this.raised = raised;
    }
    
    @Override
    public void paintBorder(Component c, Graphics g, int x, int y, int w, int h) {
        Color topLeft = raised ? OS2Clone.OS2_LIGHT : OS2Clone.OS2_SHADOW;
        Color bottomRight = raised ? OS2Clone.OS2_SHADOW : OS2Clone.OS2_LIGHT;
        
        g.setColor(topLeft);
        g.drawLine(x, y, x + w - 1, y);
        g.drawLine(x, y, x, y + h - 1);
        
        g.setColor(bottomRight);
        g.drawLine(x + w - 1, y, x + w - 1, y + h - 1);
        g.drawLine(x, y + h - 1, x + w - 1, y + h - 1);
    }
    
    @Override
    public Insets getBorderInsets(Component c) {
        return new Insets(2, 2, 2, 2);
    }
}

/**
 * Workplace Shell - Manages desktop icons
 */
class WorkplaceShell {
    private JDesktopPane desktop;
    private OS2Clone os2;
    private List<DesktopIcon> icons = new ArrayList<>();
    
    public WorkplaceShell(JDesktopPane desktop, OS2Clone os2) {
        this.desktop = desktop;
        this.os2 = os2;
        initializeDesktop();
    }
    
    private void initializeDesktop() {
        createIcon("OS/2 System", 50, 30, this::showSystemFolder);
        createIcon("Drive C:", 50, 110, () -> os2.openFileManager());
        createIcon("Command\nPrompt", 50, 190, () -> os2.openCommandPrompt());
        createIcon("Settings", 50, 270, () -> os2.openSystemSettings());
        createIcon("Information", 50, 350, this::showInfo);
    }
    
    private void createIcon(String name, int x, int y, Runnable action) {
        DesktopIcon icon = new DesktopIcon(name, x, y, action);
        icons.add(icon);
        desktop.add(icon, JLayeredPane.DEFAULT_LAYER);
    }
    
    private void showSystemFolder() {
        JOptionPane.showMessageDialog(desktop, 
            "System folder contents:\n• Startup\n• Shutdown\n• System Setup\n• Drives",
            "OS/2 System", JOptionPane.INFORMATION_MESSAGE);
    }
    
    private void showInfo() {
        JOptionPane.showMessageDialog(desktop,
            "OS/2 Warp Clone\n\nThis is a Java recreation of the classic\nOS/2 Warp 4 Workplace Shell.\n\nDouble-click icons to open.",
            "Information", JOptionPane.INFORMATION_MESSAGE);
    }
}

/**
 * Desktop Icon Component
 */
class DesktopIcon extends JComponent {
    private String name;
    private Runnable action;
    private boolean selected = false;
    private BufferedImage iconImage;
    
    public DesktopIcon(String name, int x, int y, Runnable action) {
        this.name = name;
        this.action = action;
        this.iconImage = createIconImage();
        
        setBounds(x, y, 64, 80);
        setOpaque(false);
        
        addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2 && action != null) {
                    action.run();
                }
                selected = true;
                repaint();
            }
            
            @Override
            public void mousePressed(MouseEvent e) {
                selected = true;
                repaint();
            }
        });
        
        // Deselect when clicking elsewhere
        addFocusListener(new FocusAdapter() {
            @Override
            public void focusLost(FocusEvent e) {
                selected = false;
                repaint();
            }
        });
        
        setFocusable(true);
    }
    
    private BufferedImage createIconImage() {
        BufferedImage img = new BufferedImage(32, 32, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g2d = img.createGraphics();
        g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        
        // Draw OS/2 style folder/icon
        if (name.contains("Drive")) {
            // Drive icon
            g2d.setColor(OS2Clone.OS2_GRAY);
            g2d.fillRoundRect(4, 8, 24, 18, 4, 4);
            g2d.setColor(OS2Clone.OS2_DARK_GRAY);
            g2d.drawRoundRect(4, 8, 24, 18, 4, 4);
            g2d.setColor(new Color(0x00AA00));
            g2d.fillOval(22, 12, 4, 4);
        } else if (name.contains("Command")) {
            // Terminal icon
            g2d.setColor(Color.BLACK);
            g2d.fillRect(4, 4, 24, 24);
            g2d.setColor(Color.WHITE);
            g2d.drawRect(4, 4, 24, 24);
            g2d.setColor(new Color(0x00FF00));
            g2d.setFont(new Font("Monospaced", Font.BOLD, 10));
            g2d.drawString(">_", 8, 20);
        } else if (name.contains("Settings")) {
            // Gear icon
            g2d.setColor(OS2Clone.OS2_DARK_GRAY);
            g2d.fillOval(6, 6, 20, 20);
            g2d.setColor(OS2Clone.OS2_LIGHT);
            g2d.fillOval(10, 10, 12, 12);
            g2d.setColor(OS2Clone.OS2_DARK_GRAY);
            g2d.fillOval(13, 13, 6, 6);
        } else {
            // Folder icon
            g2d.setColor(new Color(0xFFCC00));
            g2d.fillRoundRect(2, 10, 28, 18, 4, 4);
            g2d.fillRoundRect(2, 6, 14, 8, 2, 2);
            g2d.setColor(new Color(0xCC9900));
            g2d.drawRoundRect(2, 10, 28, 18, 4, 4);
        }
        
        g2d.dispose();
        return img;
    }
    
    @Override
    protected void paintComponent(Graphics g) {
        Graphics2D g2d = (Graphics2D) g;
        g2d.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, 
                            RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
        
        int iconX = (getWidth() - 32) / 2;
        int iconY = 4;
        
        // Draw selection highlight
        if (selected) {
            g2d.setColor(OS2Clone.OS2_BLUE);
            g2d.fillRect(iconX - 2, iconY - 2, 36, 36);
        }
        
        // Draw icon
        g2d.drawImage(iconImage, iconX, iconY, null);
        
        // Draw label
        g2d.setFont(new Font("SansSerif", Font.PLAIN, 11));
        FontMetrics fm = g2d.getFontMetrics();
        
        String[] lines = name.split("\n");
        int textY = 44;
        
        for (String line : lines) {
            int textWidth = fm.stringWidth(line);
            int textX = (getWidth() - textWidth) / 2;
            
            if (selected) {
                g2d.setColor(OS2Clone.OS2_BLUE);
                g2d.fillRect(textX - 2, textY - fm.getAscent(), textWidth + 4, fm.getHeight());
                g2d.setColor(Color.WHITE);
            } else {
                // Drop shadow for visibility
                g2d.setColor(Color.BLACK);
                g2d.drawString(line, textX + 1, textY + 1);
                g2d.setColor(Color.WHITE);
            }
            g2d.drawString(line, textX, textY);
            textY += fm.getHeight();
        }
    }
}

/**
 * Base OS/2 Window
 */
class OS2Window extends JInternalFrame {
    public OS2Window(String title) {
        super(title, true, true, true, true);
        
        setBackground(OS2Clone.OS2_GRAY);
        getContentPane().setBackground(OS2Clone.OS2_GRAY);
        setBounds(100 + (int)(Math.random() * 100), 50 + (int)(Math.random() * 100), 500, 350);
        
        // Custom title bar colors are handled by UIManager defaults
    }
}

/**
 * OS/2 File Manager
 */
class OS2FileManager extends OS2Window {
    private JTree directoryTree;
    private JList<String> fileList;
    private JLabel statusBar;
    private DefaultListModel<String> fileListModel;
    
    public OS2FileManager() {
        super("File Manager - Drive C:");
        initializeUI();
    }
    
    private void initializeUI() {
        JPanel mainPanel = new JPanel(new BorderLayout(2, 2));
        mainPanel.setBackground(OS2Clone.OS2_GRAY);
        mainPanel.setBorder(BorderFactory.createEmptyBorder(4, 4, 4, 4));
        
        // Toolbar
        JToolBar toolbar = createToolbar();
        mainPanel.add(toolbar, BorderLayout.NORTH);
        
        // Split pane with tree and list
        JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
        splitPane.setBackground(OS2Clone.OS2_GRAY);
        
        // Directory tree
        directoryTree = createDirectoryTree();
        JScrollPane treeScroll = new JScrollPane(directoryTree);
        treeScroll.setPreferredSize(new Dimension(180, 0));
        
        // File list
        fileListModel = new DefaultListModel<>();
        fileList = new JList<>(fileListModel);
        fileList.setBackground(Color.WHITE);
        JScrollPane listScroll = new JScrollPane(fileList);
        
        splitPane.setLeftComponent(treeScroll);
        splitPane.setRightComponent(listScroll);
        splitPane.setDividerLocation(180);
        
        mainPanel.add(splitPane, BorderLayout.CENTER);
        
        // Status bar
        statusBar = new JLabel(" Ready");
        statusBar.setBorder(new OS2Border(false));
        statusBar.setPreferredSize(new Dimension(0, 20));
        mainPanel.add(statusBar, BorderLayout.SOUTH);
        
        setContentPane(mainPanel);
        
        // Populate initial view
        populateFileList("C:\\");
    }
    
    private JToolBar createToolbar() {
        JToolBar toolbar = new JToolBar();
        toolbar.setFloatable(false);
        toolbar.setBackground(OS2Clone.OS2_GRAY);
        toolbar.setBorder(new OS2Border(true));
        
        toolbar.add(OS2Clone.createOS2Button("Open"));
        toolbar.add(OS2Clone.createOS2Button("Copy"));
        toolbar.add(OS2Clone.createOS2Button("Move"));
        toolbar.add(OS2Clone.createOS2Button("Delete"));
        toolbar.addSeparator();
        toolbar.add(OS2Clone.createOS2Button("Find"));
        
        return toolbar;
    }
    
    private JTree createDirectoryTree() {
        DefaultMutableTreeNode root = new DefaultMutableTreeNode("Drive C:");
        
        // Simulate directory structure
        DefaultMutableTreeNode os2 = new DefaultMutableTreeNode("OS2");
        os2.add(new DefaultMutableTreeNode("SYSTEM"));
        os2.add(new DefaultMutableTreeNode("DLL"));
        os2.add(new DefaultMutableTreeNode("HELP"));
        os2.add(new DefaultMutableTreeNode("APPS"));
        root.add(os2);
        
        DefaultMutableTreeNode programs = new DefaultMutableTreeNode("PROGRAMS");
        programs.add(new DefaultMutableTreeNode("ACCESSORIES"));
        programs.add(new DefaultMutableTreeNode("GAMES"));
        root.add(programs);
        
        root.add(new DefaultMutableTreeNode("TEMP"));
        root.add(new DefaultMutableTreeNode("USER"));
        
        JTree tree = new JTree(root);
        tree.setBackground(Color.WHITE);
        tree.addTreeSelectionListener(e -> {
            DefaultMutableTreeNode node = (DefaultMutableTreeNode) 
                tree.getLastSelectedPathComponent();
            if (node != null) {
                populateFileList(node.toString());
            }
        });
        
        return tree;
    }
    
    private void populateFileList(String path) {
        fileListModel.clear();
        
        // Simulated files based on path
        if (path.contains("SYSTEM")) {
            fileListModel.addElement("CONFIG.SYS");
            fileListModel.addElement("AUTOEXEC.BAT");
            fileListModel.addElement("OS2KRNL.EXE");
            fileListModel.addElement("PMSHELL.EXE");
        } else if (path.contains("DLL")) {
            fileListModel.addElement("PMWIN.DLL");
            fileListModel.addElement("PMGPI.DLL");
            fileListModel.addElement("DOSCALL1.DLL");
        } else {
            fileListModel.addElement("README.TXT");
            fileListModel.addElement("INSTALL.LOG");
        }
        
        statusBar.setText(" " + fileListModel.size() + " objects");
    }
}

/**
 * OS/2 System Settings
 */
class OS2SystemSettings extends OS2Window {
    public OS2SystemSettings() {
        super("System Setup");
        initializeUI();
        setSize(450, 350);
    }
    
    private void initializeUI() {
        JTabbedPane tabs = new JTabbedPane();
        tabs.setBackground(OS2Clone.OS2_GRAY);
        
        tabs.addTab("Scheme", createSchemePanel());
        tabs.addTab("Display", createDisplayPanel());
        tabs.addTab("Mouse", createMousePanel());
        tabs.addTab("Sound", createSoundPanel());
        tabs.addTab("System", createSystemPanel());
        
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        buttonPanel.setBackground(OS2Clone.OS2_GRAY);
        buttonPanel.add(OS2Clone.createOS2Button("OK"));
        buttonPanel.add(OS2Clone.createOS2Button("Cancel"));
        buttonPanel.add(OS2Clone.createOS2Button("Help"));
        
        getContentPane().setLayout(new BorderLayout());
        getContentPane().add(tabs, BorderLayout.CENTER);
        getContentPane().add(buttonPanel, BorderLayout.SOUTH);
    }
    
    private JPanel createSchemePanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        panel.setBackground(OS2Clone.OS2_GRAY);
        panel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.anchor = GridBagConstraints.WEST;
        
        gbc.gridx = 0; gbc.gridy = 0;
        panel.add(new JLabel("Color Scheme:"), gbc);
        
        gbc.gridx = 1;
        String[] schemes = {"OS/2 Default", "High Contrast", "Ocean Blue", "Desert", "Classic Gray"};
        JComboBox<String> schemeCombo = new JComboBox<>(schemes);
        schemeCombo.setBackground(Color.WHITE);
        panel.add(schemeCombo, gbc);
        
        gbc.gridx = 0; gbc.gridy = 1;
        panel.add(new JLabel("Window Style:"), gbc);
        
        gbc.gridx = 1;
        String[] styles = {"OS/2 Warp 4", "OS/2 Warp 3", "OS/2 2.1"};
        JComboBox<String> styleCombo = new JComboBox<>(styles);
        styleCombo.setBackground(Color.WHITE);
        panel.add(styleCombo, gbc);
        
        return panel;
    }
    
    private JPanel createDisplayPanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        panel.setBackground(OS2Clone.OS2_GRAY);
        
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.anchor = GridBagConstraints.WEST;
        
        gbc.gridx = 0; gbc.gridy = 0;
        panel.add(new JLabel("Resolution:"), gbc);
        
        gbc.gridx = 1;
        String[] resolutions = {"640x480", "800x600", "1024x768", "1280x1024"};
        panel.add(new JComboBox<>(resolutions), gbc);
        
        gbc.gridx = 0; gbc.gridy = 1;
        panel.add(new JLabel("Colors:"), gbc);
        
        gbc.gridx = 1;
        String[] colors = {"256 colors", "65536 colors", "16.7M colors"};
        panel.add(new JComboBox<>(colors), gbc);
        
        return panel;
    }
    
    private JPanel createMousePanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        panel.setBackground(OS2Clone.OS2_GRAY);
        
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.anchor = GridBagConstraints.WEST;
        
        gbc.gridx = 0; gbc.gridy = 0;
        panel.add(new JLabel("Double-click speed:"), gbc);
        
        gbc.gridx = 1;
        panel.add(new JSlider(0, 100, 50), gbc);
        
        gbc.gridx = 0; gbc.gridy = 1;
        panel.add(new JLabel("Pointer speed:"), gbc);
        
        gbc.gridx = 1;
        panel.add(new JSlider(0, 100, 50), gbc);
        
        gbc.gridx = 0; gbc.gridy = 2; gbc.gridwidth = 2;
        JCheckBox swapButtons = new JCheckBox("Swap left and right buttons");
        swapButtons.setBackground(OS2Clone.OS2_GRAY);
        panel.add(swapButtons, gbc);
        
        return panel;
    }
    
    private JPanel createSoundPanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        panel.setBackground(OS2Clone.OS2_GRAY);
        
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.anchor = GridBagConstraints.WEST;
        
        gbc.gridx = 0; gbc.gridy = 0; gbc.gridwidth = 2;
        JCheckBox enableSound = new JCheckBox("Enable system sounds");
        enableSound.setBackground(OS2Clone.OS2_GRAY);
        enableSound.setSelected(true);
        panel.add(enableSound, gbc);
        
        gbc.gridy = 1; gbc.gridwidth = 1;
        panel.add(new JLabel("Volume:"), gbc);
        
        gbc.gridx = 1;
        panel.add(new JSlider(0, 100, 75), gbc);
        
        return panel;
    }
    
    private JPanel createSystemPanel() {
        JPanel panel = new JPanel(new BorderLayout(10, 10));
        panel.setBackground(OS2Clone.OS2_GRAY);
        panel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        
        JTextArea sysInfo = new JTextArea();
        sysInfo.setEditable(false);
        sysInfo.setBackground(OS2Clone.OS2_GRAY);
        sysInfo.setText(
            "OS/2 Warp Clone Version 1.0\n\n" +
            "Java Version: " + System.getProperty("java.version") + "\n" +
            "OS: " + System.getProperty("os.name") + "\n" +
            "Architecture: " + System.getProperty("os.arch") + "\n\n" +
            "Available Memory: " + (Runtime.getRuntime().freeMemory() / 1024 / 1024) + " MB\n" +
            "Total Memory: " + (Runtime.getRuntime().totalMemory() / 1024 / 1024) + " MB"
        );
        
        panel.add(new JScrollPane(sysInfo), BorderLayout.CENTER);
        
        return panel;
    }
}

/**
 * OS/2 Command Prompt
 */
class OS2CommandPrompt extends OS2Window {
    private JTextArea terminal;
    private JTextField inputField;
    private StringBuilder history = new StringBuilder();
    private String currentDir = "C:\\";
    
    public OS2CommandPrompt() {
        super("OS/2 Command Prompt");
        initializeUI();
    }
    
    private void initializeUI() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBackground(Color.BLACK);
        
        terminal = new JTextArea();
        terminal.setBackground(Color.BLACK);
        terminal.setForeground(new Color(0x00FF00));
        terminal.setCaretColor(new Color(0x00FF00));
        terminal.setFont(new Font("Monospaced", Font.PLAIN, 14));
        terminal.setEditable(false);
        terminal.setLineWrap(true);
        
        history.append("[C:\\]$ ");
        history.append("OS/2 Warp Command Processor\n");
        history.append("(C) Copyright IBM Corporation 1987-1996\n");
        history.append("Clone Version 1.0\n\n");
        history.append(currentDir + ">");
        terminal.setText(history.toString());
        
        JScrollPane scroll = new JScrollPane(terminal);
        scroll.setBorder(null);
        
        inputField = new JTextField();
        inputField.setBackground(Color.BLACK);
        inputField.setForeground(new Color(0x00FF00));
        inputField.setCaretColor(new Color(0x00FF00));
        inputField.setFont(new Font("Monospaced", Font.PLAIN, 14));
        inputField.setBorder(null);
        
        inputField.addActionListener(e -> executeCommand(inputField.getText()));
        
        panel.add(scroll, BorderLayout.CENTER);
        panel.add(inputField, BorderLayout.SOUTH);
        
        setContentPane(panel);
        
        // Focus input on open
        addInternalFrameListener(new InternalFrameAdapter() {
            @Override
            public void internalFrameActivated(InternalFrameEvent e) {
                inputField.requestFocus();
            }
        });
    }
    
    private void executeCommand(String cmd) {
        history.append(cmd).append("\n");
        inputField.setText("");
        
        String response = processCommand(cmd.trim().toLowerCase());
        if (!response.isEmpty()) {
            history.append(response).append("\n");
        }
        
        history.append(currentDir).append(">");
        terminal.setText(history.toString());
        terminal.setCaretPosition(terminal.getText().length());
    }
    
    private String processCommand(String cmd) {
        if (cmd.isEmpty()) return "";
        
        String[] parts = cmd.split("\\s+");
        String command = parts[0];
        
        switch (command) {
            case "dir":
                return "Volume in drive C has no label\n" +
                       "Directory of " + currentDir + "\n\n" +
                       ".              <DIR>     \n" +
                       "..             <DIR>     \n" +
                       "OS2            <DIR>     \n" +
                       "PROGRAMS       <DIR>     \n" +
                       "CONFIG   SYS      1,234  \n" +
                       "        5 file(s)    1,234 bytes";
            case "cls":
                history = new StringBuilder();
                return "";
            case "ver":
                return "OS/2 Warp Clone Version 1.0";
            case "help":
                return "Available commands:\n" +
                       "DIR    - List directory contents\n" +
                       "CLS    - Clear screen\n" +
                       "VER    - Show version\n" +
                       "CD     - Change directory\n" +
                       "DATE   - Show date\n" +
                       "TIME   - Show time\n" +
                       "EXIT   - Close command prompt";
            case "date":
                return "Current date: " + new java.util.Date().toString();
            case "time":
                return "Current time: " + new java.text.SimpleDateFormat("HH:mm:ss").format(new java.util.Date());
            case "cd":
                if (parts.length > 1) {
                    if (parts[1].equals("..")) {
                        currentDir = "C:\\";
                    } else {
                        currentDir = "C:\\" + parts[1].toUpperCase();
                    }
                }
                return "";
            case "exit":
                dispose();
                return "";
            default:
                return "'" + command + "' is not recognized as an internal or external command.";
        }
    }
}
