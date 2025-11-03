public class DesktopIcon extends JComponent {
    private String name;
    private ImageIcon icon;
    
    public DesktopIcon(String name, String iconPath, int x, int y) {
        this.name = name;
        this.icon = loadIcon(iconPath);
        
        setBounds(x, y, 64, 64);
        setToolTipText(name);
        
        addMouseListener(new DesktopIconMouseAdapter());
    }
    
    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        
        // Draw icon
        if (icon != null) {
            icon.paintIcon(this, g, 0, 0);
        }
        
        // Draw label
        g.setColor(Color.BLACK);
        g.drawString(name, 0, 70);
    }
    
    private ImageIcon loadIcon(String path) {
        // Load OS/2 style icons
        try {
            return new ImageIcon(getClass().getResource("/icons/" + path));
        } catch (Exception e) {
            // Fallback to simple rectangle
            return createDefaultIcon();
        }
    }
}