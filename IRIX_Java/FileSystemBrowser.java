// FileSystemBrowser.java - Enhanced file manager
// Improvements: Better preview, breadcrumb navigation, file operations, sorting

import javax.swing.*;
import javax.swing.tree.*;
import javax.swing.table.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.text.SimpleDateFormat;
import java.util.*;

/**
 * A dual-pane file browser with tree navigation and file preview.
 */
public class FileSystemBrowser extends JPanel {
    
    private JTree fileTree;
    private DefaultTreeModel treeModel;
    private JTable fileTable;
    private DefaultTableModel tableModel;
    private JTextArea filePreview;
    private JLabel pathLabel;
    private JLabel statusLabel;
    private File currentDirectory;
    
    private static final SimpleDateFormat DATE_FORMAT = 
        new SimpleDateFormat("MMM dd, yyyy HH:mm");
    
    public FileSystemBrowser() {
        setLayout(new BorderLayout());
        currentDirectory = new File(System.getProperty("user.home"));
        
        initializeUI();
        navigateTo(currentDirectory);
    }
    
    private void initializeUI() {
        // Toolbar
        JToolBar toolbar = createToolbar();
        add(toolbar, BorderLayout.NORTH);
        
        // Main content - three panes
        JSplitPane mainSplit = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
        mainSplit.setDividerLocation(200);
        
        // Left pane - Tree view
        JPanel treePanel = createTreePanel();
        mainSplit.setLeftComponent(treePanel);
        
        // Right side - split between table and preview
        JSplitPane rightSplit = new JSplitPane(JSplitPane.VERTICAL_SPLIT);
        rightSplit.setDividerLocation(300);
        
        // File table
        JPanel tablePanel = createTablePanel();
        rightSplit.setTopComponent(tablePanel);
        
        // Preview panel
        JPanel previewPanel = createPreviewPanel();
        rightSplit.setBottomComponent(previewPanel);
        
        mainSplit.setRightComponent(rightSplit);
        add(mainSplit, BorderLayout.CENTER);
        
        // Status bar
        statusLabel = new JLabel(" Ready");
        statusLabel.setBorder(BorderFactory.createEmptyBorder(2, 5, 2, 5));
        add(statusLabel, BorderLayout.SOUTH);
    }
    
    private JToolBar createToolbar() {
        JToolBar toolbar = new JToolBar();
        toolbar.setFloatable(false);
        
        JButton backBtn = new JButton("\u2190 Back");
        backBtn.addActionListener(e -> goBack());
        
        JButton upBtn = new JButton("\u2191 Up");
        upBtn.addActionListener(e -> goUp());
        
        JButton homeBtn = new JButton("\u2302 Home");
        homeBtn.addActionListener(e -> goHome());
        
        JButton refreshBtn = new JButton("\u21BB Refresh");
        refreshBtn.addActionListener(e -> refresh());
        
        toolbar.add(backBtn);
        toolbar.add(upBtn);
        toolbar.add(homeBtn);
        toolbar.addSeparator();
        toolbar.add(refreshBtn);
        toolbar.addSeparator();
        
        // Path display
        pathLabel = new JLabel(" ");
        pathLabel.setFont(new Font("Monospaced", Font.PLAIN, 11));
        toolbar.add(new JLabel("Path: "));
        toolbar.add(pathLabel);
        
        return toolbar;
    }
    
    private JPanel createTreePanel() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBorder(BorderFactory.createTitledBorder("Folders"));
        
        // Create root node
        File[] roots = File.listRoots();
        DefaultMutableTreeNode rootNode = new DefaultMutableTreeNode("Computer");
        
        for (File root : roots) {
            FileNode node = new FileNode(root);
            rootNode.add(node);
        }
        
        // Add home directory for quick access
        FileNode homeNode = new FileNode(new File(System.getProperty("user.home")));
        rootNode.insert(homeNode, 0);
        
        treeModel = new DefaultTreeModel(rootNode);
        fileTree = new JTree(treeModel);
        fileTree.setCellRenderer(new FileTreeRenderer());
        fileTree.setRootVisible(true);
        fileTree.setShowsRootHandles(true);
        
        // Expand home by default
        fileTree.expandRow(1);
        
        // Tree selection listener
        fileTree.addTreeSelectionListener(e -> {
            TreePath path = e.getPath();
            if (path != null) {
                Object node = path.getLastPathComponent();
                if (node instanceof FileNode) {
                    FileNode fileNode = (FileNode) node;
                    navigateTo(fileNode.getFile());
                }
            }
        });
        
        // Tree expansion listener
        fileTree.addTreeWillExpandListener(new javax.swing.event.TreeWillExpandListener() {
            @Override
            public void treeWillExpand(javax.swing.event.TreeExpansionEvent event) {
                TreePath path = event.getPath();
                Object node = path.getLastPathComponent();
                if (node instanceof FileNode) {
                    ((FileNode) node).expand();
                }
            }
            
            @Override
            public void treeWillCollapse(javax.swing.event.TreeExpansionEvent event) {}
        });
        
        panel.add(new JScrollPane(fileTree), BorderLayout.CENTER);
        return panel;
    }
    
    private JPanel createTablePanel() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBorder(BorderFactory.createTitledBorder("Files"));
        
        String[] columns = {"Name", "Size", "Type", "Modified"};
        tableModel = new DefaultTableModel(columns, 0) {
            @Override
            public boolean isCellEditable(int row, int column) {
                return false;
            }
        };
        
        fileTable = new JTable(tableModel);
        fileTable.setFont(new Font("SansSerif", Font.PLAIN, 12));
        fileTable.getTableHeader().setFont(new Font("SansSerif", Font.BOLD, 12));
        fileTable.setRowHeight(20);
        fileTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        
        // Set column widths
        fileTable.getColumnModel().getColumn(0).setPreferredWidth(200);
        fileTable.getColumnModel().getColumn(1).setPreferredWidth(80);
        fileTable.getColumnModel().getColumn(2).setPreferredWidth(100);
        fileTable.getColumnModel().getColumn(3).setPreferredWidth(120);
        
        // Selection listener for preview
        fileTable.getSelectionModel().addListSelectionListener(e -> {
            if (!e.getValueIsAdjusting()) {
                int row = fileTable.getSelectedRow();
                if (row >= 0) {
                    String name = (String) tableModel.getValueAt(row, 0);
                    File file = new File(currentDirectory, name);
                    previewFile(file);
                }
            }
        });
        
        // Double-click to open
        fileTable.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) {
                    int row = fileTable.getSelectedRow();
                    if (row >= 0) {
                        String name = (String) tableModel.getValueAt(row, 0);
                        File file = new File(currentDirectory, name);
                        if (file.isDirectory()) {
                            navigateTo(file);
                        } else {
                            openFile(file);
                        }
                    }
                }
            }
        });
        
        // Context menu
        JPopupMenu contextMenu = createContextMenu();
        fileTable.setComponentPopupMenu(contextMenu);
        
        panel.add(new JScrollPane(fileTable), BorderLayout.CENTER);
        return panel;
    }
    
    private JPopupMenu createContextMenu() {
        JPopupMenu menu = new JPopupMenu();
        
        JMenuItem openItem = new JMenuItem("Open");
        openItem.addActionListener(e -> openSelectedFile());
        
        JMenuItem copyItem = new JMenuItem("Copy Path");
        copyItem.addActionListener(e -> copyPath());
        
        JMenuItem propertiesItem = new JMenuItem("Properties");
        propertiesItem.addActionListener(e -> showProperties());
        
        menu.add(openItem);
        menu.addSeparator();
        menu.add(copyItem);
        menu.addSeparator();
        menu.add(propertiesItem);
        
        return menu;
    }
    
    private JPanel createPreviewPanel() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBorder(BorderFactory.createTitledBorder("Preview"));
        
        filePreview = new JTextArea();
        filePreview.setEditable(false);
        filePreview.setFont(new Font("Monospaced", Font.PLAIN, 12));
        filePreview.setLineWrap(true);
        filePreview.setWrapStyleWord(true);
        
        panel.add(new JScrollPane(filePreview), BorderLayout.CENTER);
        return panel;
    }
    
    // Navigation methods
    
    private java.util.List<File> history = new ArrayList<>();
    private int historyIndex = -1;
    
    private void navigateTo(File directory) {
        if (!directory.isDirectory()) return;
        
        // Add to history
        if (historyIndex < 0 || !directory.equals(history.get(historyIndex))) {
            // Remove any forward history
            while (history.size() > historyIndex + 1) {
                history.remove(history.size() - 1);
            }
            history.add(directory);
            historyIndex = history.size() - 1;
        }
        
        currentDirectory = directory;
        pathLabel.setText(directory.getAbsolutePath());
        
        // Update table
        updateFileTable();
        
        // Clear preview
        filePreview.setText("");
        
        updateStatus();
    }
    
    private void updateFileTable() {
        tableModel.setRowCount(0);
        
        File[] files = currentDirectory.listFiles();
        if (files == null) return;
        
        // Sort: directories first, then by name
        Arrays.sort(files, (a, b) -> {
            if (a.isDirectory() && !b.isDirectory()) return -1;
            if (!a.isDirectory() && b.isDirectory()) return 1;
            return a.getName().compareToIgnoreCase(b.getName());
        });
        
        for (File file : files) {
            // Skip hidden files on Unix-like systems
            if (file.getName().startsWith(".")) continue;
            
            String name = file.getName();
            String size = file.isDirectory() ? "--" : formatSize(file.length());
            String type = file.isDirectory() ? "Folder" : getFileType(file);
            String modified = DATE_FORMAT.format(new Date(file.lastModified()));
            
            tableModel.addRow(new Object[]{name, size, type, modified});
        }
    }
    
    private void goBack() {
        if (historyIndex > 0) {
            historyIndex--;
            File prev = history.get(historyIndex);
            currentDirectory = prev;
            pathLabel.setText(prev.getAbsolutePath());
            updateFileTable();
            updateStatus();
        }
    }
    
    private void goUp() {
        File parent = currentDirectory.getParentFile();
        if (parent != null) {
            navigateTo(parent);
        }
    }
    
    private void goHome() {
        navigateTo(new File(System.getProperty("user.home")));
    }
    
    private void refresh() {
        updateFileTable();
        updateStatus();
    }
    
    // File operations
    
    private void previewFile(File file) {
        if (file.isDirectory()) {
            filePreview.setText("[Directory]\n\nContains " + 
                countItems(file) + " items");
            return;
        }
        
        // Check file size
        if (file.length() > 100000) {
            filePreview.setText("[File too large to preview]\n\nSize: " + 
                formatSize(file.length()));
            return;
        }
        
        // Check if it's a text file
        String type = getFileType(file);
        if (!isTextFile(type)) {
            filePreview.setText("[Binary file - no preview available]\n\nType: " + type +
                "\nSize: " + formatSize(file.length()));
            return;
        }
        
        // Read and display text content
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            StringBuilder content = new StringBuilder();
            String line;
            int lines = 0;
            
            while ((line = reader.readLine()) != null && lines < 100) {
                content.append(line).append("\n");
                lines++;
            }
            
            if (lines == 100) {
                content.append("\n[... truncated ...]");
            }
            
            filePreview.setText(content.toString());
            filePreview.setCaretPosition(0);
        } catch (IOException e) {
            filePreview.setText("[Error reading file]\n\n" + e.getMessage());
        }
    }
    
    private void openSelectedFile() {
        int row = fileTable.getSelectedRow();
        if (row >= 0) {
            String name = (String) tableModel.getValueAt(row, 0);
            File file = new File(currentDirectory, name);
            if (file.isDirectory()) {
                navigateTo(file);
            } else {
                openFile(file);
            }
        }
    }
    
    private void openFile(File file) {
        try {
            Desktop.getDesktop().open(file);
        } catch (IOException e) {
            JOptionPane.showMessageDialog(this, 
                "Cannot open file: " + e.getMessage(),
                "Error", JOptionPane.ERROR_MESSAGE);
        }
    }
    
    private void copyPath() {
        int row = fileTable.getSelectedRow();
        if (row >= 0) {
            String name = (String) tableModel.getValueAt(row, 0);
            File file = new File(currentDirectory, name);
            
            java.awt.datatransfer.StringSelection selection = 
                new java.awt.datatransfer.StringSelection(file.getAbsolutePath());
            Toolkit.getDefaultToolkit().getSystemClipboard().setContents(selection, null);
            
            statusLabel.setText(" Path copied to clipboard");
        }
    }
    
    private void showProperties() {
        int row = fileTable.getSelectedRow();
        if (row >= 0) {
            String name = (String) tableModel.getValueAt(row, 0);
            File file = new File(currentDirectory, name);
            
            String message = String.format(
                "Name: %s\n" +
                "Type: %s\n" +
                "Location: %s\n" +
                "Size: %s\n" +
                "Modified: %s\n" +
                "Readable: %s\n" +
                "Writable: %s\n" +
                "Hidden: %s",
                file.getName(),
                file.isDirectory() ? "Folder" : getFileType(file),
                file.getParent(),
                file.isDirectory() ? countItems(file) + " items" : formatSize(file.length()),
                DATE_FORMAT.format(new Date(file.lastModified())),
                file.canRead() ? "Yes" : "No",
                file.canWrite() ? "Yes" : "No",
                file.isHidden() ? "Yes" : "No"
            );
            
            JOptionPane.showMessageDialog(this, message, 
                "Properties - " + file.getName(), JOptionPane.INFORMATION_MESSAGE);
        }
    }
    
    // Utility methods
    
    private String formatSize(long bytes) {
        if (bytes < 1024) return bytes + " B";
        if (bytes < 1024 * 1024) return String.format("%.1f KB", bytes / 1024.0);
        if (bytes < 1024 * 1024 * 1024) return String.format("%.1f MB", bytes / (1024.0 * 1024));
        return String.format("%.1f GB", bytes / (1024.0 * 1024 * 1024));
    }
    
    private String getFileType(File file) {
        String name = file.getName();
        int dot = name.lastIndexOf('.');
        if (dot < 0) return "File";
        
        String ext = name.substring(dot + 1).toLowerCase();
        switch (ext) {
            case "txt": return "Text File";
            case "java": return "Java Source";
            case "py": return "Python Script";
            case "html": case "htm": return "HTML Document";
            case "css": return "CSS Stylesheet";
            case "js": return "JavaScript";
            case "json": return "JSON File";
            case "xml": return "XML Document";
            case "md": return "Markdown";
            case "pdf": return "PDF Document";
            case "doc": case "docx": return "Word Document";
            case "xls": case "xlsx": return "Excel Spreadsheet";
            case "png": case "jpg": case "jpeg": case "gif": return "Image";
            case "mp3": case "wav": case "ogg": return "Audio";
            case "mp4": case "avi": case "mkv": return "Video";
            case "zip": case "tar": case "gz": return "Archive";
            case "sh": return "Shell Script";
            case "c": case "cpp": case "h": return "C/C++ Source";
            default: return ext.toUpperCase() + " File";
        }
    }
    
    private boolean isTextFile(String type) {
        return type.contains("Text") || type.contains("Source") || 
               type.contains("Script") || type.contains("Document") ||
               type.contains("HTML") || type.contains("CSS") ||
               type.contains("JavaScript") || type.contains("JSON") ||
               type.contains("XML") || type.contains("Markdown");
    }
    
    private String countItems(File dir) {
        File[] files = dir.listFiles();
        return files != null ? String.valueOf(files.length) : "0";
    }
    
    private void updateStatus() {
        File[] files = currentDirectory.listFiles();
        int count = files != null ? files.length : 0;
        statusLabel.setText(" " + count + " items in " + currentDirectory.getAbsolutePath());
    }
    
    // Inner classes
    
    class FileNode extends DefaultMutableTreeNode {
        private final File file;
        private boolean loaded = false;
        
        public FileNode(File file) {
            super(file.getName().isEmpty() ? file.getPath() : file.getName());
            this.file = file;
            
            if (file.isDirectory()) {
                add(new DefaultMutableTreeNode("Loading..."));
            }
        }
        
        public File getFile() { return file; }
        
        public void expand() {
            if (!loaded && file.isDirectory()) {
                removeAllChildren();
                File[] files = file.listFiles();
                if (files != null) {
                    Arrays.sort(files, (a, b) -> {
                        if (a.isDirectory() && !b.isDirectory()) return -1;
                        if (!a.isDirectory() && b.isDirectory()) return 1;
                        return a.getName().compareToIgnoreCase(b.getName());
                    });
                    
                    for (File child : files) {
                        if (!child.getName().startsWith(".") && child.isDirectory()) {
                            add(new FileNode(child));
                        }
                    }
                }
                loaded = true;
                treeModel.reload(this);
            }
        }
    }
    
    class FileTreeRenderer extends DefaultTreeCellRenderer {
        @Override
        public Component getTreeCellRendererComponent(JTree tree, Object value,
                boolean sel, boolean expanded, boolean leaf, int row, boolean hasFocus) {
            super.getTreeCellRendererComponent(tree, value, sel, expanded, leaf, row, hasFocus);
            
            if (value instanceof FileNode) {
                setIcon(UIManager.getIcon("FileView.directoryIcon"));
            }
            
            return this;
        }
    }
}
