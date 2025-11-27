// WorkspaceManager.java - Enhanced workspace manager
// Improvements: Cleaner code, better visual feedback, drag-and-drop (placeholder)

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

/**
 * Manages virtual workspaces (desktops) allowing users to organize windows.
 */
public class WorkspaceManager extends JDialog {
    
    private final int numWorkspaces;
    private final JButton[] workspaceButtons;
    private final IRIXDesktop desktop;
    private int currentWorkspace = 0;
    
    // Workspace colors
    private static final Color[] WORKSPACE_COLORS = {
        new Color(173, 216, 230),  // Light Blue
        new Color(255, 182, 193),  // Light Pink
        new Color(144, 238, 144),  // Light Green
        new Color(255, 255, 224)   // Light Yellow
    };
    
    public WorkspaceManager(int numWorkspaces, IRIXDesktop desktop) {
        super((Frame)desktop, "Workspace Manager", false);
        this.numWorkspaces = numWorkspaces;
        this.desktop = desktop;
        this.workspaceButtons = new JButton[numWorkspaces];
        
        initializeUI();
        setupKeyboardShortcuts();
    }
    
    private void initializeUI() {
        setSize(500, 400);
        setDefaultCloseOperation(HIDE_ON_CLOSE);
        setLocationRelativeTo(desktop);
        
        JPanel mainPanel = new JPanel(new BorderLayout(10, 10));
        mainPanel.setBorder(BorderFactory.createEmptyBorder(15, 15, 15, 15));
        mainPanel.setBackground(new Color(240, 240, 240));
        
        // Title
        JPanel titlePanel = new JPanel(new BorderLayout());
        titlePanel.setOpaque(false);
        
        JLabel titleLabel = new JLabel("Virtual Workspaces");
        titleLabel.setFont(new Font("SansSerif", Font.BOLD, 18));
        
        JLabel subtitle = new JLabel("Click a workspace to switch, or use Ctrl+Alt+1-4");
        subtitle.setFont(new Font("SansSerif", Font.PLAIN, 11));
        subtitle.setForeground(Color.GRAY);
        
        titlePanel.add(titleLabel, BorderLayout.NORTH);
        titlePanel.add(subtitle, BorderLayout.SOUTH);
        
        // Workspace grid
        JPanel gridPanel = new JPanel(new GridLayout(2, 2, 15, 15));
        gridPanel.setOpaque(false);
        gridPanel.setBorder(BorderFactory.createEmptyBorder(15, 10, 15, 10));
        
        for (int i = 0; i < numWorkspaces; i++) {
            gridPanel.add(createWorkspaceButton(i));
        }
        
        // Info panel
        JPanel infoPanel = new JPanel(new FlowLayout(FlowLayout.CENTER));
        infoPanel.setOpaque(false);
        
        JLabel infoLabel = new JLabel("\uD83D\uDCA1 Each workspace maintains its own set of windows");
        infoLabel.setFont(new Font("SansSerif", Font.ITALIC, 11));
        infoLabel.setForeground(new Color(100, 100, 100));
        infoPanel.add(infoLabel);
        
        // Button panel
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        buttonPanel.setOpaque(false);
        
        JButton closeBtn = new JButton("Close");
        closeBtn.addActionListener(e -> setVisible(false));
        buttonPanel.add(closeBtn);
        
        // Combine info and button panels
        JPanel bottomPanel = new JPanel(new BorderLayout());
        bottomPanel.setOpaque(false);
        bottomPanel.add(infoPanel, BorderLayout.CENTER);
        bottomPanel.add(buttonPanel, BorderLayout.SOUTH);
        
        mainPanel.add(titlePanel, BorderLayout.NORTH);
        mainPanel.add(gridPanel, BorderLayout.CENTER);
        mainPanel.add(bottomPanel, BorderLayout.SOUTH);
        
        add(mainPanel);
    }
    
    private JButton createWorkspaceButton(int index) {
        workspaceButtons[index] = new JButton() {
            @Override
            protected void paintComponent(Graphics g) {
                Graphics2D g2d = (Graphics2D) g;
                g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, 
                                     RenderingHints.VALUE_ANTIALIAS_ON);
                
                // Background
                Color bgColor = WORKSPACE_COLORS[index];
                if (index == currentWorkspace) {
                    bgColor = bgColor.darker();
                } else if (getModel().isRollover()) {
                    bgColor = bgColor.brighter();
                }
                g2d.setColor(bgColor);
                g2d.fillRoundRect(0, 0, getWidth(), getHeight(), 10, 10);
                
                // Border
                g2d.setColor(index == currentWorkspace ? 
                    new Color(0, 100, 0) : Color.GRAY);
                g2d.setStroke(new BasicStroke(index == currentWorkspace ? 3 : 2));
                g2d.drawRoundRect(2, 2, getWidth()-4, getHeight()-4, 8, 8);
                
                // Active indicator
                if (index == currentWorkspace) {
                    g2d.setColor(new Color(0, 150, 0));
                    g2d.fillOval(10, 10, 16, 16);
                    g2d.setColor(Color.WHITE);
                    g2d.setFont(new Font("SansSerif", Font.BOLD, 12));
                    g2d.drawString("\u2713", 13, 22);
                }
                
                // Workspace number
                g2d.setColor(new Color(0, 0, 0, 150));
                g2d.setFont(new Font("SansSerif", Font.BOLD, 56));
                FontMetrics fm = g2d.getFontMetrics();
                String text = String.valueOf(index + 1);
                int textX = (getWidth() - fm.stringWidth(text)) / 2;
                int textY = (getHeight() + fm.getAscent()) / 2 - 10;
                g2d.drawString(text, textX, textY);
                
                // Label
                g2d.setFont(new Font("SansSerif", Font.BOLD, 13));
                fm = g2d.getFontMetrics();
                String label = "Workspace " + (index + 1);
                textX = (getWidth() - fm.stringWidth(label)) / 2;
                g2d.drawString(label, textX, getHeight() - 15);
            }
        };
        
        workspaceButtons[index].setPreferredSize(new Dimension(200, 150));
        workspaceButtons[index].setBorder(BorderFactory.createEmptyBorder());
        workspaceButtons[index].setContentAreaFilled(false);
        workspaceButtons[index].setFocusPainted(false);
        workspaceButtons[index].setCursor(new Cursor(Cursor.HAND_CURSOR));
        workspaceButtons[index].setToolTipText("Switch to Workspace " + (index + 1) + 
                                               " (Ctrl+Alt+" + (index + 1) + ")");
        
        workspaceButtons[index].addActionListener(e -> {
            switchToWorkspace(index);
            setVisible(false);
        });
        
        return workspaceButtons[index];
    }
    
    private void switchToWorkspace(int workspace) {
        currentWorkspace = workspace;
        desktop.switchWorkspace(workspace);
        repaintButtons();
    }
    
    private void repaintButtons() {
        for (JButton btn : workspaceButtons) {
            btn.repaint();
        }
    }
    
    private void setupKeyboardShortcuts() {
        // Workspace shortcuts
        for (int i = 0; i < numWorkspaces && i < 4; i++) {
            final int workspace = i;
            KeyStroke keyStroke = KeyStroke.getKeyStroke(
                KeyEvent.VK_1 + i,
                InputEvent.CTRL_DOWN_MASK | InputEvent.ALT_DOWN_MASK
            );
            
            getRootPane().getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW)
                .put(keyStroke, "switchToWorkspace" + i);
            
            getRootPane().getActionMap().put("switchToWorkspace" + i, new AbstractAction() {
                @Override
                public void actionPerformed(ActionEvent e) {
                    switchToWorkspace(workspace);
                    setVisible(false);
                }
            });
        }
        
        // ESC to close
        KeyStroke escKey = KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE, 0);
        getRootPane().getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW)
            .put(escKey, "closeDialog");
        getRootPane().getActionMap().put("closeDialog", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                setVisible(false);
            }
        });
    }
    
    /**
     * Update the current workspace indicator
     */
    public void setCurrentWorkspace(int workspace) {
        this.currentWorkspace = workspace;
        repaintButtons();
    }
    
    /**
     * Get the current workspace index
     */
    public int getCurrentWorkspace() {
        return currentWorkspace;
    }
}
