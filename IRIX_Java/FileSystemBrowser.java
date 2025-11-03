// FileSystemBrowser.java
import javax.swing.*;
import javax.swing.tree.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;

public class FileSystemBrowser extends JPanel {
    private JTree fileTree;
    private DefaultTreeModel treeModel;
    private JTextArea filePreview;
    
    public FileSystemBrowser() {
        setLayout(new BorderLayout());
        
        // Create file tree
        File rootFile = new File(System.getProperty("user.home"));
        FileNode rootNode = new FileNode(rootFile);
        treeModel = new DefaultTreeModel(rootNode);
        fileTree = new JTree(treeModel);
        
        // Custom renderer for files and folders
        fileTree.setCellRenderer(new FileTreeRenderer());
        
        // Double-click to open files/folders
        fileTree.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) {
                    TreePath path = fileTree.getPathForLocation(e.getX(), e.getY());
                    if (path != null) {
                        FileNode node = (FileNode) path.getLastPathComponent();
                        if (node != null && node.isDirectory()) {
                            node.expand();
                            treeModel.reload(node);
                        } else if (node != null) {
                            previewFile(node.getFile());
                        }
                    }
                }
            }
        });
        
        filePreview = new JTextArea();
        filePreview.setEditable(false);
        filePreview.setFont(new Font("Monospaced", Font.PLAIN, 12));
        
        JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT,
            new JScrollPane(fileTree), new JScrollPane(filePreview));
        splitPane.setDividerLocation(300);
        
        add(splitPane, BorderLayout.CENTER);
        
        // Toolbar
        JToolBar toolbar = new JToolBar();
        toolbar.setFloatable(false);
        
        JButton refreshBtn = new JButton("Refresh");
        refreshBtn.addActionListener(e -> refreshTree());
        
        JButton homeBtn = new JButton("Home");
        homeBtn.addActionListener(e -> goHome());
        
        toolbar.add(refreshBtn);
        toolbar.add(homeBtn);
        
        add(toolbar, BorderLayout.NORTH);
    }
    
    private void previewFile(File file) {
        try {
            if (file.length() > 100000) { // Don't preview large files
                filePreview.setText("File too large to preview");
                return;
            }
            
            BufferedReader reader = new BufferedReader(new FileReader(file));
            StringBuilder content = new StringBuilder();
            String line;
            int lines = 0;
            while ((line = reader.readLine()) != null && lines < 100) {
                content.append(line).append("\n");
                lines++;
            }
            reader.close();
            filePreview.setText(content.toString());
            
            if (lines == 100) {
                filePreview.append("\n... (file truncated)");
            }
        } catch (IOException e) {
            filePreview.setText("Could not read file: " + e.getMessage());
        }
    }
    
    private void refreshTree() {
        FileNode root = (FileNode) treeModel.getRoot();
        root.refresh();
        treeModel.reload(root);
    }
    
    private void goHome() {
        File home = new File(System.getProperty("user.home"));
        FileNode root = new FileNode(home);
        treeModel.setRoot(root);
    }
    
    // File node for tree
    class FileNode extends DefaultMutableTreeNode {
        private File file;
        private boolean loaded = false;
        
        public FileNode(File file) {
            super(file.getName());
            this.file = file;
            
            if (file.isDirectory()) {
                add(new DefaultMutableTreeNode("Loading..."));
            }
        }
        
        public File getFile() { return file; }
        public boolean isDirectory() { return file.isDirectory(); }
        
        public void expand() {
            if (!loaded && file.isDirectory()) {
                removeAllChildren();
                File[] files = file.listFiles();
                if (files != null) {
                    for (File child : files) {
                        add(new FileNode(child));
                    }
                }
                loaded = true;
            }
        }
        
        public void refresh() {
            if (file.isDirectory()) {
                removeAllChildren();
                loaded = false;
                add(new DefaultMutableTreeNode("Loading..."));
            }
        }
        
        @Override
        public String toString() {
            return file.getName();
        }
    }
    
    // Custom tree renderer
    class FileTreeRenderer extends DefaultTreeCellRenderer {
        private Icon folderIcon = UIManager.getIcon("FileView.directoryIcon");
        private Icon fileIcon = UIManager.getIcon("FileView.fileIcon");
        
        @Override
        public Component getTreeCellRendererComponent(JTree tree, Object value,
                boolean sel, boolean expanded, boolean leaf, int row, boolean hasFocus) {
            super.getTreeCellRendererComponent(tree, value, sel, expanded, leaf, row, hasFocus);
            
            if (value instanceof FileNode) {
                FileNode node = (FileNode) value;
                if (node.isDirectory()) {
                    setIcon(folderIcon);
                } else {
                    setIcon(fileIcon);
                }
            }
            
            return this;
        }
    }
}