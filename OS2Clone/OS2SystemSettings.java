public class OS2SystemSettings extends OS2Window {
    private JTabbedPane tabbedPane;
    
    public OS2SystemSettings() {
        super("System Setup", true, true, true, true);
        
        initializeComponents();
    }
    
    private void initializeComponents() {
        tabbedPane = new JTabbedPane(JTabbedPane.TOP);
        tabbedPane.setBackground(new Color(0xC0C0C0));
        
        // Create OS/2 style tabs
        tabbedPane.addTab("Scheme", createSchemePanel());
        tabbedPane.addTab("Font", createFontPanel());
        tabbedPane.addTab("Mouse", createMousePanel());
        tabbedPane.addTab("Sound", createSoundPanel());
        
        getContentPane().add(tabbedPane);
    }
    
    private JPanel createSchemePanel() {
        JPanel panel = new JPanel(new GridLayout(4, 2));
        panel.setBackground(new Color(0xC0C0C0));
        
        // OS/2 style color scheme options
        String[] schemes = {"OS/2 Default", "High Contrast", "Ocean", "Desert"};
        JComboBox<String> schemeCombo = new JComboBox<>(schemes);
        
        panel.add(new JLabel("Color Scheme:"));
        panel.add(schemeCombo);
        
        return panel;
    }
}