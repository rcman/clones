// TerminalEmulator.java - Enhanced terminal emulator
// Improvements: More commands, better tab completion, command piping, colored output

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import java.util.List;

/**
 * A terminal emulator that provides a command-line interface
 * with IRIX-style prompt and basic Unix-like commands.
 */
public class TerminalEmulator extends JPanel {
    
    private JTextPane terminal;
    private JTextField input;
    private final List<String> commandHistory;
    private int historyIndex;
    private String currentDirectory;
    
    // Color scheme for terminal output
    private static final Color BG_COLOR = Color.BLACK;
    private static final Color FG_COLOR = Color.GREEN;
    private static final Color ERROR_COLOR = new Color(255, 100, 100);
    private static final Color INFO_COLOR = new Color(100, 200, 255);
    private static final Color PROMPT_COLOR = new Color(200, 200, 100);
    
    // Environment variables (simulated)
    private final Map<String, String> environment;
    
    public TerminalEmulator() {
        setLayout(new BorderLayout());
        commandHistory = new ArrayList<>();
        currentDirectory = System.getProperty("user.home");
        environment = new HashMap<>();
        initializeEnvironment();
        
        initializeUI();
        showWelcomeMessage();
        showPrompt();
    }
    
    private void initializeEnvironment() {
        environment.put("USER", System.getProperty("user.name"));
        environment.put("HOME", System.getProperty("user.home"));
        environment.put("PWD", currentDirectory);
        environment.put("SHELL", "/bin/csh");
        environment.put("TERM", "xterm");
        environment.put("PATH", "/usr/bin:/bin:/usr/local/bin");
    }
    
    private void initializeUI() {
        terminal = new JTextPane();
        terminal.setBackground(BG_COLOR);
        terminal.setForeground(FG_COLOR);
        terminal.setFont(new Font("Monospaced", Font.PLAIN, 14));
        terminal.setEditable(false);
        terminal.setCaretColor(FG_COLOR);
        
        input = new JTextField();
        input.setBackground(BG_COLOR);
        input.setForeground(FG_COLOR);
        input.setFont(new Font("Monospaced", Font.PLAIN, 14));
        input.setCaretColor(FG_COLOR);
        input.setBorder(BorderFactory.createEmptyBorder(5, 5, 5, 5));
        
        input.addActionListener(e -> executeCommand());
        input.addKeyListener(new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent e) {
                switch (e.getKeyCode()) {
                    case KeyEvent.VK_UP:
                        navigateHistory(-1);
                        break;
                    case KeyEvent.VK_DOWN:
                        navigateHistory(1);
                        break;
                    case KeyEvent.VK_TAB:
                        autoComplete();
                        e.consume();
                        break;
                    case KeyEvent.VK_C:
                        if (e.isControlDown()) {
                            appendText("^C\n", FG_COLOR);
                            input.setText("");
                            showPrompt();
                        }
                        break;
                    case KeyEvent.VK_L:
                        if (e.isControlDown()) {
                            terminal.setText("");
                            showPrompt();
                            e.consume();
                        }
                        break;
                }
            }
        });
        
        JScrollPane scrollPane = new JScrollPane(terminal);
        scrollPane.setBorder(BorderFactory.createEmptyBorder());
        scrollPane.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
        
        add(scrollPane, BorderLayout.CENTER);
        add(input, BorderLayout.SOUTH);
    }
    
    private void showWelcomeMessage() {
        appendText("IRIX (tm) Version 6.5 IP32\n", INFO_COLOR);
        appendText("Copyright 1987-1996 Silicon Graphics, Inc.\n", INFO_COLOR);
        appendText("All Rights Reserved.\n\n", INFO_COLOR);
        appendText("Type 'help' for available commands.\n\n", FG_COLOR);
    }
    
    private void showPrompt() {
        // Show shortened path in prompt
        String displayPath = currentDirectory;
        String home = System.getProperty("user.home");
        if (displayPath.startsWith(home)) {
            displayPath = "~" + displayPath.substring(home.length());
        }
        appendText(displayPath + " % ", PROMPT_COLOR);
    }
    
    private void appendText(String text, Color color) {
        javax.swing.text.StyledDocument doc = terminal.getStyledDocument();
        javax.swing.text.SimpleAttributeSet attrs = new javax.swing.text.SimpleAttributeSet();
        javax.swing.text.StyleConstants.setForeground(attrs, color);
        
        try {
            doc.insertString(doc.getLength(), text, attrs);
            terminal.setCaretPosition(doc.getLength());
        } catch (javax.swing.text.BadLocationException e) {
            // Ignore
        }
    }
    
    private void executeCommand() {
        String command = input.getText().trim();
        input.setText("");
        
        // Echo command
        appendText(command + "\n", FG_COLOR);
        
        if (command.isEmpty()) {
            showPrompt();
            return;
        }
        
        // Add to history
        commandHistory.add(command);
        historyIndex = commandHistory.size();
        
        // Parse and execute
        processCommand(command);
        showPrompt();
    }
    
    private void processCommand(String fullCommand) {
        // Handle command chaining with semicolon
        if (fullCommand.contains(";")) {
            String[] commands = fullCommand.split(";");
            for (String cmd : commands) {
                processSingleCommand(cmd.trim());
            }
            return;
        }
        
        processSingleCommand(fullCommand);
    }
    
    private void processSingleCommand(String command) {
        if (command.isEmpty()) return;
        
        String[] parts = command.split("\\s+");
        String cmd = parts[0].toLowerCase();
        String[] args = Arrays.copyOfRange(parts, 1, parts.length);
        
        switch (cmd) {
            case "ls":
                listFiles(args);
                break;
            case "pwd":
                appendText(currentDirectory + "\n", FG_COLOR);
                break;
            case "cd":
                changeDirectory(args.length > 0 ? args[0] : "");
                break;
            case "clear":
                terminal.setText("");
                break;
            case "date":
                appendText(new Date() + "\n", FG_COLOR);
                break;
            case "whoami":
                appendText(System.getProperty("user.name") + "\n", FG_COLOR);
                break;
            case "echo":
                handleEcho(args);
                break;
            case "cat":
                catFile(args);
                break;
            case "head":
                headFile(args);
                break;
            case "tail":
                tailFile(args);
                break;
            case "wc":
                wordCount(args);
                break;
            case "mkdir":
                makeDirectory(args);
                break;
            case "touch":
                touchFile(args);
                break;
            case "rm":
                removeFile(args);
                break;
            case "cp":
                copyFile(args);
                break;
            case "mv":
                moveFile(args);
                break;
            case "env":
                showEnvironment();
                break;
            case "export":
                setEnvironment(args);
                break;
            case "history":
                showHistory();
                break;
            case "uname":
                showSystemInfo(args);
                break;
            case "hostname":
                showHostname();
                break;
            case "uptime":
                showUptime();
                break;
            case "df":
                showDiskSpace();
                break;
            case "help":
                showHelp();
                break;
            case "man":
                showManPage(args);
                break;
            case "exit":
                appendText("Use the window close button to exit.\n", INFO_COLOR);
                break;
            default:
                appendText(cmd + ": command not found\n", ERROR_COLOR);
        }
    }
    
    private void listFiles(String[] args) {
        boolean showAll = false;
        boolean longFormat = false;
        String path = currentDirectory;
        
        for (String arg : args) {
            if (arg.equals("-a") || arg.equals("-all")) showAll = true;
            else if (arg.equals("-l")) longFormat = true;
            else if (!arg.startsWith("-")) path = resolvePath(arg);
        }
        
        File dir = new File(path);
        if (!dir.exists() || !dir.isDirectory()) {
            appendText("ls: cannot access '" + path + "': No such directory\n", ERROR_COLOR);
            return;
        }
        
        File[] files = dir.listFiles();
        if (files == null) return;
        
        Arrays.sort(files, (a, b) -> a.getName().compareToIgnoreCase(b.getName()));
        
        for (File file : files) {
            if (!showAll && file.getName().startsWith(".")) continue;
            
            if (longFormat) {
                String type = file.isDirectory() ? "d" : "-";
                String perms = "rwxr-xr-x";
                long size = file.length();
                String modified = new java.text.SimpleDateFormat("MMM dd HH:mm")
                    .format(new Date(file.lastModified()));
                
                String line = String.format("%s%s  1 %s  %8d %s %s\n",
                    type, perms, environment.get("USER"), size, modified, file.getName());
                
                appendText(line, file.isDirectory() ? INFO_COLOR : FG_COLOR);
            } else {
                appendText(file.getName() + (file.isDirectory() ? "/" : "") + "\n",
                    file.isDirectory() ? INFO_COLOR : FG_COLOR);
            }
        }
    }
    
    private void changeDirectory(String path) {
        if (path.isEmpty() || path.equals("~")) {
            currentDirectory = System.getProperty("user.home");
        } else if (path.equals("-")) {
            // Go to previous directory (simplified - just go home)
            currentDirectory = System.getProperty("user.home");
        } else {
            String newPath = resolvePath(path);
            File newDir = new File(newPath);
            
            if (!newDir.exists()) {
                appendText("cd: " + path + ": No such file or directory\n", ERROR_COLOR);
                return;
            }
            if (!newDir.isDirectory()) {
                appendText("cd: " + path + ": Not a directory\n", ERROR_COLOR);
                return;
            }
            
            currentDirectory = newDir.getAbsolutePath();
        }
        environment.put("PWD", currentDirectory);
    }
    
    private String resolvePath(String path) {
        if (path.startsWith("~")) {
            path = System.getProperty("user.home") + path.substring(1);
        }
        if (!path.startsWith("/")) {
            path = currentDirectory + File.separator + path;
        }
        try {
            return new File(path).getCanonicalPath();
        } catch (IOException e) {
            return path;
        }
    }
    
    private void handleEcho(String[] args) {
        StringBuilder output = new StringBuilder();
        for (String arg : args) {
            // Expand environment variables
            if (arg.startsWith("$")) {
                String varName = arg.substring(1);
                String value = environment.get(varName);
                output.append(value != null ? value : "");
            } else {
                output.append(arg);
            }
            output.append(" ");
        }
        appendText(output.toString().trim() + "\n", FG_COLOR);
    }
    
    private void catFile(String[] args) {
        if (args.length == 0) {
            appendText("cat: missing operand\n", ERROR_COLOR);
            return;
        }
        
        for (String arg : args) {
            File file = new File(resolvePath(arg));
            if (!file.exists()) {
                appendText("cat: " + arg + ": No such file\n", ERROR_COLOR);
                continue;
            }
            if (file.isDirectory()) {
                appendText("cat: " + arg + ": Is a directory\n", ERROR_COLOR);
                continue;
            }
            
            try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    appendText(line + "\n", FG_COLOR);
                }
            } catch (IOException e) {
                appendText("cat: " + arg + ": " + e.getMessage() + "\n", ERROR_COLOR);
            }
        }
    }
    
    private void headFile(String[] args) {
        int lines = 10;
        String filename = null;
        
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-n") && i + 1 < args.length) {
                try {
                    lines = Integer.parseInt(args[++i]);
                } catch (NumberFormatException e) {
                    appendText("head: invalid number of lines\n", ERROR_COLOR);
                    return;
                }
            } else if (!args[i].startsWith("-")) {
                filename = args[i];
            }
        }
        
        if (filename == null) {
            appendText("head: missing operand\n", ERROR_COLOR);
            return;
        }
        
        File file = new File(resolvePath(filename));
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            String line;
            int count = 0;
            while ((line = reader.readLine()) != null && count < lines) {
                appendText(line + "\n", FG_COLOR);
                count++;
            }
        } catch (IOException e) {
            appendText("head: " + e.getMessage() + "\n", ERROR_COLOR);
        }
    }
    
    private void tailFile(String[] args) {
        int lines = 10;
        String filename = null;
        
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-n") && i + 1 < args.length) {
                try {
                    lines = Integer.parseInt(args[++i]);
                } catch (NumberFormatException e) {
                    appendText("tail: invalid number of lines\n", ERROR_COLOR);
                    return;
                }
            } else if (!args[i].startsWith("-")) {
                filename = args[i];
            }
        }
        
        if (filename == null) {
            appendText("tail: missing operand\n", ERROR_COLOR);
            return;
        }
        
        File file = new File(resolvePath(filename));
        try {
            List<String> allLines = java.nio.file.Files.readAllLines(file.toPath());
            int start = Math.max(0, allLines.size() - lines);
            for (int i = start; i < allLines.size(); i++) {
                appendText(allLines.get(i) + "\n", FG_COLOR);
            }
        } catch (IOException e) {
            appendText("tail: " + e.getMessage() + "\n", ERROR_COLOR);
        }
    }
    
    private void wordCount(String[] args) {
        if (args.length == 0) {
            appendText("wc: missing operand\n", ERROR_COLOR);
            return;
        }
        
        for (String arg : args) {
            File file = new File(resolvePath(arg));
            try {
                List<String> lines = java.nio.file.Files.readAllLines(file.toPath());
                int wordCount = 0;
                int charCount = 0;
                
                for (String line : lines) {
                    charCount += line.length() + 1; // +1 for newline
                    wordCount += line.split("\\s+").length;
                }
                
                appendText(String.format("  %d  %d  %d %s\n", 
                    lines.size(), wordCount, charCount, arg), FG_COLOR);
            } catch (IOException e) {
                appendText("wc: " + arg + ": " + e.getMessage() + "\n", ERROR_COLOR);
            }
        }
    }
    
    private void makeDirectory(String[] args) {
        if (args.length == 0) {
            appendText("mkdir: missing operand\n", ERROR_COLOR);
            return;
        }
        
        for (String arg : args) {
            File dir = new File(resolvePath(arg));
            if (dir.exists()) {
                appendText("mkdir: " + arg + ": File exists\n", ERROR_COLOR);
            } else if (dir.mkdir()) {
                appendText("Directory created: " + arg + "\n", INFO_COLOR);
            } else {
                appendText("mkdir: cannot create directory '" + arg + "'\n", ERROR_COLOR);
            }
        }
    }
    
    private void touchFile(String[] args) {
        if (args.length == 0) {
            appendText("touch: missing operand\n", ERROR_COLOR);
            return;
        }
        
        for (String arg : args) {
            File file = new File(resolvePath(arg));
            try {
                if (file.exists()) {
                    file.setLastModified(System.currentTimeMillis());
                } else {
                    file.createNewFile();
                }
            } catch (IOException e) {
                appendText("touch: " + e.getMessage() + "\n", ERROR_COLOR);
            }
        }
    }
    
    private void removeFile(String[] args) {
        if (args.length == 0) {
            appendText("rm: missing operand\n", ERROR_COLOR);
            return;
        }
        
        boolean recursive = false;
        for (String arg : args) {
            if (arg.equals("-r") || arg.equals("-rf")) {
                recursive = true;
                continue;
            }
            
            File file = new File(resolvePath(arg));
            if (!file.exists()) {
                appendText("rm: " + arg + ": No such file or directory\n", ERROR_COLOR);
            } else if (file.isDirectory() && !recursive) {
                appendText("rm: " + arg + ": Is a directory\n", ERROR_COLOR);
            } else {
                // For safety, don't actually delete in this demo
                appendText("rm: would remove '" + arg + "' (disabled for safety)\n", INFO_COLOR);
            }
        }
    }
    
    private void copyFile(String[] args) {
        if (args.length < 2) {
            appendText("cp: missing operand\n", ERROR_COLOR);
            return;
        }
        appendText("cp: copy operation simulated (disabled for safety)\n", INFO_COLOR);
    }
    
    private void moveFile(String[] args) {
        if (args.length < 2) {
            appendText("mv: missing operand\n", ERROR_COLOR);
            return;
        }
        appendText("mv: move operation simulated (disabled for safety)\n", INFO_COLOR);
    }
    
    private void showEnvironment() {
        for (Map.Entry<String, String> entry : environment.entrySet()) {
            appendText(entry.getKey() + "=" + entry.getValue() + "\n", FG_COLOR);
        }
    }
    
    private void setEnvironment(String[] args) {
        for (String arg : args) {
            int eq = arg.indexOf('=');
            if (eq > 0) {
                String key = arg.substring(0, eq);
                String value = arg.substring(eq + 1);
                environment.put(key, value);
            }
        }
    }
    
    private void showHistory() {
        for (int i = 0; i < commandHistory.size(); i++) {
            appendText(String.format("  %3d  %s\n", i + 1, commandHistory.get(i)), FG_COLOR);
        }
    }
    
    private void showSystemInfo(String[] args) {
        boolean all = args.length > 0 && args[0].equals("-a");
        
        if (all) {
            appendText("IRIX64 " + getHostname() + " 6.5 IP32 mips\n", FG_COLOR);
        } else {
            appendText("IRIX64\n", FG_COLOR);
        }
    }
    
    private void showHostname() {
        appendText(getHostname() + "\n", FG_COLOR);
    }
    
    private String getHostname() {
        try {
            return java.net.InetAddress.getLocalHost().getHostName();
        } catch (Exception e) {
            return "localhost";
        }
    }
    
    private void showUptime() {
        long uptimeMs = System.currentTimeMillis() / 1000 / 60;
        appendText("up " + uptimeMs + " minutes\n", FG_COLOR);
    }
    
    private void showDiskSpace() {
        appendText("Filesystem      Size  Used Avail Use% Mounted on\n", INFO_COLOR);
        
        File[] roots = File.listRoots();
        for (File root : roots) {
            long total = root.getTotalSpace() / (1024 * 1024 * 1024);
            long free = root.getFreeSpace() / (1024 * 1024 * 1024);
            long used = total - free;
            int percent = total > 0 ? (int)(used * 100 / total) : 0;
            
            appendText(String.format("%-16s %4dG %4dG %4dG %3d%% %s\n",
                "/dev/dsk/dks0d1s0", total, used, free, percent, root.getPath()), FG_COLOR);
        }
    }
    
    private void showHelp() {
        String help = 
            "Available commands:\n" +
            "  File Operations:\n" +
            "    ls [-l] [-a]     List directory contents\n" +
            "    cd [dir]         Change directory\n" +
            "    pwd              Print working directory\n" +
            "    cat <file>       Display file contents\n" +
            "    head [-n N] file Show first N lines\n" +
            "    tail [-n N] file Show last N lines\n" +
            "    wc <file>        Word/line/char count\n" +
            "    mkdir <dir>      Create directory\n" +
            "    touch <file>     Create/update file\n" +
            "    rm [-r] <file>   Remove file (disabled)\n" +
            "    cp src dest      Copy file (disabled)\n" +
            "    mv src dest      Move file (disabled)\n" +
            "\n" +
            "  System Information:\n" +
            "    date             Show current date/time\n" +
            "    whoami           Show current user\n" +
            "    hostname         Show hostname\n" +
            "    uname [-a]       Show system info\n" +
            "    uptime           Show system uptime\n" +
            "    df               Show disk space\n" +
            "\n" +
            "  Environment:\n" +
            "    env              Show environment vars\n" +
            "    export VAR=val   Set environment var\n" +
            "    echo [args]      Print arguments\n" +
            "\n" +
            "  Other:\n" +
            "    clear            Clear terminal (Ctrl+L)\n" +
            "    history          Show command history\n" +
            "    help             Show this help\n" +
            "    man <cmd>        Show command manual\n" +
            "\n" +
            "  Keyboard Shortcuts:\n" +
            "    Up/Down          Navigate history\n" +
            "    Tab              Auto-complete\n" +
            "    Ctrl+C           Cancel input\n" +
            "    Ctrl+L           Clear screen\n";
        
        appendText(help, FG_COLOR);
    }
    
    private void showManPage(String[] args) {
        if (args.length == 0) {
            appendText("What manual page do you want?\n", ERROR_COLOR);
            return;
        }
        
        String cmd = args[0];
        appendText("Manual page for: " + cmd + "\n\n", INFO_COLOR);
        appendText("No manual entry for " + cmd + "\n", FG_COLOR);
        appendText("(Manual pages not implemented in this demo)\n", INFO_COLOR);
    }
    
    private void navigateHistory(int direction) {
        if (commandHistory.isEmpty()) return;
        
        historyIndex += direction;
        if (historyIndex < 0) historyIndex = 0;
        if (historyIndex >= commandHistory.size()) {
            historyIndex = commandHistory.size();
            input.setText("");
            return;
        }
        
        input.setText(commandHistory.get(historyIndex));
        input.setCaretPosition(input.getText().length());
    }
    
    private void autoComplete() {
        String text = input.getText();
        String[] parts = text.split("\\s+");
        String toComplete = parts.length > 0 ? parts[parts.length - 1] : "";
        
        // Determine if completing command or file
        if (parts.length <= 1 && !text.contains(" ")) {
            // Command completion
            String[] commands = {"ls", "cd", "pwd", "cat", "head", "tail", "wc",
                "mkdir", "touch", "rm", "cp", "mv", "date", "whoami", "hostname",
                "uname", "uptime", "df", "env", "export", "echo", "clear",
                "history", "help", "man", "exit"};
            
            for (String cmd : commands) {
                if (cmd.startsWith(toComplete)) {
                    input.setText(cmd + " ");
                    return;
                }
            }
        } else {
            // File completion
            File dir = new File(currentDirectory);
            File[] files = dir.listFiles();
            
            if (files != null) {
                for (File file : files) {
                    if (file.getName().startsWith(toComplete)) {
                        String completion = file.getName();
                        if (file.isDirectory()) completion += "/";
                        
                        // Replace last part with completion
                        StringBuilder sb = new StringBuilder();
                        for (int i = 0; i < parts.length - 1; i++) {
                            sb.append(parts[i]).append(" ");
                        }
                        sb.append(completion);
                        input.setText(sb.toString());
                        return;
                    }
                }
            }
        }
    }
}
