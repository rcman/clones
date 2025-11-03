// TerminalEmulator.java
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

public class TerminalEmulator extends JPanel {
    private JTextArea terminal;
    private JTextField input;
    private List<String> commandHistory;
    private int historyIndex;
    private String currentDirectory;
    
    public TerminalEmulator() {
        setLayout(new BorderLayout());
        commandHistory = new ArrayList<>();
        currentDirectory = System.getProperty("user.home");
        
        terminal = new JTextArea();
        terminal.setBackground(Color.BLACK);
        terminal.setForeground(Color.GREEN);
        terminal.setFont(new Font("Monospaced", Font.PLAIN, 14));
        terminal.setEditable(false);
        
        input = new JTextField();
        input.setBackground(Color.BLACK);
        input.setForeground(Color.GREEN);
        input.setFont(new Font("Monospaced", Font.PLAIN, 14));
        input.setCaretColor(Color.GREEN);
        
        // Welcome message
        terminal.append("IRIX (tm) Version 6.5 IP32\n");
        terminal.append("Copyright 1987-1996 Silicon Graphics, Inc.\n");
        terminal.append("All Rights Reserved.\n\n");
        updatePrompt();
        
        input.addActionListener(e -> executeCommand());
        
        // History navigation
        input.addKeyListener(new KeyAdapter() {
            public void keyPressed(KeyEvent e) {
                if (e.getKeyCode() == KeyEvent.VK_UP) {
                    navigateHistory(-1);
                } else if (e.getKeyCode() == KeyEvent.VK_DOWN) {
                    navigateHistory(1);
                } else if (e.getKeyCode() == KeyEvent.VK_TAB) {
                    autoComplete();
                    e.consume();
                }
            }
        });
        
        add(new JScrollPane(terminal), BorderLayout.CENTER);
        add(input, BorderLayout.SOUTH);
    }
    
    private void updatePrompt() {
        String prompt = "\n" + currentDirectory + " % ";
        terminal.append(prompt);
        terminal.setCaretPosition(terminal.getText().length());
    }
    
    private void executeCommand() {
        String command = input.getText().trim();
        if (command.isEmpty()) {
            updatePrompt();
            input.setText("");
            return;
        }
        
        terminal.append(command + "\n");
        commandHistory.add(command);
        historyIndex = commandHistory.size();
        
        processCommand(command);
        input.setText("");
    }
    
    private void processCommand(String command) {
        String[] parts = command.split(" ");
        String cmd = parts[0].toLowerCase();
        
        switch (cmd) {
            case "ls":
                listFiles();
                break;
            case "pwd":
                terminal.append(currentDirectory + "\n");
                break;
            case "cd":
                changeDirectory(parts.length > 1 ? parts[1] : "");
                break;
            case "clear":
                terminal.setText("");
                break;
            case "date":
                terminal.append(new Date() + "\n");
                break;
            case "whoami":
                terminal.append(System.getProperty("user.name") + "\n");
                break;
            case "echo":
                terminal.append(command.substring(5) + "\n");
                break;
            case "help":
                showHelp();
                break;
            default:
                terminal.append("irix: " + cmd + ": command not found\n");
        }
        
        updatePrompt();
    }
    
    private void listFiles() {
        File dir = new File(currentDirectory);
        File[] files = dir.listFiles();
        if (files != null) {
            for (File file : files) {
                String type = file.isDirectory() ? "d" : "-";
                String name = file.getName();
                terminal.append(type + " " + name + "\n");
            }
        }
    }
    
    private void changeDirectory(String path) {
        if (path.isEmpty()) {
            currentDirectory = System.getProperty("user.home");
        } else if (path.equals("..")) {
            File current = new File(currentDirectory);
            String parent = current.getParent();
            if (parent != null) {
                currentDirectory = parent;
            }
        } else {
            File newDir = new File(currentDirectory, path);
            if (newDir.exists() && newDir.isDirectory()) {
                currentDirectory = newDir.getAbsolutePath();
            } else {
                terminal.append("cd: " + path + ": No such directory\n");
            }
        }
    }
    
    private void showHelp() {
        terminal.append("Available commands:\n");
        terminal.append("  ls          List directory contents\n");
        terminal.append("  cd [dir]    Change directory\n");
        terminal.append("  pwd         Print working directory\n");
        terminal.append("  clear       Clear terminal\n");
        terminal.append("  date        Show current date/time\n");
        terminal.append("  whoami      Show current user\n");
        terminal.append("  echo [text] Print text\n");
        terminal.append("  help        Show this help\n");
    }
    
    private void navigateHistory(int direction) {
        if (commandHistory.isEmpty()) return;
        
        historyIndex += direction;
        if (historyIndex < 0) historyIndex = 0;
        if (historyIndex >= commandHistory.size()) historyIndex = commandHistory.size() - 1;
        
        if (historyIndex >= 0 && historyIndex < commandHistory.size()) {
            input.setText(commandHistory.get(historyIndex));
        }
    }
    
    private void autoComplete() {
        // Basic tab completion for directory names
        String text = input.getText();
        File current = new File(currentDirectory);
        File[] files = current.listFiles();
        
        if (files != null) {
            for (File file : files) {
                if (file.getName().startsWith(text)) {
                    input.setText(file.getName());
                    break;
                }
            }
        }
    }
}