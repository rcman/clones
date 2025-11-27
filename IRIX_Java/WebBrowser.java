// WebBrowser.java - Enhanced web browser with history and bookmarks
// Improvements: History, bookmarks, better error handling, home page

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.net.*;
import java.util.*;

/**
 * A simple web browser with history and bookmark support.
 */
public class WebBrowser extends JPanel {
    
    private JTextField addressBar;
    private JEditorPane contentPane;
    private JButton backButton, forwardButton, refreshButton, homeButton;
    private JComboBox<String> bookmarksCombo;
    
    private final java.util.List<String> history = new ArrayList<>();
    private int historyIndex = -1;
    private final java.util.List<String> bookmarks = new ArrayList<>();
    
    private static final String HOME_PAGE = "about:home";
    
    public WebBrowser() {
        setLayout(new BorderLayout());
        
        initializeBookmarks();
        initializeUI();
        navigateTo(HOME_PAGE);
    }
    
    private void initializeBookmarks() {
        bookmarks.add("about:home");
        bookmarks.add("about:help");
        bookmarks.add("file://" + System.getProperty("user.home"));
    }
    
    private void initializeUI() {
        // Toolbar
        JToolBar toolbar = new JToolBar();
        toolbar.setFloatable(false);
        
        backButton = createButton("\u2190", "Back");
        forwardButton = createButton("\u2192", "Forward");
        refreshButton = createButton("\u21BB", "Refresh");
        homeButton = createButton("\u2302", "Home");
        
        backButton.addActionListener(e -> goBack());
        forwardButton.addActionListener(e -> goForward());
        refreshButton.addActionListener(e -> refresh());
        homeButton.addActionListener(e -> navigateTo(HOME_PAGE));
        
        toolbar.add(backButton);
        toolbar.add(forwardButton);
        toolbar.add(refreshButton);
        toolbar.add(homeButton);
        toolbar.addSeparator();
        
        // Address bar
        addressBar = new JTextField();
        addressBar.setFont(new Font("SansSerif", Font.PLAIN, 12));
        addressBar.addActionListener(e -> navigateTo(addressBar.getText()));
        
        JButton goButton = new JButton("Go");
        goButton.addActionListener(e -> navigateTo(addressBar.getText()));
        
        JPanel addressPanel = new JPanel(new BorderLayout(5, 0));
        addressPanel.add(new JLabel(" URL: "), BorderLayout.WEST);
        addressPanel.add(addressBar, BorderLayout.CENTER);
        addressPanel.add(goButton, BorderLayout.EAST);
        
        toolbar.add(addressPanel);
        toolbar.addSeparator();
        
        // Bookmarks
        JButton bookmarkBtn = new JButton("\u2606");
        bookmarkBtn.setToolTipText("Add Bookmark");
        bookmarkBtn.addActionListener(e -> addBookmark());
        toolbar.add(bookmarkBtn);
        
        add(toolbar, BorderLayout.NORTH);
        
        // Content area
        contentPane = new JEditorPane();
        contentPane.setContentType("text/html");
        contentPane.setEditable(false);
        
        // Handle link clicks
        contentPane.addHyperlinkListener(e -> {
            if (e.getEventType() == javax.swing.event.HyperlinkEvent.EventType.ACTIVATED) {
                navigateTo(e.getURL().toString());
            }
        });
        
        add(new JScrollPane(contentPane), BorderLayout.CENTER);
        
        // Status bar
        JPanel statusBar = new JPanel(new BorderLayout());
        JLabel statusLabel = new JLabel(" Ready");
        statusBar.add(statusLabel, BorderLayout.WEST);
        add(statusBar, BorderLayout.SOUTH);
        
        updateNavButtons();
    }
    
    private JButton createButton(String text, String tooltip) {
        JButton btn = new JButton(text);
        btn.setToolTipText(tooltip);
        btn.setFocusPainted(false);
        return btn;
    }
    
    private void navigateTo(String url) {
        try {
            if (url.equals("about:home")) {
                showHomePage();
            } else if (url.equals("about:help")) {
                showHelpPage();
            } else if (url.equals("about:bookmarks")) {
                showBookmarksPage();
            } else if (url.startsWith("http://") || url.startsWith("https://")) {
                contentPane.setPage(new URL(url));
                addToHistory(url);
            } else if (url.startsWith("file://")) {
                contentPane.setPage(new URL(url));
                addToHistory(url);
            } else {
                // Try to treat as file path or add http://
                try {
                    java.nio.file.Path path = java.nio.file.Paths.get(url);
                    if (java.nio.file.Files.exists(path)) {
                        contentPane.setPage(path.toUri().toURL());
                        addToHistory(path.toUri().toString());
                    } else {
                        // Try with http://
                        String httpUrl = "http://" + url;
                        contentPane.setPage(new URL(httpUrl));
                        addToHistory(httpUrl);
                    }
                } catch (Exception e) {
                    showError("Could not load: " + url + "\n" + e.getMessage());
                }
            }
            
            addressBar.setText(url);
            
        } catch (Exception e) {
            showError("Navigation error: " + e.getMessage());
        }
        
        updateNavButtons();
    }
    
    private void showHomePage() {
        String html = "<html><head><style>" +
            "body { font-family: sans-serif; text-align: center; padding: 50px; " +
            "background: linear-gradient(#2a4a7a, #1a3a6a); color: white; }" +
            "h1 { font-size: 48px; margin-bottom: 10px; }" +
            "p { color: #aaccff; }" +
            ".links { margin-top: 30px; }" +
            ".links a { color: #88ddff; margin: 0 15px; text-decoration: none; }" +
            ".links a:hover { text-decoration: underline; }" +
            "</style></head><body>" +
            "<h1>IRIX Web Browser</h1>" +
            "<p>Welcome to the IRIX Web Browser</p>" +
            "<div class='links'>" +
            "<a href='about:help'>Help</a> | " +
            "<a href='about:bookmarks'>Bookmarks</a> | " +
            "<a href='file://" + System.getProperty("user.home") + "'>Home Folder</a>" +
            "</div></body></html>";
        
        contentPane.setText(html);
        addressBar.setText("about:home");
    }
    
    private void showHelpPage() {
        String html = "<html><body style='font-family: sans-serif; padding: 20px;'>" +
            "<h1>IRIX Browser Help</h1>" +
            "<h2>Features</h2>" +
            "<ul>" +
            "<li>Browse local HTML files</li>" +
            "<li>Navigate with Back/Forward buttons</li>" +
            "<li>Bookmark pages for quick access</li>" +
            "<li>View files from your computer</li>" +
            "</ul>" +
            "<h2>Supported URLs</h2>" +
            "<ul>" +
            "<li><b>http://</b> and <b>https://</b> - Web pages</li>" +
            "<li><b>file://</b> - Local files</li>" +
            "<li><b>about:home</b> - Home page</li>" +
            "<li><b>about:help</b> - This help page</li>" +
            "<li><b>about:bookmarks</b> - Your bookmarks</li>" +
            "</ul>" +
            "<p><a href='about:home'>Return to Home</a></p>" +
            "</body></html>";
        
        contentPane.setText(html);
        addressBar.setText("about:help");
    }
    
    private void showBookmarksPage() {
        StringBuilder html = new StringBuilder();
        html.append("<html><body style='font-family: sans-serif; padding: 20px;'>");
        html.append("<h1>Bookmarks</h1><ul>");
        
        for (String bookmark : bookmarks) {
            html.append("<li><a href='").append(bookmark).append("'>")
                .append(bookmark).append("</a></li>");
        }
        
        html.append("</ul>");
        html.append("<p><a href='about:home'>Return to Home</a></p>");
        html.append("</body></html>");
        
        contentPane.setText(html.toString());
        addressBar.setText("about:bookmarks");
    }
    
    private void showError(String message) {
        String html = "<html><body style='font-family: sans-serif; padding: 20px; " +
            "text-align: center;'>" +
            "<h1 style='color: #cc0000;'>Error</h1>" +
            "<p>" + message.replace("<", "&lt;").replace(">", "&gt;") + "</p>" +
            "<p><a href='about:home'>Return to Home</a></p>" +
            "</body></html>";
        
        contentPane.setText(html);
    }
    
    private void addToHistory(String url) {
        // Remove any forward history
        while (history.size() > historyIndex + 1) {
            history.remove(history.size() - 1);
        }
        
        history.add(url);
        historyIndex = history.size() - 1;
    }
    
    private void goBack() {
        if (historyIndex > 0) {
            historyIndex--;
            String url = history.get(historyIndex);
            addressBar.setText(url);
            try {
                if (url.startsWith("about:")) {
                    if (url.equals("about:home")) showHomePage();
                    else if (url.equals("about:help")) showHelpPage();
                    else if (url.equals("about:bookmarks")) showBookmarksPage();
                } else {
                    contentPane.setPage(new URL(url));
                }
            } catch (Exception e) {
                showError(e.getMessage());
            }
        }
        updateNavButtons();
    }
    
    private void goForward() {
        if (historyIndex < history.size() - 1) {
            historyIndex++;
            String url = history.get(historyIndex);
            addressBar.setText(url);
            try {
                if (url.startsWith("about:")) {
                    if (url.equals("about:home")) showHomePage();
                    else if (url.equals("about:help")) showHelpPage();
                    else if (url.equals("about:bookmarks")) showBookmarksPage();
                } else {
                    contentPane.setPage(new URL(url));
                }
            } catch (Exception e) {
                showError(e.getMessage());
            }
        }
        updateNavButtons();
    }
    
    private void refresh() {
        String current = addressBar.getText();
        if (!current.isEmpty()) {
            navigateTo(current);
        }
    }
    
    private void addBookmark() {
        String current = addressBar.getText();
        if (!current.isEmpty() && !bookmarks.contains(current)) {
            bookmarks.add(current);
            JOptionPane.showMessageDialog(this, "Bookmark added!");
        }
    }
    
    private void updateNavButtons() {
        backButton.setEnabled(historyIndex > 0);
        forwardButton.setEnabled(historyIndex < history.size() - 1);
    }
}
