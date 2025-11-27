// IRIXWindow.java - Enhanced IRIX-style window with custom title bar
// Improvements: Better drag handling, cleaner separation, proper state management

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

/**
 * Custom JInternalFrame styled to match the IRIX 6.5 window appearance.
 * Features custom title bar with minimize/maximize/close buttons and
 * workspace support for virtual desktop functionality.
 */
public class IRIXWindow extends JInternalFrame {
    
    // Color constants matching IRIX theme
    private static final Color TITLE_BAR_COLOR = new Color(0, 102, 204);
    private static final Color LIGHT_GRAY = new Color(192, 192, 192);
    private static final Color DARK_GRAY = new Color(128, 128, 128);
    private static final Color HIGHLIGHT = new Color(224, 224, 224);
    private static final Color SHADOW = new Color(64, 64, 64);
    
    private static final int TITLE_BAR_HEIGHT = 24;
    private static final Dimension BUTTON_SIZE = new Dimension(20, 16);
    
    private final IRIXDesktop desktop;
    private int workspace = 0;
    
    // Drag handling
    private Point dragStart;
    private Point windowStart;
    
    /**
     * Creates a new IRIX-styled window
     * @param title Window title
     * @param width Initial width
     * @param height Initial height  
     * @param desktop Parent desktop reference
     */
    public IRIXWindow(String title, int width, int height, IRIXDesktop desktop) {
        super(title, true, true, true, true);
        this.desktop = desktop;
        setSize(width, height);
        setDefaultCloseOperation(HIDE_ON_CLOSE);
        
        initializeWindow();
    }
    
    private void initializeWindow() {
        // Remove default title bar
        putClientProperty("JInternalFrame.isPalette", Boolean.FALSE);
        ((javax.swing.plaf.basic.BasicInternalFrameUI)getUI()).setNorthPane(null);
        
        // Add custom title bar
        JPanel customTitleBar = createCustomTitleBar();
        add(customTitleBar, BorderLayout.NORTH);
        
        // Setup window border
        setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(SHADOW, 1),
            BorderFactory.createLineBorder(HIGHLIGHT, 1)
        ));
    }
    
    private JPanel createCustomTitleBar() {
        JPanel titleBar = new JPanel(new BorderLayout());
        titleBar.setBackground(TITLE_BAR_COLOR);
        titleBar.setPreferredSize(new Dimension(getWidth(), TITLE_BAR_HEIGHT));
        titleBar.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(HIGHLIGHT, 1),
            BorderFactory.createLineBorder(SHADOW, 1)
        ));
        
        // Title label
        JLabel titleLabel = new JLabel(" " + getTitle());
        titleLabel.setForeground(Color.WHITE);
        titleLabel.setFont(new Font("Dialog", Font.BOLD, 12));
        
        // Window control buttons
        JPanel controls = createWindowControls();
        
        titleBar.add(titleLabel, BorderLayout.WEST);
        titleBar.add(controls, BorderLayout.EAST);
        
        // Setup drag handling on title bar
        setupDragHandling(titleBar);
        
        return titleBar;
    }
    
    private JPanel createWindowControls() {
        JPanel controls = new JPanel(new FlowLayout(FlowLayout.RIGHT, 2, 0));
        controls.setOpaque(false);
        
        JButton minimize = createTitleButton("_", "Minimize");
        JButton maximize = createTitleButton("\u25A1", "Maximize");
        JButton close = createTitleButton("X", "Close");
        
        minimize.addActionListener(e -> minimizeWindow());
        maximize.addActionListener(e -> toggleMaximize());
        close.addActionListener(e -> closeWindow());
        
        controls.add(minimize);
        controls.add(maximize);
        controls.add(close);
        
        return controls;
    }
    
    private JButton createTitleButton(String text, String tooltip) {
        JButton button = new JButton(text) {
            @Override
            protected void paintComponent(Graphics g) {
                Graphics2D g2d = (Graphics2D) g;
                g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, 
                                     RenderingHints.VALUE_ANTIALIAS_ON);
                
                if (getModel().isPressed()) {
                    paintPressedButton(g2d);
                } else {
                    paintRaisedButton(g2d);
                }
                
                // Draw button text
                g2d.setColor(Color.BLACK);
                FontMetrics fm = g2d.getFontMetrics();
                int x = (getWidth() - fm.stringWidth(getText())) / 2;
                int y = (getHeight() - fm.getHeight()) / 2 + fm.getAscent();
                g2d.drawString(getText(), x, y);
            }
            
            private void paintPressedButton(Graphics2D g2d) {
                g2d.setColor(SHADOW);
                g2d.fillRect(0, 0, getWidth(), getHeight());
                g2d.setColor(HIGHLIGHT);
                g2d.drawRect(0, 0, getWidth()-1, getHeight()-1);
            }
            
            private void paintRaisedButton(Graphics2D g2d) {
                g2d.setColor(LIGHT_GRAY);
                g2d.fillRect(0, 0, getWidth(), getHeight());
                
                // Highlight (top-left)
                g2d.setColor(HIGHLIGHT);
                g2d.drawLine(0, 0, getWidth()-1, 0);
                g2d.drawLine(0, 0, 0, getHeight()-1);
                
                // Shadow (bottom-right)
                g2d.setColor(SHADOW);
                g2d.drawLine(0, getHeight()-1, getWidth()-1, getHeight()-1);
                g2d.drawLine(getWidth()-1, 0, getWidth()-1, getHeight()-1);
            }
        };
        
        button.setPreferredSize(BUTTON_SIZE);
        button.setBorder(BorderFactory.createEmptyBorder());
        button.setContentAreaFilled(false);
        button.setFocusPainted(false);
        button.setFont(new Font("Dialog", Font.BOLD, 10));
        button.setToolTipText(tooltip);
        
        return button;
    }
    
    private void setupDragHandling(JPanel titleBar) {
        titleBar.addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                dragStart = e.getPoint();
                windowStart = getLocation();
                
                // Bring window to front when clicked
                try {
                    setSelected(true);
                } catch (Exception ex) {
                    // Ignore
                }
            }
        });
        
        titleBar.addMouseMotionListener(new MouseMotionAdapter() {
            @Override
            public void mouseDragged(MouseEvent e) {
                if (dragStart != null && windowStart != null) {
                    Point current = e.getPoint();
                    int deltaX = current.x - dragStart.x;
                    int deltaY = current.y - dragStart.y;
                    setLocation(windowStart.x + deltaX, windowStart.y + deltaY);
                }
            }
        });
    }
    
    // ========== Window Operations ==========
    
    private void minimizeWindow() {
        try {
            setIcon(true);
        } catch (Exception ex) {
            // Ignore property veto
        }
    }
    
    private void toggleMaximize() {
        try {
            setMaximum(!isMaximum());
        } catch (Exception ex) {
            // Ignore property veto
        }
    }
    
    private void closeWindow() {
        setVisible(false);
        desktop.removeWindow(this);
    }
    
    // ========== Workspace Support ==========
    
    /**
     * Gets the workspace this window belongs to
     * @return Workspace index (0-based)
     */
    public int getWorkspace() {
        return workspace;
    }
    
    /**
     * Sets the workspace this window belongs to
     * @param workspace Workspace index (0-based)
     */
    public void setWorkspace(int workspace) {
        this.workspace = workspace;
    }
    
    /**
     * Moves this window to the specified workspace
     * @param workspace Target workspace index
     */
    public void moveToWorkspace(int workspace) {
        this.workspace = workspace;
        // Update visibility based on current desktop workspace
        setVisible(workspace == desktop.getCurrentWorkspace());
    }
}
