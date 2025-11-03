// WebBrowser.java
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.net.*;

public class WebBrowser extends JPanel {
    private JTextField addressBar;
    private JEditorPane contentPane;
    private JButton backButton, forwardButton, refreshButton;
    
    public WebBrowser() {
        setLayout(new BorderLayout());
        
        // Toolbar
        JToolBar toolbar = new JToolBar();
        toolbar.setFloatable(false);
        
        backButton = new JButton("←");
        forwardButton = new JButton("→");
        refreshButton = new JButton("Refresh");
        
        addressBar = new JTextField();
        JButton goButton = new JButton("Go");
        
        toolbar.add(backButton);
        toolbar.add(forwardButton);
        toolbar.add(refreshButton);
        toolbar.add(new JLabel(" Address: "));
        toolbar.add(addressBar);
        toolbar.add(goButton);
        
        // Content area
        contentPane = new JEditorPane();
        contentPane.setContentType("text/html");
        contentPane.setEditable(false);
        
        // Set initial page
        contentPane.setText("<html><body><center><h1>IRIX Web Browser</h1>" +
            "<p>Welcome to the IRIX Web Browser</p>" +
            "<p>Try navigating to: about:help</p></center></body></html>");
        
        // Event handlers
        goButton.addActionListener(e -> navigateTo(addressBar.getText()));
        addressBar.addActionListener(e -> navigateTo(addressBar.getText()));
        refreshButton.addActionListener(e -> refreshPage());
        
        backButton.addActionListener(e -> {
            // Implementation would track history
            JOptionPane.showMessageDialog(this, "Back button clicked");
        });
        
        forwardButton.addActionListener(e -> {
            // Implementation would track history  
            JOptionPane.showMessageDialog(this, "Forward button clicked");
        });
        
        add(toolbar, BorderLayout.NORTH);
        add(new JScrollPane(contentPane), BorderLayout.CENTER);
    }
    
    private void navigateTo(String url) {
        try {
            if (url.equals("about:help")) {
                contentPane.setText("<html><body><h1>IRIX Browser Help</h1>" +
                    "<p>This is a simple web browser component.</p>" +
                    "<p>Supported protocols: http, https, file</p>" +
                    "<p>Enter a URL in the address bar to navigate.</p></body></html>");
            } else if (url.startsWith("http://") || url.startsWith("https://")) {
                contentPane.setPage(new URL(url));
                addressBar.setText(url);
            } else if (url.startsWith("file://")) {
                contentPane.setPage(new URL(url));
                addressBar.setText(url);
            } else {
                // Try as file path
                try {
                    java.nio.file.Path path = java.nio.file.Paths.get(url);
                    if (java.nio.file.Files.exists(path)) {
                        contentPane.setPage(path.toUri().toURL());
                        addressBar.setText(path.toUri().toString());
                    } else {
                        throw new Exception("File not found");
                    }
                } catch (Exception e) {
                    contentPane.setText("<html><body><h1>Error</h1><p>Could not load: " + url + 
                        "</p><p>" + e.getMessage() + "</p></body></html>");
                }
            }
        } catch (Exception e) {
            contentPane.setText("<html><body><h1>Navigation Error</h1><p>" + 
                e.getMessage() + "</p></body></html>");
        }
    }
    
    private void refreshPage() {
        String current = addressBar.getText();
        if (!current.isEmpty()) {
            navigateTo(current);
        }
    }
}