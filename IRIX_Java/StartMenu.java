// StartMenu.java - Enhanced Version
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class StartMenu extends JWindow {
    private final Color MENU_BG = new Color(189, 189, 189);
    private final Color HEADER_BG = new Color(0, 102, 204);
    private final Color HIGHLIGHT = new Color(0, 0, 128);
    private final Color HIGHLIGHT_TEXT = Color.WHITE;
    private final Color DARK_GRAY = new Color(99, 99, 99);
    private final Color SHADOW = new Color(66, 66, 66);
    private IRIXDesktop desktop;
    
    public StartMenu(IRIXDesktop desktop) {
        this.desktop = desktop;
        setSize(240, 400);
        setBackground(new Color(0, 0, 0, 0));
        
        JPanel content = new JPanel(new BorderLayout());
        content.setBackground(MENU_BG);
        content.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(Color.BLACK, 2),
            BorderFactory.createCompoundBorder(
                BorderFactory.createLineBorder(new Color(238, 238, 238), 1),
                BorderFactory.createEmptyBorder(0, 0, 0, 0)
            )
        ));
        
        // Header with SGI branding
        JPanel header = new JPanel(new BorderLayout());
        header.setBackground(HEADER_BG);
        header.setPreferredSize(new Dimension(240, 50));
        
        JLabel logoLabel = new JLabel("  IRIX");
        logoLabel.setForeground(Color.WHITE);
        logoLabel.setFont(new Font("SansSerif", Font.BOLD, 24));
        
        JLabel subtitle = new JLabel("  Applications");
        subtitle.setForeground(new Color(200, 220, 255));
        subtitle.setFont(new Font("SansSerif", Font.PLAIN, 11));
        
        JPanel headerText = new JPanel(new BorderLayout());
        headerText.setBackground(HEADER_BG);
        headerText.add(logoLabel, BorderLayout.NORTH);
        headerText.add(subtitle, BorderLayout.SOUTH);
        
        header.add(headerText, BorderLayout.CENTER);
        
        // Menu items
        String[][] menuItems = {
            {"System Manager", "system", "⚙️"},
            {"Terminal", "terminal", "🖥️"},
            {"File Manager", "files", "📁"},
            {"Text Editor", "editor", "📝"},
            {"Web Browser", "browser", "🌐"},
            {"Calculator", "calculator", "🔢"},
            {"Drawing Tool", "draw", "🎨"},
            {"Media Player", "media", "🎵"},
            {"", "separator", ""},
            {"System Settings", "settings", "⚡"},
            {"Help", "help", "❓"},
            {"", "separator", ""},
            {"Shutdown", "shutdown", "⏻"}
        };
        
        JPanel menuPanel = new JPanel();
        menuPanel.setLayout(new BoxLayout(menuPanel, BoxLayout.Y_AXIS));
        menuPanel.setBackground(MENU_BG);
        menuPanel.setBorder(BorderFactory.createEmptyBorder(5, 2, 5, 2));
        
        for (String[] item : menuItems) {
            if (item[1].equals("separator")) {
                JSeparator sep = new JSeparator();
                sep.setForeground(DARK_GRAY);
                sep.setMaximumSize(new Dimension(Integer.MAX_VALUE, 1));
                menuPanel.add(Box.createVerticalStrut(3));
                menuPanel.add(sep);
                menuPanel.add(Box.createVerticalStrut(3));
            } else {
                JPanel menuItem = createMenuItem(item[0], item[1], item[2]);
                menuPanel.add(menuItem);
            }
        }
        
        JScrollPane scrollPane = new JScrollPane(menuPanel);
        scrollPane.setBorder(BorderFactory.createEmptyBorder());
        scrollPane.getVerticalScrollBar().setUnitIncrement(16);
        
        content.add(header, BorderLayout.NORTH);
        content.add(scrollPane, BorderLayout.CENTER);
        
        setContentPane(content);
        
        // Click outside to close
        addMouseListener(new MouseAdapter() {
            public void mouseExited(MouseEvent e) {
                // Could auto-hide here if desired
            }
        });
    }
    
    private JPanel createMenuItem(String text, String action, String emoji) {
        JPanel menuItem = new JPanel(new BorderLayout());
        menuItem.setOpaque(true);
        menuItem.setBackground(MENU_BG);
        menuItem.setBorder(BorderFactory.createEmptyBorder(4, 8, 4, 8));
        menuItem.setMaximumSize(new Dimension(Integer.MAX_VALUE, 32));
        
        JLabel icon = new JLabel(emoji + "  ");
        icon.setFont(new Font("SansSerif", Font.PLAIN, 16));
        
        JLabel label = new JLabel(text);
        label.setFont(new Font("SansSerif", Font.PLAIN, 12));
        label.setForeground(Color.BLACK);
        
        menuItem.add(icon, BorderLayout.WEST);
        menuItem.add(label, BorderLayout.CENTER);
        
        menuItem.addMouseListener(new MouseAdapter() {
            public void mouseEntered(MouseEvent e) {
                menuItem.setBackground(HIGHLIGHT);
                label.setForeground(HIGHLIGHT_TEXT);
                icon.setForeground(HIGHLIGHT_TEXT);
            }
            
            public void mouseExited(MouseEvent e) {
                menuItem.setBackground(MENU_BG);
                label.setForeground(Color.BLACK);
                icon.setForeground(Color.BLACK);
            }
            
            public void mouseClicked(MouseEvent e) {
                desktop.playClickSound();
                setVisible(false);
                executeMenuAction(action);
            }
        });
        
        return menuItem;
    }
    
    private void executeMenuAction(String action) {
        switch (action) {
            case "system":
                desktop.openSystemManager();
                break;
            case "terminal":
                desktop.openTerminal();
                break;
            case "files":
                desktop.openFileManager();
                break;
            case "editor":
                desktop.openTextEditor();
                break;
            case "browser":
                desktop.openWebBrowser();
                break;
            case "calculator":
                desktop.openCalculator();
                break;
            case "draw":
                desktop.openDrawingApp();
                break;
            case "media":
                desktop.openMediaPlayer();
                break;
            case "settings":
                JOptionPane.showMessageDialog(desktop, 
                    "System Settings\n\nConfigure display, network, and system preferences.",
                    "System Settings",
                    JOptionPane.INFORMATION_MESSAGE);
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
            "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n" +
            "Welcome to the IRIX Desktop Environment!\n\n" +
            "KEYBOARD SHORTCUTS:\n" +
            "  Ctrl+Alt+T       Open Terminal\n" +
            "  Ctrl+Alt+1-4     Switch Workspaces\n\n" +
            "APPLICATIONS:\n" +
            "  System Manager   - View system information and processes\n" +
            "  Terminal         - Command line interface\n" +
            "  File Manager     - Browse and manage files\n" +
            "  Text Editor      - Edit text files\n" +
            "  Web Browser      - Basic HTML browsing\n" +
            "  Calculator       - Perform calculations\n" +
            "  Drawing Tool     - Create simple drawings\n" +
            "  Media Player     - Media playback simulation\n\n" +
            "FEATURES:\n" +
            "  • Multiple virtual workspaces\n" +
            "  • Window management\n" +
            "  • Desktop context menu (right-click)\n" +
            "  • Taskbar with window buttons\n\n" +
            "CREDITS:\n" +
            "  Inspired by Silicon Graphics IRIX 6.5\n" +
            "  Version 1.0 - Enhanced Edition\n\n" +
            "For more information, visit the system documentation."
        );
        
        JScrollPane scrollPane = new JScrollPane(helpText);
        helpDialog.add(scrollPane);
        helpDialog.setVisible(true);
    }
}
