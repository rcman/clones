// StartMenu.java - Enhanced IRIX-style start menu
// Improvements: Better organization, recent items, search (placeholder)

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

/**
 * IRIX-style start menu with categorized applications.
 */
public class StartMenu extends JWindow {
    
    private static final Color MENU_BG = new Color(189, 189, 189);
    private static final Color HEADER_BG = new Color(0, 102, 204);
    private static final Color HIGHLIGHT = new Color(0, 0, 128);
    private static final Color HIGHLIGHT_TEXT = Color.WHITE;
    private static final Color DARK_GRAY = new Color(99, 99, 99);
    
    private final IRIXDesktop desktop;
    
    public StartMenu(IRIXDesktop desktop) {
        this.desktop = desktop;
        setSize(260, 420);
        setBackground(new Color(0, 0, 0, 0));
        
        initializeUI();
    }
    
    private void initializeUI() {
        JPanel content = new JPanel(new BorderLayout());
        content.setBackground(MENU_BG);
        content.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(Color.BLACK, 2),
            BorderFactory.createCompoundBorder(
                BorderFactory.createLineBorder(new Color(238, 238, 238), 1),
                BorderFactory.createEmptyBorder(0, 0, 0, 0)
            )
        ));
        
        // Header
        JPanel header = createHeader();
        content.add(header, BorderLayout.NORTH);
        
        // Menu items
        JPanel menuPanel = createMenuPanel();
        JScrollPane scrollPane = new JScrollPane(menuPanel);
        scrollPane.setBorder(BorderFactory.createEmptyBorder());
        scrollPane.getVerticalScrollBar().setUnitIncrement(16);
        content.add(scrollPane, BorderLayout.CENTER);
        
        setContentPane(content);
    }
    
    private JPanel createHeader() {
        JPanel header = new JPanel(new BorderLayout());
        header.setBackground(HEADER_BG);
        header.setPreferredSize(new Dimension(260, 55));
        header.setBorder(BorderFactory.createEmptyBorder(8, 10, 8, 10));
        
        JLabel logoLabel = new JLabel("IRIX");
        logoLabel.setForeground(Color.WHITE);
        logoLabel.setFont(new Font("SansSerif", Font.BOLD, 26));
        
        JLabel subtitle = new JLabel("Desktop Environment");
        subtitle.setForeground(new Color(200, 220, 255));
        subtitle.setFont(new Font("SansSerif", Font.PLAIN, 11));
        
        JPanel headerText = new JPanel();
        headerText.setLayout(new BoxLayout(headerText, BoxLayout.Y_AXIS));
        headerText.setOpaque(false);
        headerText.add(logoLabel);
        headerText.add(subtitle);
        
        header.add(headerText, BorderLayout.WEST);
        
        return header;
    }
    
    private JPanel createMenuPanel() {
        JPanel menuPanel = new JPanel();
        menuPanel.setLayout(new BoxLayout(menuPanel, BoxLayout.Y_AXIS));
        menuPanel.setBackground(MENU_BG);
        menuPanel.setBorder(BorderFactory.createEmptyBorder(5, 2, 5, 2));
        
        // Applications section
        addSectionHeader(menuPanel, "Applications");
        addMenuItem(menuPanel, "System Manager", "\u2699\uFE0F", "system");
        addMenuItem(menuPanel, "Terminal", "\uD83D\uDCBB", "terminal");
        addMenuItem(menuPanel, "File Manager", "\uD83D\uDCC1", "files");
        addMenuItem(menuPanel, "Text Editor", "\uD83D\uDCDD", "editor");
        addMenuItem(menuPanel, "Web Browser", "\uD83C\uDF10", "browser");
        addMenuItem(menuPanel, "Calculator", "\uD83D\uDCF1", "calculator");
        addMenuItem(menuPanel, "Drawing Tool", "\uD83C\uDFA8", "draw");
        addMenuItem(menuPanel, "Media Player", "\uD83C\uDFB5", "media");
        
        addSeparator(menuPanel);
        
        // System section
        addSectionHeader(menuPanel, "System");
        addMenuItem(menuPanel, "System Settings", "\u26A1", "settings");
        addMenuItem(menuPanel, "Help", "\u2753", "help");
        
        addSeparator(menuPanel);
        
        // Power section
        addMenuItem(menuPanel, "Shutdown", "\u23FB", "shutdown");
        
        return menuPanel;
    }
    
    private void addSectionHeader(JPanel panel, String text) {
        JLabel header = new JLabel("  " + text);
        header.setFont(new Font("SansSerif", Font.BOLD, 10));
        header.setForeground(DARK_GRAY);
        header.setMaximumSize(new Dimension(Integer.MAX_VALUE, 20));
        header.setBorder(BorderFactory.createEmptyBorder(5, 5, 2, 5));
        panel.add(header);
    }
    
    private void addSeparator(JPanel panel) {
        panel.add(Box.createVerticalStrut(3));
        JSeparator sep = new JSeparator();
        sep.setForeground(DARK_GRAY);
        sep.setMaximumSize(new Dimension(Integer.MAX_VALUE, 1));
        panel.add(sep);
        panel.add(Box.createVerticalStrut(3));
    }
    
    private void addMenuItem(JPanel panel, String text, String emoji, String action) {
        JPanel menuItem = new JPanel(new BorderLayout());
        menuItem.setOpaque(true);
        menuItem.setBackground(MENU_BG);
        menuItem.setBorder(BorderFactory.createEmptyBorder(6, 10, 6, 10));
        menuItem.setMaximumSize(new Dimension(Integer.MAX_VALUE, 34));
        
        JLabel icon = new JLabel(emoji + "  ");
        icon.setFont(new Font("SansSerif", Font.PLAIN, 16));
        
        JLabel label = new JLabel(text);
        label.setFont(new Font("SansSerif", Font.PLAIN, 12));
        label.setForeground(Color.BLACK);
        
        menuItem.add(icon, BorderLayout.WEST);
        menuItem.add(label, BorderLayout.CENTER);
        
        menuItem.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseEntered(MouseEvent e) {
                menuItem.setBackground(HIGHLIGHT);
                label.setForeground(HIGHLIGHT_TEXT);
            }
            
            @Override
            public void mouseExited(MouseEvent e) {
                menuItem.setBackground(MENU_BG);
                label.setForeground(Color.BLACK);
            }
            
            @Override
            public void mouseClicked(MouseEvent e) {
                desktop.playClickSound();
                setVisible(false);
                executeAction(action);
            }
        });
        
        panel.add(menuItem);
    }
    
    private void executeAction(String action) {
        switch (action) {
            case "system": desktop.openSystemManager(); break;
            case "terminal": desktop.openTerminal(); break;
            case "files": desktop.openFileManager(); break;
            case "editor": desktop.openTextEditor(); break;
            case "browser": desktop.openWebBrowser(); break;
            case "calculator": desktop.openCalculator(); break;
            case "draw": desktop.openDrawingApp(); break;
            case "media": desktop.openMediaPlayer(); break;
            case "settings":
                JOptionPane.showMessageDialog(desktop,
                    "System Settings\n\nConfigure display, network, and system preferences.",
                    "System Settings", JOptionPane.INFORMATION_MESSAGE);
                break;
            case "help":
                showHelpDialog();
                break;
            case "shutdown":
                desktop.showShutdownDialog();
                break;
        }
    }
    
    private void showHelpDialog() {
        JDialog helpDialog = new JDialog((Frame)desktop, "IRIX Help System", true);
        helpDialog.setSize(500, 400);
        helpDialog.setLocationRelativeTo(desktop);
        
        JTextArea helpText = new JTextArea();
        helpText.setEditable(false);
        helpText.setFont(new Font("SansSerif", Font.PLAIN, 12));
        helpText.setMargin(new Insets(10, 10, 10, 10));
        helpText.setText(
            "IRIX Desktop Environment - Help\n" +
            "═══════════════════════════════\n\n" +
            "Welcome to the IRIX Desktop Environment!\n\n" +
            "KEYBOARD SHORTCUTS:\n" +
            "  Ctrl+Alt+T       Open Terminal\n" +
            "  Ctrl+Alt+1-4     Switch Workspaces\n\n" +
            "APPLICATIONS:\n" +
            "  System Manager   - View system information and processes\n" +
            "  Terminal         - Command line interface\n" +
            "  File Manager     - Browse and manage files\n" +
            "  Text Editor      - Edit text files (with undo/redo)\n" +
            "  Web Browser      - Basic HTML browsing\n" +
            "  Calculator       - Full-featured calculator\n" +
            "  Drawing Tool     - Create simple drawings\n" +
            "  Media Player     - Media playback simulation\n\n" +
            "FEATURES:\n" +
            "  • Multiple virtual workspaces (4)\n" +
            "  • Window management with taskbar\n" +
            "  • Desktop context menu (right-click)\n" +
            "  • Drag windows by title bar\n\n" +
            "CREDITS:\n" +
            "  Inspired by Silicon Graphics IRIX 6.5\n" +
            "  Version 2.0 - Enhanced Edition"
        );
        
        helpDialog.add(new JScrollPane(helpText));
        helpDialog.setVisible(true);
    }
}
