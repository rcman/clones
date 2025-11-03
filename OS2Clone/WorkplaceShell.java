public class WorkplaceShell {
    private List<DesktopObject> desktopObjects;
    private StartButton startButton;
    private Taskbar taskbar;
    
    public WorkplaceShell() {
        desktopObjects = new ArrayList<>();
        initializeDesktop();
    }
    
    private void initializeDesktop() {
        // Create desktop icons
        createDesktopIcon("OS/2 System", "system.png", 50, 50);
        createDesktopIcon("Drive C", "drive_c.png", 50, 100);
        createDesktopIcon("Printer", "printer.png", 50, 150);
    }
    
    private void createDesktopIcon(String name, String icon, int x, int y) {
        DesktopIcon desktopIcon = new DesktopIcon(name, icon, x, y);
        desktopObjects.add(desktopIcon);
    }
}