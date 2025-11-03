// WorkspaceManager.java - Enhanced Version
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class WorkspaceManager extends JDialog {
    private int numWorkspaces;
    private JButton[] workspaceButtons;
    private IRIXDesktop desktop;
    private int currentWorkspace = 0;
    
    public WorkspaceManager(int numWorkspaces, IRIXDesktop desktop) {
        super((Frame)desktop, "Workspace Manager", false);
        this.numWorkspaces = numWorkspaces;
        this.desktop = desktop;
        initializeUI();
    }
    
    private void initializeUI() {
        setSize(500, 380);
        setDefaultCloseOperation(HIDE_ON_CLOSE);
        setLocationRelativeTo(desktop);
        
        JPanel mainPanel = new JPanel(new BorderLayout(10, 10));
        mainPanel.setBorder(BorderFactory.createEmptyBorder(15, 15, 15, 15));
        mainPanel.setBackground(new Color(240, 240, 240));
        
        // Title panel
        JPanel titlePanel = new JPanel(new BorderLayout());
        titlePanel.setBackground(new Color(240, 240, 240));
        
        JLabel titleLabel = new JLabel("Virtual Workspaces");
        titleLabel.setFont(new Font("SansSerif", Font.BOLD, 16));
        
        JLabel subtitle = new JLabel("Click a workspace to switch, or use Ctrl+Alt+1-4");
        subtitle.setFont(new Font("SansSerif", Font.PLAIN, 11));
        subtitle.setForeground(Color.GRAY);
        
        titlePanel.add(titleLabel, BorderLayout.NORTH);
        titlePanel.add(subtitle, BorderLayout.SOUTH);
        
        // Workspace grid
        JPanel gridPanel = new JPanel(new GridLayout(2, 2, 15, 15));
        gridPanel.setBackground(new Color(240, 240, 240));
        gridPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        
        workspaceButtons = new JButton[numWorkspaces];
        
        Color[] colors = {
            new Color(173, 216, 230),  // Light Blue
            new Color(255, 182, 193),  // Light Pink
            new Color(144, 238, 144),  // Light Green
            new Color(255, 255, 224)   // Light Yellow
        };
        
        for (int i = 0; i < numWorkspaces; i++) {
            final int workspaceNum = i;
            
            JPanel wsPanel = new JPanel(new BorderLayout());
            wsPanel.setBackground(colors[i]);
            wsPanel.setBorder(BorderFactory.createCompoundBorder(
                BorderFactory.createLineBorder(Color.GRAY, 2),
                BorderFactory.createEmptyBorder(10, 10, 10, 10)
            ));
            
            workspaceButtons[i] = new JButton() {
                @Override
                protected void paintComponent(Graphics g) {
                    Graphics2D g2d = (Graphics2D) g;
                    g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                    
                    // Background
                    if (workspaceNum == currentWorkspace) {
                        g2d.setColor(colors[workspaceNum].darker());
                    } else if (getModel().isRollover()) {
                        g2d.setColor(colors[workspaceNum].brighter());
                    } else {
                        g2d.setColor(colors[workspaceNum]);
                    }
                    g2d.fillRect(0, 0, getWidth(), getHeight());
                    
                    // Border
                    g2d.setColor(Color.GRAY);
                    g2d.setStroke(new BasicStroke(2));
                    g2d.drawRect(1, 1, getWidth()-2, getHeight()-2);
                    
                    // Active indicator
                    if (workspaceNum == currentWorkspace) {
                        g2d.setColor(new Color(0, 128, 0));
                        g2d.fillOval(10, 10, 12, 12);
                        g2d.setColor(Color.WHITE);
                        g2d.setFont(new Font("SansSerif", Font.BOLD, 8));
                        g2d.drawString("✓", 13, 19);
                    }
                    
                    // Workspace number
                    g2d.setColor(Color.BLACK);
                    g2d.setFont(new Font("SansSerif", Font.BOLD, 48));
                    FontMetrics fm = g2d.getFontMetrics();
                    String text = String.valueOf(workspaceNum + 1);
                    int textWidth = fm.stringWidth(text);
                    int textHeight = fm.getHeight();
                    g2d.drawString(text, (getWidth() - textWidth) / 2, 
                                  (getHeight() + textHeight / 2) / 2);
                    
                    // Label
                    g2d.setFont(new Font("SansSerif", Font.BOLD, 14));
                    fm = g2d.getFontMetrics();
                    String label = "Workspace " + (workspaceNum + 1);
                    textWidth = fm.stringWidth(label);
                    g2d.drawString(label, (getWidth() - textWidth) / 2, getHeight() - 15);
                }
            };
            
            workspaceButtons[i].setPreferredSize(new Dimension(200, 140));
            workspaceButtons[i].setBorder(BorderFactory.createEmptyBorder());
            workspaceButtons[i].setContentAreaFilled(false);
            workspaceButtons[i].setFocusPainted(false);
            workspaceButtons[i].setCursor(new Cursor(Cursor.HAND_CURSOR));
            
            workspaceButtons[i].addActionListener(e -> {
                currentWorkspace = workspaceNum;
                desktop.switchWorkspace(workspaceNum);
                setVisible(false);
                repaintButtons();
            });
            
            // Tooltip
            workspaceButtons[i].setToolTipText("Switch to Workspace " + (i + 1) + 
                                              " (Ctrl+Alt+" + (i + 1) + ")");
            
            gridPanel.add(workspaceButtons[i]);
        }
        
        // Info panel
        JPanel infoPanel = new JPanel(new FlowLayout(FlowLayout.CENTER));
        infoPanel.setBackground(new Color(240, 240, 240));
        
        JLabel infoLabel = new JLabel("💡 Each workspace maintains its own set of windows");
        infoLabel.setFont(new Font("SansSerif", Font.ITALIC, 11));
        infoLabel.setForeground(new Color(100, 100, 100));
        
        infoPanel.add(infoLabel);
        
        // Button panel
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        buttonPanel.setBackground(new Color(240, 240, 240));
        
        JButton closeBtn = new JButton("Close");
        closeBtn.setFont(new Font("SansSerif", Font.PLAIN, 11));
        closeBtn.addActionListener(e -> setVisible(false));
        
        buttonPanel.add(closeBtn);
        
        mainPanel.add(titlePanel, BorderLayout.NORTH);
        mainPanel.add(gridPanel, BorderLayout.CENTER);
        mainPanel.add(infoPanel, BorderLayout.SOUTH);
        
        JPanel bottomPanel = new JPanel(new BorderLayout());
        bottomPanel.setBackground(new Color(240, 240, 240));
        bottomPanel.add(buttonPanel, BorderLayout.SOUTH);
        
        mainPanel.add(bottomPanel, BorderLayout.SOUTH);
        
        add(mainPanel);
        
        setupKeyboardShortcuts();
    }
    
    private void repaintButtons() {
        for (JButton btn : workspaceButtons) {
            btn.repaint();
        }
    }
    
    private void setupKeyboardShortcuts() {
        for (int i = 0; i < numWorkspaces && i < 4; i++) {
            final int workspaceNum = i;
            KeyStroke keyStroke = KeyStroke.getKeyStroke(
                KeyEvent.VK_1 + i, 
                InputEvent.CTRL_DOWN_MASK | InputEvent.ALT_DOWN_MASK
            );
            
            getRootPane().getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW)
                .put(keyStroke, "switchToWorkspace" + i);
                
            getRootPane().getActionMap().put("switchToWorkspace" + i, new AbstractAction() {
                public void actionPerformed(ActionEvent e) {
                    currentWorkspace = workspaceNum;
                    desktop.switchWorkspace(workspaceNum);
                    setVisible(false);
                    repaintButtons();
                }
            });
        }
        
        // ESC to close
        KeyStroke escKey = KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE, 0);
        getRootPane().getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW).put(escKey, "closeDialog");
        getRootPane().getActionMap().put("closeDialog", new AbstractAction() {
            public void actionPerformed(ActionEvent e) {
                setVisible(false);
            }
        });
    }
    
    public void setCurrentWorkspace(int workspace) {
        this.currentWorkspace = workspace;
        repaintButtons();
    }
}
