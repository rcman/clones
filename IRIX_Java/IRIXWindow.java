// IRIXWindow.java
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.datatransfer.*;

public class IRIXWindow extends JInternalFrame {
    private final Color LIGHT_GRAY = new Color(192, 192, 192);
    private final Color DARK_GRAY = new Color(128, 128, 128);
    private final Color HIGHLIGHT = new Color(224, 224, 224);
    private final Color SHADOW = new Color(64, 64, 64);
    private IRIXDesktop desktop;
    private int workspace = 0;
    private Point dragStart;
    private Point windowStart;
    
    public IRIXWindow(String title, int width, int height, IRIXDesktop desktop) {
        super(title, true, true, true, true);
        this.desktop = desktop;
        setSize(width, height);
        setDefaultCloseOperation(HIDE_ON_CLOSE);
        createCustomTitleBar();
        setupDragHandling();
    }
    
    private void createCustomTitleBar() {
        putClientProperty("JInternalFrame.isPalette", Boolean.FALSE);
        ((javax.swing.plaf.basic.BasicInternalFrameUI)getUI()).setNorthPane(null);
        
        JPanel customTitleBar = new JPanel(new BorderLayout());
        customTitleBar.setBackground(new Color(0, 102, 204));
        customTitleBar.setPreferredSize(new Dimension(getWidth(), 24));
        customTitleBar.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(HIGHLIGHT, 1),
            BorderFactory.createLineBorder(SHADOW, 1)
        ));
        
        // Title with icon
        JLabel titleLabel = new JLabel(" " + getTitle());
        titleLabel.setForeground(Color.WHITE);
        titleLabel.setFont(new Font("Dialog", Font.BOLD, 12));
        
        // Controls
        JPanel controls = new JPanel(new FlowLayout(FlowLayout.RIGHT, 2, 0));
        controls.setOpaque(false);
        
        JButton minimize = createTitleButton("_");
        JButton maximize = createTitleButton("□");
        JButton close = createTitleButton("X");
        
        minimize.addActionListener(e -> {
            try { setIcon(true); } catch (Exception ex) {}
        });
        
        maximize.addActionListener(e -> {
            try { setMaximum(!isMaximum()); } catch (Exception ex) {}
        });
        
        close.addActionListener(e -> {
            setVisible(false);
            desktop.removeWindow(this);
        });
        
        controls.add(minimize);
        controls.add(maximize);
        controls.add(close);
        
        customTitleBar.add(titleLabel, BorderLayout.WEST);
        customTitleBar.add(controls, BorderLayout.EAST);
        
        add(customTitleBar, BorderLayout.NORTH);
    }
    
    private JButton createTitleButton(String text) {
        JButton button = new JButton(text) {
            @Override
            protected void paintComponent(Graphics g) {
                Graphics2D g2d = (Graphics2D) g;
                
                if (getModel().isPressed()) {
                    g2d.setColor(SHADOW);
                    g2d.fillRect(0, 0, getWidth(), getHeight());
                    g2d.setColor(HIGHLIGHT);
                    g2d.drawRect(0, 0, getWidth()-1, getHeight()-1);
                } else {
                    g2d.setColor(LIGHT_GRAY);
                    g2d.fillRect(0, 0, getWidth(), getHeight());
                    g2d.setColor(HIGHLIGHT);
                    g2d.drawLine(0, 0, getWidth()-1, 0);
                    g2d.drawLine(0, 0, 0, getHeight()-1);
                    g2d.setColor(SHADOW);
                    g2d.drawLine(0, getHeight()-1, getWidth()-1, getHeight()-1);
                    g2d.drawLine(getWidth()-1, 0, getWidth()-1, getHeight()-1);
                }
                
                g2d.setColor(Color.BLACK);
                FontMetrics fm = g2d.getFontMetrics();
                int x = (getWidth() - fm.stringWidth(getText())) / 2;
                int y = (getHeight() - fm.getHeight()) / 2 + fm.getAscent();
                g2d.drawString(getText(), x, y);
            }
        };
        
        button.setPreferredSize(new Dimension(20, 16));
        button.setBorder(BorderFactory.createEmptyBorder());
        button.setContentAreaFilled(false);
        button.setFocusPainted(false);
        button.setFont(new Font("Dialog", Font.BOLD, 10));
        
        return button;
    }
    
    private void setupDragHandling() {
        // Drag the entire window by title bar
        Component[] comps = getComponents();
        for (Component comp : comps) {
            if (comp instanceof JPanel) {
                comp.addMouseListener(new MouseAdapter() {
                    public void mousePressed(MouseEvent e) {
                        dragStart = e.getPoint();
                        windowStart = getLocation();
                    }
                    
                    public void mouseClicked(MouseEvent e) {
                        try { setSelected(true); } catch (Exception ex) {}
                    }
                });
                
                comp.addMouseMotionListener(new MouseMotionAdapter() {
                    public void mouseDragged(MouseEvent e) {
                        if (dragStart != null) {
                            Point current = e.getPoint();
                            int deltaX = current.x - dragStart.x;
                            int deltaY = current.y - dragStart.y;
                            setLocation(windowStart.x + deltaX, windowStart.y + deltaY);
                        }
                    }
                });
                break;
            }
        }
    }
    
    public int getWorkspace() {
        return workspace;
    }
    
    public void setWorkspace(int workspace) {
        this.workspace = workspace;
    }
}