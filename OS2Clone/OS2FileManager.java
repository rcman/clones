public class OS2FileManager extends OS2Window {
    private JTree directoryTree;
    private JList<File> fileList;
    private DefaultTreeModel treeModel;
    
    public OS2FileManager() {
        super("File Manager - Drive C", true, true, true, true);
        
        initializeFileManager();
    }
    
    private void initializeFileManager() {
        JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
        
        // Directory tree (left)
        directoryTree = createDirectoryTree();
        JScrollPane treeScroll = new JScrollPane(directoryTree);
        
        // File list (right)
        fileList = createFileList();
        JScrollPane listScroll = new JScrollPane(fileList);
        
        splitPane.setLeftComponent(treeScroll);
        splitPane.setRightComponent(listScroll);
        splitPane.setDividerLocation(200);
        
        getContentPane().add(splitPane, BorderLayout.CENTER);
        
        // Add OS/2 style toolbar
        addToolBar();
    }
    
    private JTree createDirectoryTree() {
        DefaultMutableTreeNode root = new DefaultMutableTreeNode("Drive C");
        // Populate with directory structure
        treeModel = new DefaultTreeModel(root);
        return new JTree(treeModel);
    }
}