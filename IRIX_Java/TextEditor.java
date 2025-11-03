// TextEditor.java
import javax.swing.*;
import javax.swing.event.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;

public class TextEditor extends JTextArea {
    private Timer updateTimer;
    private JMenuBar menuBar;
    
    public TextEditor() {
        setFont(new Font("Monospaced", Font.PLAIN, 14));
        setTabSize(4);
        setLineWrap(false);
        setWrapStyleWord(false);
        
        // Create menu bar
        createMenuBar();
    }
    
    private void createMenuBar() {
        menuBar = new JMenuBar();
        
        JMenu fileMenu = new JMenu("File");
        JMenuItem newItem = new JMenuItem("New");
        JMenuItem openItem = new JMenuItem("Open");
        JMenuItem saveItem = new JMenuItem("Save");
        JMenuItem exitItem = new JMenuItem("Exit");
        
        newItem.addActionListener(e -> setText(""));
        openItem.addActionListener(e -> openFile());
        saveItem.addActionListener(e -> saveFile());
        exitItem.addActionListener(e -> {
            Container ancestor = getTopLevelAncestor();
            if (ancestor != null) {
                ancestor.setVisible(false);
            }
        });
        
        // Add keyboard shortcuts
        newItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_N, InputEvent.CTRL_DOWN_MASK));
        openItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_O, InputEvent.CTRL_DOWN_MASK));
        saveItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_S, InputEvent.CTRL_DOWN_MASK));
        
        fileMenu.add(newItem);
        fileMenu.add(openItem);
        fileMenu.add(saveItem);
        fileMenu.addSeparator();
        fileMenu.add(exitItem);
        
        JMenu editMenu = new JMenu("Edit");
        JMenuItem cutItem = new JMenuItem("Cut");
        JMenuItem copyItem = new JMenuItem("Copy");
        JMenuItem pasteItem = new JMenuItem("Paste");
        JMenuItem selectAllItem = new JMenuItem("Select All");
        JMenuItem findItem = new JMenuItem("Find...");
        
        cutItem.addActionListener(e -> cut());
        copyItem.addActionListener(e -> copy());
        pasteItem.addActionListener(e -> paste());
        selectAllItem.addActionListener(e -> selectAll());
        findItem.addActionListener(e -> showFindDialog());
        
        // Add keyboard shortcuts
        cutItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_X, InputEvent.CTRL_DOWN_MASK));
        copyItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_C, InputEvent.CTRL_DOWN_MASK));
        pasteItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_V, InputEvent.CTRL_DOWN_MASK));
        selectAllItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_A, InputEvent.CTRL_DOWN_MASK));
        findItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F, InputEvent.CTRL_DOWN_MASK));
        
        editMenu.add(cutItem);
        editMenu.add(copyItem);
        editMenu.add(pasteItem);
        editMenu.addSeparator();
        editMenu.add(selectAllItem);
        editMenu.add(findItem);
        
        JMenu viewMenu = new JMenu("View");
        JCheckBoxMenuItem lineWrapItem = new JCheckBoxMenuItem("Line Wrap");
        lineWrapItem.addActionListener(e -> {
            setLineWrap(lineWrapItem.isSelected());
            setWrapStyleWord(lineWrapItem.isSelected());
        });
        viewMenu.add(lineWrapItem);
        
        JMenu helpMenu = new JMenu("Help");
        JMenuItem aboutItem = new JMenuItem("About");
        aboutItem.addActionListener(e -> showAboutDialog());
        helpMenu.add(aboutItem);
        
        menuBar.add(fileMenu);
        menuBar.add(editMenu);
        menuBar.add(viewMenu);
        menuBar.add(helpMenu);
    }
    
    public JMenuBar getMenuBar() {
        return menuBar;
    }
    
    private void openFile() {
        JFileChooser chooser = new JFileChooser();
        chooser.setCurrentDirectory(new File(System.getProperty("user.home")));
        
        if (chooser.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
            try {
                File file = chooser.getSelectedFile();
                BufferedReader reader = new BufferedReader(new FileReader(file));
                StringBuilder content = new StringBuilder();
                String line;
                
                while ((line = reader.readLine()) != null) {
                    content.append(line).append("\n");
                }
                reader.close();
                
                setText(content.toString());
                setCaretPosition(0);
            } catch (IOException e) {
                JOptionPane.showMessageDialog(this, 
                    "Error opening file: " + e.getMessage(),
                    "Error",
                    JOptionPane.ERROR_MESSAGE);
            }
        }
    }
    
    private void saveFile() {
        JFileChooser chooser = new JFileChooser();
        chooser.setCurrentDirectory(new File(System.getProperty("user.home")));
        
        if (chooser.showSaveDialog(this) == JFileChooser.APPROVE_OPTION) {
            try {
                File file = chooser.getSelectedFile();
                BufferedWriter writer = new BufferedWriter(new FileWriter(file));
                writer.write(getText());
                writer.close();
                
                JOptionPane.showMessageDialog(this, 
                    "File saved successfully",
                    "Success",
                    JOptionPane.INFORMATION_MESSAGE);
            } catch (IOException e) {
                JOptionPane.showMessageDialog(this, 
                    "Error saving file: " + e.getMessage(),
                    "Error",
                    JOptionPane.ERROR_MESSAGE);
            }
        }
    }
    
    private void showFindDialog() {
        String searchText = JOptionPane.showInputDialog(this, "Find:", "Find", JOptionPane.PLAIN_MESSAGE);
        
        if (searchText != null && !searchText.isEmpty()) {
            String text = getText();
            int index = text.indexOf(searchText, getCaretPosition());
            
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
                    JOptionPane.showMessageDialog(this, 
                        "Text not found",
                        "Find",
                        JOptionPane.INFORMATION_MESSAGE);
                }
            }
        }
    }
    
    private void showAboutDialog() {
        JOptionPane.showMessageDialog(this,
            "IRIX Text Editor\n" +
            "Version 1.0\n\n" +
            "A simple text editor for the IRIX Desktop Environment",
            "About",
            JOptionPane.INFORMATION_MESSAGE);
    }
}
