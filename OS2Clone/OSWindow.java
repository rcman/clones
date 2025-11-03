public class OS2Window extends JInternalFrame {
    private static final Color OS2_BLUE = new Color(0, 0, 128);
    private static final Color OS2_GRAY = new Color(0xC0C0C0);
    
    public OS2Window(String title, boolean resizable, boolean closable, 
                    boolean maximizable, boolean iconifiable) {
        super(title, resizable, closable, maximizable, iconifiable);
        
        setUI(new OS2WindowUI());
        setBackground(OS2_GRAY);
        setFrameIcon(createOS2FrameIcon());
        
        // OS/2 style window setup
        setBounds(100, 100, 400, 300);
    }
    
    private ImageIcon createOS2FrameIcon() {
        BufferedImage icon = new BufferedImage(16, 16, BufferedImage.TYPE_INT_RGB);
        Graphics2D g2d = icon.createGraphics();
        
        // Draw OS/2 style window icon
        g2d.setColor(OS2_BLUE);
        g2d.fillRect(0, 0, 16, 16);
        g2d.setColor(Color.WHITE);
        g2d.drawString("OS", 2, 12);
        
        g2d.dispose();
        return new ImageIcon(icon);
    }
}