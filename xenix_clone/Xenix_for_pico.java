import java.util.*;
import java.io.*;
import java.nio.file.*;
import java.text.SimpleDateFormat;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import com.fazecast.jSerialComm.*;

public class XenixOS {
    private FileSystem fs;
    private String currentUser;
    private String hostname;
    private Scanner scanner;
    private boolean running;
    private static final String SAVE_FILE = "xenix_state.dat";
    private Map<String, MountPoint> mountPoints;
    private SerialTerminal serialTerminal;
    private boolean useSerial;
    
    public XenixOS(String serialPort, int baudRate) {
        this.currentUser = "root";
        this.hostname = "xenix";
        this.running = true;
        this.mountPoints = new HashMap<>();
        this.useSerial = false;
        
        // Initialize serial if port specified
        if (serialPort != null && !serialPort.isEmpty()) {
            try {
                this.serialTerminal = new SerialTerminal(serialPort, baudRate);
                this.useSerial = true;
                this.scanner = null;
            } catch (Exception e) {
                System.out.println("Failed to open serial port: " + e.getMessage());
                System.out.println("Falling back to console mode");
                this.scanner = new Scanner(System.in);
            }
        } else {
            this.scanner = new Scanner(System.in);
        }
        
        // Load or create filesystem
        this.fs = loadFileSystem();
    }
    
    public XenixOS() {
        this(null, 9600);
    }
    
    private FileSystem loadFileSystem() {
        File saveFile = new File(SAVE_FILE);
        if (saveFile.exists()) {
            try (ObjectInputStream ois = new ObjectInputStream(new FileInputStream(saveFile))) {
                System.out.println("Loading saved filesystem...");
                return (FileSystem) ois.readObject();
            } catch (Exception e) {
                System.out.println("Error loading filesystem: " + e.getMessage());
                System.out.println("Creating new filesystem...");
            }
        }
        return new FileSystem();
    }
    
    private void saveFileSystem() {
        try (ObjectOutputStream oos = new ObjectOutputStream(new FileOutputStream(SAVE_FILE))) {
            oos.writeObject(fs);
            System.out.println("Filesystem saved.");
        } catch (Exception e) {
            System.out.println("Error saving filesystem: " + e.getMessage());
        }
    }
    
    public void boot() {
        println("Xenix System V/386");
        println("Copyright (c) 1984-1989 Microsoft Corporation");
        println("All rights reserved.\n");
        
        if (useSerial) {
            println("Serial terminal connected at " + serialTerminal.getBaudRate() + " baud");
            // Send VT100 initialization
            print("\033[2J"); // Clear screen
            print("\033[H");  // Home cursor
        }
        
        // Display MOTD
        String motd = fs.readFile("/etc/motd");
        if (motd != null && !motd.isEmpty()) {
            println(motd);
        }
        
        println("\nLogin: " + currentUser);
        println("Welcome to Xenix\n");
        
        while (running) {
            print(currentUser + "@" + hostname + ":" + fs.getCurrentPath() + "$ ");
            String input = readLine();
            if (input != null && !input.isEmpty()) {
                executeCommand(input);
            }
        }
    }
    
    private void print(String text) {
        if (useSerial && serialTerminal != null) {
            serialTerminal.write(text);
        } else {
            System.out.print(text);
        }
    }
    
    private void println(String text) {
        print(text + "\r\n");
    }
    
    private String readLine() {
        if (useSerial && serialTerminal != null) {
            return serialTerminal.readLine();
        } else if (scanner != null) {
            return scanner.nextLine();
        }
        return "";
    }
    
    private void executeCommand(String input) {
        input = input.trim();
        String[] parts = input.split("\\s+");
        String cmd = parts[0];
        String[] args = Arrays.copyOfRange(parts, 1, parts.length);
        
        try {
            switch (cmd) {
                case "ls": ls(args); break;
                case "cd": cd(args); break;
                case "pwd": pwd(); break;
                case "mkdir": mkdir(args); break;
                case "rmdir": rmdir(args); break;
                case "cat": cat(args); break;
                case "echo": echo(args); break;
                case "touch": touch(args); break;
                case "rm": rm(args); break;
                case "cp": cp(args); break;
                case "mv": mv(args); break;
                case "find": find(args); break;
                case "grep": grep(args); break;
                case "wc": wc(args); break;
                case "ps": ps(); break;
                case "who": who(); break;
                case "date": date(); break;
                case "clear": clear(); break;
                case "help": help(); break;
                case "exit": exit(); break;
                case "uname": uname(); break;
                case "df": df(); break;
                case "du": du(args); break;
                case "chmod": chmod(args); break;
                case "chown": chown(args); break;
                case "vi": vi(args); break;
                case "ed": ed(args); break;
                case "mount": mount(args); break;
                case "umount": umount(args); break;
                case "stty": stty(args); break;
                case "serial": serial(args); break;
                default:
                    println(cmd + ": command not found");
            }
        } catch (Exception e) {
            println("Error: " + e.getMessage());
        }
    }
    
    private void ls(String[] args) {
        List<FileNode> files = fs.listFiles();
        boolean longFormat = args.length > 0 && args[0].equals("-l");
        
        if (longFormat) {
            for (FileNode f : files) {
                println(String.format("%s %s %s %8d %s %s",
                    f.permissions, f.owner, f.group, f.size,
                    f.modified, f.name));
            }
        } else {
            StringBuilder line = new StringBuilder();
            for (FileNode f : files) {
                line.append(f.name).append("  ");
            }
            if (!files.isEmpty()) println(line.toString());
        }
    }
    
    private void cd(String[] args) {
        if (args.length == 0) {
            fs.changeDirectory("/home/" + currentUser);
        } else {
            fs.changeDirectory(args[0]);
        }
    }
    
    private void pwd() {
        println(fs.getCurrentPath());
    }
    
    private void mkdir(String[] args) {
        if (args.length == 0) {
            println("mkdir: missing operand");
            return;
        }
        fs.createDirectory(args[0]);
    }
    
    private void rmdir(String[] args) {
        if (args.length == 0) {
            println("rmdir: missing operand");
            return;
        }
        fs.removeDirectory(args[0]);
    }
    
    private void cat(String[] args) {
        if (args.length == 0) {
            println("cat: missing operand");
            return;
        }
        String content = fs.readFile(args[0]);
        if (content != null) {
            println(content);
        }
    }
    
    private void echo(String[] args) {
        boolean redirect = false;
        String filename = null;
        List<String> words = new ArrayList<>();
        
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals(">") && i + 1 < args.length) {
                redirect = true;
                filename = args[i + 1];
                break;
            }
            words.add(args[i]);
        }
        
        String output = String.join(" ", words);
        
        if (redirect && filename != null) {
            fs.writeFile(filename, output);
        } else {
            println(output);
        }
    }
    
    private void touch(String[] args) {
        if (args.length == 0) {
            println("touch: missing operand");
            return;
        }
        fs.createFile(args[0]);
    }
    
    private void rm(String[] args) {
        if (args.length == 0) {
            println("rm: missing operand");
            return;
        }
        fs.removeFile(args[0]);
    }
    
    private void cp(String[] args) {
        if (args.length < 2) {
            println("cp: missing operand");
            return;
        }
        fs.copyFile(args[0], args[1]);
    }
    
    private void mv(String[] args) {
        if (args.length < 2) {
            println("mv: missing operand");
            return;
        }
        fs.moveFile(args[0], args[1]);
    }
    
    private void find(String[] args) {
        String pattern = args.length > 0 ? args[0] : "*";
        List<String> results = fs.find(pattern);
        for (String path : results) {
            println(path);
        }
    }
    
    private void grep(String[] args) {
        if (args.length < 2) {
            println("grep: missing operand");
            return;
        }
        String pattern = args[0];
        String filename = args[1];
        String content = fs.readFile(filename);
        if (content != null) {
            String[] lines = content.split("\n");
            for (String line : lines) {
                if (line.contains(pattern)) {
                    println(line);
                }
            }
        }
    }
    
    private void wc(String[] args) {
        if (args.length == 0) {
            println("wc: missing operand");
            return;
        }
        String content = fs.readFile(args[0]);
        if (content != null) {
            String[] lines = content.split("\n");
            int words = 0;
            for (String line : lines) {
                words += line.split("\\s+").length;
            }
            println(String.format("%d %d %d %s", lines.length, words, content.length(), args[0]));
        }
    }
    
    private void ps() {
        println("  PID TTY      TIME CMD");
        println("    1 console  0:00 init");
        println("   42 tty01    0:01 sh");
        println("  156 tty01    0:00 xenix");
    }
    
    private void who() {
        println(currentUser + "     tty01    " + new Date());
    }
    
    private void date() {
        SimpleDateFormat sdf = new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy");
        println(sdf.format(new Date()));
    }
    
    private void clear() {
        if (useSerial) {
            print("\033[2J"); // VT100 clear screen
            print("\033[H");  // VT100 home cursor
        } else {
            System.out.print("\033[H\033[2J");
            System.out.flush();
        }
    }
    
    private void uname() {
        println("Xenix xenix 3.0 i386");
    }
    
    private void df() {
        println("Filesystem     1K-blocks  Used Available Use% Mounted on");
        println("/dev/hd0a        1048576 52428   996148   5% /");
        
        for (Map.Entry<String, MountPoint> entry : mountPoints.entrySet()) {
            println(String.format("%-15s %8d %5d %9d  %2d%% %s",
                entry.getValue().device, 
                entry.getValue().totalBlocks,
                entry.getValue().usedBlocks,
                entry.getValue().totalBlocks - entry.getValue().usedBlocks,
                (entry.getValue().usedBlocks * 100) / entry.getValue().totalBlocks,
                entry.getKey()));
        }
    }
    
    private void du(String[] args) {
        String path = args.length > 0 ? args[0] : ".";
        println("4\t" + path);
    }
    
    private void chmod(String[] args) {
        if (args.length < 2) {
            println("chmod: missing operand");
            return;
        }
        fs.chmod(args[1], args[0]);
    }
    
    private void chown(String[] args) {
        if (args.length < 2) {
            println("chown: missing operand");
            return;
        }
        fs.chown(args[1], args[0]);
    }
    
    private void stty(String[] args) {
        if (args.length == 0) {
            if (useSerial && serialTerminal != null) {
                println("speed " + serialTerminal.getBaudRate() + " baud;");
                println("line = 0;");
                println("intr = ^C; quit = ^\\; erase = ^H; kill = ^U;");
            } else {
                println("Not a serial terminal");
            }
        } else {
            println("stty: parameter changes not supported");
        }
    }
    
    private void serial(String[] args) {
        if (args.length == 0) {
            println("Available serial ports:");
            SerialPort[] ports = SerialPort.getCommPorts();
            for (SerialPort port : ports) {
                println("  " + port.getSystemPortName() + " - " + port.getDescriptivePortName());
            }
            if (useSerial && serialTerminal != null) {
                println("\nCurrently connected: " + serialTerminal.getPortName() + " at " + serialTerminal.getBaudRate() + " baud");
            }
        } else if (args[0].equals("disconnect")) {
            if (useSerial && serialTerminal != null) {
                serialTerminal.close();
                println("Serial port disconnected");
                useSerial = false;
                scanner = new Scanner(System.in);
            }
        } else {
            println("Usage: serial [disconnect]");
        }
    }
    
    private void vi(String[] args) {
        if (args.length == 0) {
            println("vi: missing filename");
            return;
        }
        
        String filename = args[0];
        String content = fs.readFile(filename);
        if (content == null) content = "";
        
        List<String> lines = new ArrayList<>(Arrays.asList(content.split("\n", -1)));
        if (lines.isEmpty()) lines.add("");
        
        println("\"" + filename + "\" " + lines.size() + " lines");
        println("Simple VI Editor - Commands: :w (save), :q (quit), :wq (save and quit), i (insert mode), ESC (command mode)");
        
        boolean editing = true;
        boolean insertMode = false;
        int currentLine = 0;
        
        while (editing) {
            if (!insertMode) {
                println("\n--- Line " + (currentLine + 1) + " of " + lines.size() + " ---");
                if (currentLine < lines.size()) {
                    println(lines.get(currentLine));
                }
                print(":");
            }
            
            String input = readLine();
            
            if (insertMode) {
                if (input.equals("ESC") || input.equals("esc")) {
                    insertMode = false;
                    println("-- COMMAND MODE --");
                } else {
                    if (currentLine >= lines.size()) {
                        lines.add(input);
                    } else {
                        lines.set(currentLine, input);
                    }
                    currentLine++;
                    if (currentLine >= lines.size()) {
                        lines.add("");
                    }
                }
            } else {
                switch (input.trim()) {
                    case "i":
                        insertMode = true;
                        println("-- INSERT MODE -- (type ESC to return to command mode)");
                        break;
                    case "w":
                    case ":w":
                        fs.writeFile(filename, String.join("\n", lines));
                        println("\"" + filename + "\" " + lines.size() + " lines written");
                        break;
                    case "q":
                    case ":q":
                        editing = false;
                        break;
                    case "wq":
                    case ":wq":
                        fs.writeFile(filename, String.join("\n", lines));
                        println("\"" + filename + "\" " + lines.size() + " lines written");
                        editing = false;
                        break;
                    case "n":
                        if (currentLine < lines.size() - 1) currentLine++;
                        break;
                    case "p":
                        if (currentLine > 0) currentLine--;
                        break;
                    default:
                        println("Unknown command. Use :w, :q, :wq, i, n (next), p (previous)");
                }
            }
        }
    }
    
    private void ed(String[] args) {
        if (args.length == 0) {
            println("ed: missing filename");
            return;
        }
        
        String filename = args[0];
        String content = fs.readFile(filename);
        if (content == null) content = "";
        
        println("ED line editor. Commands: a (append), p (print), w (write), q (quit)");
        
        StringBuilder buffer = new StringBuilder(content);
        boolean editing = true;
        
        while (editing) {
            print("*");
            String cmd = readLine();
            
            switch (cmd.trim()) {
                case "a":
                    println("Enter text (type . on a line by itself to finish):");
                    while (true) {
                        String line = readLine();
                        if (line.equals(".")) break;
                        buffer.append(line).append("\n");
                    }
                    break;
                case "p":
                    println(buffer.toString());
                    break;
                case "w":
                    fs.writeFile(filename, buffer.toString());
                    println(buffer.length() + " bytes written");
                    break;
                case "q":
                    editing = false;
                    break;
                default:
                    println("?");
            }
        }
    } quit), i (insert mode), ESC (command mode)");
        
        boolean editing = true;
        boolean insertMode = false;
        int currentLine = 0;
        
        while (editing) {
            if (!insertMode) {
                System.out.println("\n--- Line " + (currentLine + 1) + " of " + lines.size() + " ---");
                if (currentLine < lines.size()) {
                    System.out.println(lines.get(currentLine));
                }
                System.out.print(":");
            }
            
            String input = scanner.nextLine();
            
            if (insertMode) {
                if (input.equals("ESC") || input.equals("esc")) {
                    insertMode = false;
                    System.out.println("-- COMMAND MODE --");
                } else {
                    if (currentLine >= lines.size()) {
                        lines.add(input);
                    } else {
                        lines.set(currentLine, input);
                    }
                    currentLine++;
                    if (currentLine >= lines.size()) {
                        lines.add("");
                    }
                }
            } else {
                switch (input.trim()) {
                    case "i":
                        insertMode = true;
                        System.out.println("-- INSERT MODE -- (type ESC to return to command mode)");
                        break;
                    case "w":
                    case ":w":
                        fs.writeFile(filename, String.join("\n", lines));
                        System.out.println("\"" + filename + "\" " + lines.size() + " lines written");
                        break;
                    case "q":
                    case ":q":
                        editing = false;
                        break;
                    case "wq":
                    case ":wq":
                        fs.writeFile(filename, String.join("\n", lines));
                        System.out.println("\"" + filename + "\" " + lines.size() + " lines written");
                        editing = false;
                        break;
                    case "n":
                        if (currentLine < lines.size() - 1) currentLine++;
                        break;
                    case "p":
                        if (currentLine > 0) currentLine--;
                        break;
                    default:
                        System.out.println("Unknown command. Use :w, :q, :wq, i, n (next), p (previous)");
                }
            }
        }
    }
    
    private void ed(String[] args) {
        if (args.length == 0) {
            System.out.println("ed: missing filename");
            return;
        }
        
        String filename = args[0];
        String content = fs.readFile(filename);
        if (content == null) content = "";
        
        System.out.println("ED line editor. Commands: a (append), p (print), w (write), q (quit)");
        
        StringBuilder buffer = new StringBuilder(content);
        boolean editing = true;
        
        while (editing) {
            System.out.print("*");
            String cmd = scanner.nextLine();
            
            switch (cmd.trim()) {
                case "a":
                    System.out.println("Enter text (type . on a line by itself to finish):");
                    while (true) {
                        String line = scanner.nextLine();
                        if (line.equals(".")) break;
                        buffer.append(line).append("\n");
                    }
                    break;
                case "p":
                    System.out.println(buffer.toString());
                    break;
                case "w":
                    fs.writeFile(filename, buffer.toString());
                    System.out.println(buffer.length() + " bytes written");
                    break;
                case "q":
                    editing = false;
                    break;
                default:
                    System.out.println("?");
            }
        }
    }
    
    private void mount(String[] args) {
        if (args.length < 2) {
            println("Usage: mount <device> <mountpoint>");
            println("       mount (show mounted filesystems)");
            if (args.length == 0) {
                for (Map.Entry<String, MountPoint> entry : mountPoints.entrySet()) {
                    println(entry.getValue().device + " on " + entry.getKey() + " type " + entry.getValue().fsType);
                }
            }
            return;
        }
        
        String device = args[0];
        String mountpoint = args[1];
        
        // Check if device file exists
        File deviceFile = new File(device);
        if (!deviceFile.exists()) {
            println("mount: " + device + ": No such file or directory");
            return;
        }
        
        // Create mount point directory if it doesn't exist
        fs.createDirectory(mountpoint);
        
        try {
            FAT32FileSystem fat32 = new FAT32FileSystem(device);
            MountPoint mp = new MountPoint(device, mountpoint, "fat32", fat32);
            mountPoints.put(mountpoint, mp);
            
            // Import FAT32 files into virtual filesystem
            for (FAT32File file : fat32.listFiles()) {
                String virtualPath = mountpoint + "/" + file.name;
                if (file.isDirectory) {
                    fs.createDirectoryAtPath(virtualPath);
                } else {
                    fs.writeFileAtPath(virtualPath, file.content);
                }
            }
            
            println("Mounted " + device + " on " + mountpoint);
        } catch (Exception e) {
            println("mount: " + e.getMessage());
        }
    }
    
    private void umount(String[] args) {
        if (args.length == 0) {
            println("umount: missing operand");
            return;
        }
        
        String mountpoint = args[0];
        if (mountPoints.containsKey(mountpoint)) {
            mountPoints.remove(mountpoint);
            println("Unmounted " + mountpoint);
        } else {
            println("umount: " + mountpoint + ": not mounted");
        }
    }
    
    private void help() {
        println("Available commands:");
        println("File: ls, cd, pwd, mkdir, rmdir, cat, echo, touch, rm, cp, mv, find");
        println("Text: grep, wc, vi, ed");
        println("System: ps, who, date, clear, uname, df, du, chmod, chown");
        println("Mount: mount, umount");
        println("Serial: serial, stty");
        println("Other: help, exit");
    }
    
    private void exit() {
        println("Saving filesystem state...");
        saveFileSystem();
        println("logout");
        if (useSerial && serialTerminal != null) {
            serialTerminal.close();
        }
        running = false;
    }
    
    public static void main(String[] args) {
        String serialPort = null;
        int baudRate = 9600;
        
        // Parse command line arguments
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-serial") && i + 1 < args.length) {
                serialPort = args[i + 1];
                i++;
            } else if (args[i].equals("-baud") && i + 1 < args.length) {
                try {
                    baudRate = Integer.parseInt(args[i + 1]);
                } catch (NumberFormatException e) {
                    System.out.println("Invalid baud rate, using default 9600");
                }
                i++;
            } else if (args[i].equals("-list-ports")) {
                System.out.println("Available serial ports:");
                SerialPort[] ports = SerialPort.getCommPorts();
                for (SerialPort port : ports) {
                    System.out.println("  " + port.getSystemPortName() + " - " + port.getDescriptivePortName());
                }
                return;
            } else if (args[i].equals("-help")) {
                System.out.println("Xenix OS Simulator");
                System.out.println("Usage: java XenixOS [options]");
                System.out.println("Options:");
                System.out.println("  -serial <port>    Connect to serial port (e.g., COM1, /dev/ttyUSB0)");
                System.out.println("  -baud <rate>      Set baud rate (default: 9600)");
                System.out.println("  -list-ports       List available serial ports");
                System.out.println("  -help             Show this help message");
                System.out.println("\nExample:");
                System.out.println("  java XenixOS -serial COM3 -baud 115200");
                return;
            }
        }
        
        XenixOS os = new XenixOS(serialPort, baudRate);
        os.boot();
    }
}

class FileNode implements Serializable {
    private static final long serialVersionUID = 1L;
    String name;
    boolean isDirectory;
    String content;
    String permissions;
    String owner;
    String group;
    int size;
    String modified;
    Map<String, FileNode> children;
    
    public FileNode(String name, boolean isDirectory) {
        this.name = name;
        this.isDirectory = isDirectory;
        this.content = "";
        this.permissions = isDirectory ? "drwxr-xr-x" : "-rw-r--r--";
        this.owner = "root";
        this.group = "root";
        this.size = 0;
        this.modified = new SimpleDateFormat("MMM dd HH:mm").format(new Date());
        this.children = isDirectory ? new HashMap<>() : null;
    }
}

class FileSystem implements Serializable {
    private static final long serialVersionUID = 1L;
    private FileNode root;
    private transient FileNode current;
    private List<String> pathStack;
    
    public FileSystem() {
        root = new FileNode("/", true);
        current = root;
        pathStack = new ArrayList<>();
        pathStack.add("/");
        
        createInitialStructure();
    }
    
    private void createInitialStructure() {
        root.children.put("bin", new FileNode("bin", true));
        root.children.put("etc", new FileNode("etc", true));
        root.children.put("usr", new FileNode("usr", true));
        root.children.put("tmp", new FileNode("tmp", true));
        root.children.put("home", new FileNode("home", true));
        root.children.put("mnt", new FileNode("mnt", true));
        
        FileNode home = root.children.get("home");
        home.children.put("root", new FileNode("root", true));
        
        // Create default MOTD
        FileNode etc = root.children.get("etc");
        FileNode motd = new FileNode("motd", false);
        motd.content = "Welcome to Xenix System V!\n\nLast login: " + new Date() + "\n";
        motd.size = motd.content.length();
        etc.children.put("motd", motd);
    }
    
    private void readObject(ObjectInputStream in) throws IOException, ClassNotFoundException {
        in.defaultReadObject();
        current = navigateToPath(pathStack);
    }
    
    public String getCurrentPath() {
        return String.join("/", pathStack).replaceAll("//", "/");
    }
    
    public List<FileNode> listFiles() {
        return new ArrayList<>(current.children.values());
    }
    
    public void changeDirectory(String path) {
        if (path.equals("..")) {
            if (pathStack.size() > 1) {
                pathStack.remove(pathStack.size() - 1);
                current = navigateToPath(pathStack);
            }
        } else if (path.equals("/")) {
            pathStack.clear();
            pathStack.add("/");
            current = root;
        } else if (path.startsWith("/")) {
            // Absolute path
            String[] parts = path.split("/");
            List<String> newPath = new ArrayList<>();
            newPath.add("/");
            FileNode node = root;
            
            for (int i = 1; i < parts.length; i++) {
                if (parts[i].isEmpty()) continue;
                if (node.children.containsKey(parts[i])) {
                    node = node.children.get(parts[i]);
                    if (node.isDirectory) {
                        newPath.add(parts[i]);
                    } else {
                        System.out.println("cd: " + path + ": Not a directory");
                        return;
                    }
                } else {
                    System.out.println("cd: " + path + ": No such directory");
                    return;
                }
            }
            pathStack = newPath;
            current = node;
        } else {
            FileNode target = current.children.get(path);
            if (target != null && target.isDirectory) {
                pathStack.add(path);
                current = target;
            } else {
                System.out.println("cd: " + path + ": No such directory");
            }
        }
    }
    
    private FileNode navigateToPath(List<String> path) {
        FileNode node = root;
        for (int i = 1; i < path.size(); i++) {
            node = node.children.get(path.get(i));
            if (node == null) return root;
        }
        return node;
    }
    
    public void createDirectory(String name) {
        if (!current.children.containsKey(name)) {
            current.children.put(name, new FileNode(name, true));
        } else {
            System.out.println("mkdir: cannot create directory '" + name + "': File exists");
        }
    }
    
    public void createDirectoryAtPath(String path) {
        String[] parts = path.split("/");
        FileNode node = root;
        
        for (int i = 1; i < parts.length; i++) {
            if (parts[i].isEmpty()) continue;
            if (!node.children.containsKey(parts[i])) {
                node.children.put(parts[i], new FileNode(parts[i], true));
            }
            node = node.children.get(parts[i]);
        }
    }
    
    public void removeDirectory(String name) {
        FileNode node = current.children.get(name);
        if (node != null && node.isDirectory && node.children.isEmpty()) {
            current.children.remove(name);
        } else {
            System.out.println("rmdir: failed to remove '" + name + "'");
        }
    }
    
    public void createFile(String name) {
        if (!current.children.containsKey(name)) {
            current.children.put(name, new FileNode(name, false));
        }
    }
    
    public void writeFile(String name, String content) {
        FileNode file = current.children.get(name);
        if (file == null) {
            file = new FileNode(name, false);
            current.children.put(name, file);
        }
        if (!file.isDirectory) {
            file.content = content;
            file.size = content.length();
            file.modified = new SimpleDateFormat("MMM dd HH:mm").format(new Date());
        }
    }
    
    public void writeFileAtPath(String path, String content) {
        String[] parts = path.split("/");
        FileNode node = root;
        
        for (int i = 1; i < parts.length - 1; i++) {
            if (parts[i].isEmpty()) continue;
            if (!node.children.containsKey(parts[i])) {
                node.children.put(parts[i], new FileNode(parts[i], true));
            }
            node = node.children.get(parts[i]);
        }
        
        String filename = parts[parts.length - 1];
        FileNode file = new FileNode(filename, false);
        file.content = content;
        file.size = content.length();
        node.children.put(filename, file);
    }
    
    public String readFile(String name) {
        if (name.startsWith("/")) {
            // Absolute path
            String[] parts = name.split("/");
            FileNode node = root;
            
            for (int i = 1; i < parts.length; i++) {
                if (parts[i].isEmpty()) continue;
                if (node.children.containsKey(parts[i])) {
                    node = node.children.get(parts[i]);
                } else {
                    System.out.println("cat: " + name + ": No such file");
                    return null;
                }
            }
            
            if (node != null && !node.isDirectory) {
                return node.content;
            }
            System.out.println("cat: " + name + ": Is a directory");
            return null;
        }
        
        FileNode file = current.children.get(name);
        if (file != null && !file.isDirectory) {
            return file.content;
        }
        System.out.println("cat: " + name + ": No such file");
        return null;
    }
    
    public void removeFile(String name) {
        if (current.children.containsKey(name)) {
            current.children.remove(name);
        } else {
            System.out.println("rm: cannot remove '" + name + "': No such file");
        }
    }
    
    public void copyFile(String src, String dest) {
        FileNode source = current.children.get(src);
        if (source != null && !source.isDirectory) {
            FileNode copy = new FileNode(dest, false);
            copy.content = source.content;
            copy.size = source.size;
            current.children.put(dest, copy);
        } else {
            System.out.println("cp: cannot copy '" + src + "'");
        }
    }
    
    public void moveFile(String src, String dest) {
        FileNode source = current.children.get(src);
        if (source != null) {
            current.children.remove(src);
            source.name = dest;
            current.children.put(dest, source);
        } else {
            System.out.println("mv: cannot move '" + src + "'");
        }
    }
    
    public List<String> find(String pattern) {
        List<String> results = new ArrayList<>();
        for (String name : current.children.keySet()) {
            if (pattern.equals("*") || name.contains(pattern)) {
                results.add(getCurrentPath() + "/" + name);
            }
        }
        return results;
    }
    
    public void chmod(String file, String mode) {
        FileNode node = current.children.get(file);
        if (node != null) {
            node.permissions = node.isDirectory ? "drwxr-xr-x" : "-rw-r--r--";
        }
    }
    
    public void chown(String file, String owner) {
        FileNode node = current.children.get(file);
        if (node != null) {
            node.owner = owner;
        }
    }
}

class MountPoint {
    String device;
    String mountpoint;
    String fsType;
    FAT32FileSystem filesystem;
    int totalBlocks;
    int usedBlocks;
    
    public MountPoint(String device, String mountpoint, String fsType, FAT32FileSystem fs) {
        this.device = device;
        this.mountpoint = mountpoint;
        this.fsType = fsType;
        this.filesystem = fs;
        this.totalBlocks = fs.getTotalClusters() * 4;
        this.usedBlocks = fs.getUsedClusters() * 4;
    }
}

class FAT32File {
    String name;
    boolean isDirectory;
    String content;
    int size;
    
    public FAT32File(String name, boolean isDirectory, String content, int size) {
        this.name = name;
        this.isDirectory = isDirectory;
        this.content = content;
        this.size = size;
    }
}

class FAT32FileSystem {
    private String devicePath;
    private List<FAT32File> files;
    private int totalClusters;
    private int usedClusters;
    
    public FAT32FileSystem(String devicePath) throws Exception {
        this.devicePath = devicePath;
        this.files = new ArrayList<>();
        this.totalClusters = 0;
        this.usedClusters = 0;
        
        parseFAT32();
    }
    
    private void parseFAT32() throws Exception {
        File device = new File(devicePath);
        
        try (RandomAccessFile raf = new RandomAccessFile(device, "r")) {
            // Read boot sector
            byte[] bootSector = new byte[512];
            raf.read(bootSector);
            
            // Check for FAT32 signature
            ByteBuffer bb = ByteBuffer.wrap(bootSector);
            bb.order(ByteOrder.LITTLE_ENDIAN);
            
            // Parse BPB (BIOS Parameter Block)
            bb.position(11);
            int bytesPerSector = bb.getShort() & 0xFFFF;
            int sectorsPerCluster = bb.get() & 0xFF;
            bb.position(32);
            int totalSectors = bb.getInt();
            bb.position(36);
            int sectorsPerFAT = bb.getInt();
            bb.position(44);
            int rootDirCluster = bb.getInt();
            
            // Calculate filesystem parameters
            totalClusters = totalSectors / sectorsPerCluster;
            
            // Read root directory entries
            long rootDirOffset = (rootDirCluster - 2) * sectorsPerCluster * bytesPerSector;
            raf.seek(rootDirOffset);
            
            // Read directory entries
            byte[] dirEntry = new byte[32];
            while (raf.read(dirEntry) == 32) {
                // Check if entry is valid
                if (dirEntry[0] == 0) break; // No more entries
                if (dirEntry[0] == (byte)0xE5) continue; // Deleted entry
                if ((dirEntry[11] & 0x0F) == 0x0F) continue; // Long filename entry
                
                // Extract filename
                StringBuilder name = new StringBuilder();
                for (int i = 0; i < 8; i++) {
                    if (dirEntry[i] != ' ') name.append((char)dirEntry[i]);
                }
                if (dirEntry[8] != ' ') {
                    name.append('.');
                    for (int i = 8; i < 11; i++) {
                        if (dirEntry[i] != ' ') name.append((char)dirEntry[i]);
                    }
                }
                
                // Check attributes
                boolean isDir = (dirEntry[11] & 0x10) != 0;
                
                // Get file size
                ByteBuffer sizeBuf = ByteBuffer.wrap(dirEntry, 28, 4);
                sizeBuf.order(ByteOrder.LITTLE_ENDIAN);
                int fileSize = sizeBuf.getInt();
                
                // Read file content (simplified - just read a small amount)
                String content = "";
                if (!isDir && fileSize > 0 && fileSize < 10000) {
                    byte[] fileData = new byte[Math.min(fileSize, 1024)];
                    long currentPos = raf.getFilePointer();
                    // In real FAT32, we'd follow the cluster chain
                    // This is a simplified version
                    raf.read(fileData);
                    content = new String(fileData, 0, Math.min(fileSize, 1024));
                    raf.seek(currentPos);
                }
                
                files.add(new FAT32File(name.toString().toLowerCase(), isDir, content, fileSize));
                usedClusters++;
            }
            
        } catch (Exception e) {
            throw new Exception("Failed to parse FAT32 filesystem: " + e.getMessage());
        }
    }
    
    public List<FAT32File> listFiles() {
        return files;
    }
    
    public int getTotalClusters() {
        return totalClusters > 0 ? totalClusters : 32768;
    }
    
    public int getUsedClusters() {
        return usedClusters;
    }
}

class SerialTerminal {
    private SerialPort serialPort;
    private InputStream inputStream;
    private OutputStream outputStream;
    private StringBuilder lineBuffer;
    private Thread readerThread;
    private volatile boolean running;
    private Queue<Character> inputQueue;
    
    public SerialTerminal(String portName, int baudRate) throws Exception {
        // Find and open the serial port
        SerialPort[] ports = SerialPort.getCommPorts();
        for (SerialPort port : ports) {
            if (port.getSystemPortName().equals(portName)) {
                serialPort = port;
                break;
            }
        }
        
        if (serialPort == null) {
            throw new Exception("Serial port " + portName + " not found");
        }
        
        // Configure port for Raspberry Pi Pico
        // Standard settings: 8 data bits, 1 stop bit, no parity
        serialPort.setComPortParameters(baudRate, 8, SerialPort.ONE_STOP_BIT, SerialPort.NO_PARITY);
        
        // Set flow control - hardware flow control for reliable communication
        serialPort.setFlowControl(SerialPort.FLOW_CONTROL_RTS_ENABLED | SerialPort.FLOW_CONTROL_CTS_ENABLED);
        
        // Timeouts optimized for Pico's USB CDC serial
        serialPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 100, 0);
        
        if (!serialPort.openPort()) {
            throw new Exception("Failed to open serial port " + portName);
        }
        
        // Small delay for Pico to stabilize after port open
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            // Ignore
        }
        
        inputStream = serialPort.getInputStream();
        outputStream = serialPort.getOutputStream();
        lineBuffer = new StringBuilder();
        inputQueue = new java.util.concurrent.ConcurrentLinkedQueue<>();
        running = true;
        
        // Start background reader thread for non-blocking I/O
        readerThread = new Thread(this::readInputThread);
        readerThread.setDaemon(true);
        readerThread.start();
        
        // Send VT100 initialization sequence
        write("\033[2J"); // Clear screen
        write("\033[H");  // Home cursor
        write("\033[?7h"); // Enable line wrap
        write("\033[?25h"); // Show cursor
        
        // Flush any stale data from Pico
        try {
            Thread.sleep(50);
            while (inputStream.available() > 0) {
                inputStream.read();
            }
        } catch (Exception e) {
            // Ignore
        }
    }
    
    private void readInputThread() {
        byte[] buffer = new byte[64]; // Small buffer for efficiency
        
        while (running) {
            try {
                int available = inputStream.available();
                if (available > 0) {
                    int bytesToRead = Math.min(available, buffer.length);
                    int bytesRead = inputStream.read(buffer, 0, bytesToRead);
                    
                    if (bytesRead > 0) {
                        for (int i = 0; i < bytesRead; i++) {
                            inputQueue.offer((char) (buffer[i] & 0xFF));
                        }
                    }
                } else {
                    Thread.sleep(10); // Small sleep to prevent busy-waiting
                }
            } catch (Exception e) {
                if (running) {
                    System.err.println("Error reading from serial: " + e.getMessage());
                }
                break;
            }
        }
    }
    
    public void write(String text) {
        try {
            // Convert LF to CRLF for proper terminal display
            text = text.replace("\n", "\r\n");
            
            // Write in small chunks for Pico compatibility
            byte[] bytes = text.getBytes();
            int chunkSize = 32; // Small chunks to avoid overwhelming Pico's buffer
            
            for (int i = 0; i < bytes.length; i += chunkSize) {
                int len = Math.min(chunkSize, bytes.length - i);
                outputStream.write(bytes, i, len);
                outputStream.flush();
                
                // Small delay between chunks for Pico to process
                if (len == chunkSize && i + chunkSize < bytes.length) {
                    try {
                        Thread.sleep(5);
                    } catch (InterruptedException e) {
                        break;
                    }
                }
            }
        } catch (IOException e) {
            System.err.println("Error writing to serial port: " + e.getMessage());
        }
    }
    
    public String readLine() {
        lineBuffer.setLength(0);
        
        while (true) {
            // Check for input from queue
            Character c = inputQueue.poll();
            
            if (c == null) {
                try {
                    Thread.sleep(10);
                } catch (InterruptedException e) {
                    break;
                }
                continue;
            }
            
            // Handle different control characters
            if (c == '\r' || c == '\n') {
                if (lineBuffer.length() > 0) {
                    write("\r\n"); // Echo newline
                    String result = lineBuffer.toString();
                    lineBuffer.setLength(0);
                    return result;
                }
            } else if (c == 127 || c == 8) { // Backspace or DEL
                if (lineBuffer.length() > 0) {
                    lineBuffer.setLength(lineBuffer.length() - 1);
                    write("\b \b"); // Erase character on screen
                }
            } else if (c == 3) { // Ctrl+C
                write("^C\r\n");
                lineBuffer.setLength(0);
                return "";
            } else if (c == 4) { // Ctrl+D (EOF for Pico)
                if (lineBuffer.length() == 0) {
                    write("logout\r\n");
                    return "exit";
                }
            } else if (c == 21) { // Ctrl+U (kill line)
                int len = lineBuffer.length();
                for (int i = 0; i < len; i++) {
                    write("\b \b");
                }
                lineBuffer.setLength(0);
            } else if (c >= 32 && c < 127) { // Printable ASCII characters
                lineBuffer.append(c);
                write(String.valueOf(c)); // Echo character
            }
            // Ignore other control characters
        }
    }
    
    public void close() {
        running = false;
        
        if (readerThread != null) {
            try {
                readerThread.join(1000);
            } catch (InterruptedException e) {
                // Ignore
            }
        }
        
        try {
            if (inputStream != null) inputStream.close();
            if (outputStream != null) outputStream.close();
            if (serialPort != null) serialPort.closePort();
        } catch (IOException e) {
            System.err.println("Error closing serial port: " + e.getMessage());
        }
    }
    
    public String getPortName() {
        return serialPort != null ? serialPort.getSystemPortName() : "unknown";
    }
    
    public int getBaudRate() {
        return serialPort != null ? serialPort.getBaudRate() : 0;
    }
}
