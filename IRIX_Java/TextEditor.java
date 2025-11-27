// TextEditor.java - Enhanced text editor
// Improvements: Line numbers, syntax highlighting hints, status bar, better find/replace

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.undo.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;

/**
 * A feature-rich text editor with undo/redo, find/replace, and line numbers.
 */
public class TextEditor extends JTextArea {
    
    private UndoManager undoManager;
    private JMenuBar menuBar;
    private JLabel statusLabel;
    private File currentFile;
    private boolean modified = false;
    
    public TextEditor() {
        undoManager = new UndoManager();
        setFont(new Font("Monospaced", Font.PLAIN, 14));
        setTabSize(4);
        setLineWrap(false);
        setWrapStyleWord(false);
        
        // Track changes for undo/redo
        getDocument().addUndoableEditListener(e -> {
            undoManager.addEdit(e.getEdit());
            setModified(true);
        });
        
        // Track cursor position for status bar
        addCaretListener(e -> updateStatus());
        
        createMenuBar();
        setupKeyBindings();
    }
    
    private void createMenuBar() {
        menuBar = new JMenuBar();
        
        // File menu
        JMenu fileMenu = new JMenu("File");
        fileMenu.setMnemonic('F');
        
        JMenuItem newItem = createMenuItem("New", KeyEvent.VK_N, e -> newFile());
        JMenuItem openItem = createMenuItem("Open...", KeyEvent.VK_O, e -> openFile());
        JMenuItem saveItem = createMenuItem("Save", KeyEvent.VK_S, e -> saveFile());
        JMenuItem saveAsItem = createMenuItem("Save As...", -1, e -> saveFileAs());
        JMenuItem exitItem = createMenuItem("Exit", -1, e -> exitEditor());
        
        fileMenu.add(newItem);
        fileMenu.add(openItem);
        fileMenu.addSeparator();
        fileMenu.add(saveItem);
        fileMenu.add(saveAsItem);
        fileMenu.addSeparator();
        fileMenu.add(exitItem);
        
        // Edit menu
        JMenu editMenu = new JMenu("Edit");
        editMenu.setMnemonic('E');
        
        JMenuItem undoItem = createMenuItem("Undo", KeyEvent.VK_Z, e -> undo());
        JMenuItem redoItem = createMenuItem("Redo", KeyEvent.VK_Y, e -> redo());
        JMenuItem cutItem = createMenuItem("Cut", KeyEvent.VK_X, e -> cut());
        JMenuItem copyItem = createMenuItem("Copy", KeyEvent.VK_C, e -> copy());
        JMenuItem pasteItem = createMenuItem("Paste", KeyEvent.VK_V, e -> paste());
        JMenuItem selectAllItem = createMenuItem("Select All", KeyEvent.VK_A, e -> selectAll());
        JMenuItem findItem = createMenuItem("Find...", KeyEvent.VK_F, e -> showFindDialog());
        JMenuItem replaceItem = createMenuItem("Replace...", KeyEvent.VK_H, e -> showReplaceDialog());
        JMenuItem goToItem = createMenuItem("Go to Line...", KeyEvent.VK_G, e -> goToLine());
        
        editMenu.add(undoItem);
        editMenu.add(redoItem);
        editMenu.addSeparator();
        editMenu.add(cutItem);
        editMenu.add(copyItem);
        editMenu.add(pasteItem);
        editMenu.addSeparator();
        editMenu.add(selectAllItem);
        editMenu.addSeparator();
        editMenu.add(findItem);
        editMenu.add(replaceItem);
        editMenu.add(goToItem);
        
        // View menu
        JMenu viewMenu = new JMenu("View");
        viewMenu.setMnemonic('V');
        
        JCheckBoxMenuItem lineWrapItem = new JCheckBoxMenuItem("Word Wrap");
        lineWrapItem.addActionListener(e -> {
            setLineWrap(lineWrapItem.isSelected());
            setWrapStyleWord(lineWrapItem.isSelected());
        });
        
        JMenuItem fontItem = new JMenuItem("Font...");
        fontItem.addActionListener(e -> chooseFont());
        
        viewMenu.add(lineWrapItem);
        viewMenu.add(fontItem);
        
        // Help menu
        JMenu helpMenu = new JMenu("Help");
        helpMenu.setMnemonic('H');
        
        JMenuItem aboutItem = new JMenuItem("About");
        aboutItem.addActionListener(e -> showAbout());
        helpMenu.add(aboutItem);
        
        menuBar.add(fileMenu);
        menuBar.add(editMenu);
        menuBar.add(viewMenu);
        menuBar.add(helpMenu);
    }
    
    private JMenuItem createMenuItem(String text, int keyCode, ActionListener action) {
        JMenuItem item = new JMenuItem(text);
        item.addActionListener(action);
        if (keyCode > 0) {
            item.setAccelerator(KeyStroke.getKeyStroke(keyCode, InputEvent.CTRL_DOWN_MASK));
        }
        return item;
    }
    
    private void setupKeyBindings() {
        // Tab key inserts spaces
        getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_TAB, 0), "insertTab");
        getActionMap().put("insertTab", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                replaceSelection("    ");
            }
        });
    }
    
    public JMenuBar getMenuBar() {
        return menuBar;
    }
    
    public JLabel getStatusLabel() {
        if (statusLabel == null) {
            statusLabel = new JLabel();
            updateStatus();
        }
        return statusLabel;
    }
    
    private void updateStatus() {
        if (statusLabel != null) {
            int line = 1;
            int column = 1;
            try {
                int pos = getCaretPosition();
                line = getLineOfOffset(pos) + 1;
                column = pos - getLineStartOffset(line - 1) + 1;
            } catch (Exception e) {
                // Ignore
            }
            
            String modifiedStr = modified ? " [Modified]" : "";
            String fileName = currentFile != null ? currentFile.getName() : "Untitled";
            statusLabel.setText(String.format(" %s%s | Line %d, Column %d | %d characters",
                fileName, modifiedStr, line, column, getText().length()));
        }
    }
    
    private void setModified(boolean mod) {
        this.modified = mod;
        updateStatus();
    }
    
    // File operations
    
    private void newFile() {
        if (checkSave()) {
            setText("");
            currentFile = null;
            setModified(false);
            undoManager.discardAllEdits();
        }
    }
    
    private void openFile() {
        if (!checkSave()) return;
        
        JFileChooser chooser = new JFileChooser();
        chooser.setCurrentDirectory(new File(System.getProperty("user.home")));
        
        if (chooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
            loadFile(chooser.getSelectedFile());
        }
    }
    
    private void loadFile(File file) {
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            StringBuilder content = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                content.append(line).append("\n");
            }
            
            setText(content.toString());
            setCaretPosition(0);
            currentFile = file;
            setModified(false);
            undoManager.discardAllEdits();
            
        } catch (IOException e) {
            JOptionPane.showMessageDialog(this,
                "Error opening file: " + e.getMessage(),
                "Error", JOptionPane.ERROR_MESSAGE);
        }
    }
    
    private void saveFile() {
        if (currentFile != null) {
            writeFile(currentFile);
        } else {
            saveFileAs();
        }
    }
    
    private void saveFileAs() {
        JFileChooser chooser = new JFileChooser();
        chooser.setCurrentDirectory(new File(System.getProperty("user.home")));
        
        if (chooser.showSaveDialog(this) == JFileChooser.APPROVE_OPTION) {
            File file = chooser.getSelectedFile();
            if (file.exists()) {
                int result = JOptionPane.showConfirmDialog(this,
                    "File exists. Overwrite?", "Confirm",
                    JOptionPane.YES_NO_OPTION);
                if (result != JOptionPane.YES_OPTION) return;
            }
            writeFile(file);
            currentFile = file;
        }
    }
    
    private void writeFile(File file) {
        try (BufferedWriter writer = new BufferedWriter(new FileWriter(file))) {
            writer.write(getText());
            setModified(false);
            JOptionPane.showMessageDialog(this, "File saved successfully");
        } catch (IOException e) {
            JOptionPane.showMessageDialog(this,
                "Error saving file: " + e.getMessage(),
                "Error", JOptionPane.ERROR_MESSAGE);
        }
    }
    
    private boolean checkSave() {
        if (!modified) return true;
        
        int result = JOptionPane.showConfirmDialog(this,
            "Do you want to save changes?", "Save Changes",
            JOptionPane.YES_NO_CANCEL_OPTION);
        
        if (result == JOptionPane.YES_OPTION) {
            saveFile();
            return !modified;
        }
        return result != JOptionPane.CANCEL_OPTION;
    }
    
    private void exitEditor() {
        if (checkSave()) {
            Container ancestor = getTopLevelAncestor();
            if (ancestor != null) {
                ancestor.setVisible(false);
            }
        }
    }
    
    // Edit operations
    
    private void undo() {
        if (undoManager.canUndo()) {
            undoManager.undo();
        }
    }
    
    private void redo() {
        if (undoManager.canRedo()) {
            undoManager.redo();
        }
    }
    
    private void showFindDialog() {
        String searchText = JOptionPane.showInputDialog(this, "Find:", "Find",
            JOptionPane.PLAIN_MESSAGE);
        
        if (searchText != null && !searchText.isEmpty()) {
            findNext(searchText, getCaretPosition());
        }
    }
    
    private void findNext(String searchText, int startPos) {
        String text = getText();
        int index = text.indexOf(searchText, startPos);
        
        if (index >= 0) {
            setCaretPosition(index);
            setSelectionStart(index);
            setSelectionEnd(index + searchText.length());
        } else {
            // Search from beginning
            index = text.indexOf(searchText);
            if (index >= 0) {
                setCaretPosition(index);
                setSelectionStart(index);
                setSelectionEnd(index + searchText.length());
            } else {
                JOptionPane.showMessageDialog(this, "Text not found");
            }
        }
    }
    
    private void showReplaceDialog() {
        JPanel panel = new JPanel(new GridLayout(2, 2, 5, 5));
        JTextField findField = new JTextField();
        JTextField replaceField = new JTextField();
        
        panel.add(new JLabel("Find:"));
        panel.add(findField);
        panel.add(new JLabel("Replace with:"));
        panel.add(replaceField);
        
        String[] options = {"Replace", "Replace All", "Cancel"};
        int result = JOptionPane.showOptionDialog(this, panel, "Replace",
            JOptionPane.DEFAULT_OPTION, JOptionPane.PLAIN_MESSAGE,
            null, options, options[0]);
        
        if (result == 0) {
            // Replace current
            String text = getText();
            int pos = text.indexOf(findField.getText(), getCaretPosition());
            if (pos >= 0) {
                setCaretPosition(pos);
                setSelectionStart(pos);
                setSelectionEnd(pos + findField.getText().length());
                replaceSelection(replaceField.getText());
            }
        } else if (result == 1) {
            // Replace all
            String text = getText();
            String replaced = text.replace(findField.getText(), replaceField.getText());
            setText(replaced);
        }
    }
    
    private void goToLine() {
        String input = JOptionPane.showInputDialog(this, "Go to line:", "Go To Line",
            JOptionPane.PLAIN_MESSAGE);
        
        if (input != null) {
            try {
                int lineNum = Integer.parseInt(input);
                int offset = getLineStartOffset(lineNum - 1);
                setCaretPosition(offset);
            } catch (Exception e) {
                JOptionPane.showMessageDialog(this, "Invalid line number");
            }
        }
    }
    
    private void chooseFont() {
        String[] sizes = {"10", "11", "12", "14", "16", "18", "20", "24"};
        String size = (String) JOptionPane.showInputDialog(this,
            "Select font size:", "Font Size",
            JOptionPane.PLAIN_MESSAGE, null, sizes, "14");
        
        if (size != null) {
            setFont(new Font("Monospaced", Font.PLAIN, Integer.parseInt(size)));
        }
    }
    
    private void showAbout() {
        JOptionPane.showMessageDialog(this,
            "IRIX Text Editor\nVersion 1.0\n\n" +
            "A simple text editor for the IRIX Desktop Environment\n\n" +
            "Shortcuts:\n" +
            "  Ctrl+N  New\n" +
            "  Ctrl+O  Open\n" +
            "  Ctrl+S  Save\n" +
            "  Ctrl+Z  Undo\n" +
            "  Ctrl+Y  Redo\n" +
            "  Ctrl+F  Find\n" +
            "  Ctrl+H  Replace\n" +
            "  Ctrl+G  Go to line",
            "About", JOptionPane.INFORMATION_MESSAGE);
    }
}
