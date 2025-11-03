import java.util.*;
import java.io.*;
import java.nio.file.*;
import java.text.SimpleDateFormat;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class XenixOS {
    private FileSystem fs;
    private String currentUser;
    private String hostname;
    private Scanner scanner;
    private boolean running;
    private static final String SAVE_FILE = "xenix_state.dat";
    private Map<String, MountPoint> mountPoints;
    
    public XenixOS() {
        this.currentUser = "root";
        this.hostname = "xenix";
        this.scanner = new Scanner(System.in);
        this.running = true;
        this.mountPoints = new HashMap<>();
        
        // Load or create filesystem
        this.fs = loadFileSystem();
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
        System.out.println("Xenix System V/386");
        System.out.println("Copyright (c) 1984-1989 Microsoft Corporation");
        System.out.println("All rights reserved.\n");
        
        // Display MOTD
        String motd = fs.readFile("/etc/motd");
        if (motd != null && !motd.isEmpty()) {
            System.out.println(motd);
        }
        
        System.out.println("\nLogin: " + currentUser);
        System.out.println("Welcome to Xenix\n");
        
        while (running) {
            System.out.print(currentUser + "@" + hostname + ":" + fs.getCurrentPath() + "$ ");
            String input = scanner.nextLine().trim();
            if (!input.isEmpty()) {
                executeCommand(input);
            }
        }
    }
    
    private void executeCommand(String input) {
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
                default:
                    System.out.println(cmd + ": command not found");
            }
        } catch (Exception e) {
            System.out.println("Error: " + e.getMessage());
        }
    }
    
    private void ls(String[] args) {
        List<FileNode> files = fs.listFiles();
        boolean longFormat = args.length > 0 && args[0].equals("-l");
        
        if (longFormat) {
            for (FileNode f : files) {
                System.out.printf("%s %s %s %8d %s %s\n",
                    f.permissions, f.owner, f.group, f.size,
                    f.modified, f.name);
            }
        } else {
            for (FileNode f : files) {
                System.out.print(f.name + "  ");
            }
            if (!files.isEmpty()) System.out.println();
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
        System.out.println(fs.getCurrentPath());
    }
    
    private void mkdir(String[] args) {
        if (args.length == 0) {
            System.out.println("mkdir: missing operand");
            return;
        }
        fs.createDirectory(args[0]);
    }
    
    private void rmdir(String[] args) {
        if (args.length == 0) {
            System.out.println("rmdir: missing operand");
            return;
        }
        fs.removeDirectory(args[0]);
    }
    
    private void cat(String[] args) {
        if (args.length == 0) {
            System.out.println("cat: missing operand");
            return;
        }
        String content = fs.readFile(args[0]);
        if (content != null) {
            System.out.println(content);
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
            System.out.println(output);
        }
    }
    
    private void touch(String[] args) {
        if (args.length == 0) {
            System.out.println("touch: missing operand");
            return;
        }
        fs.createFile(args[0]);
    }
    
    private void rm(String[] args) {
        if (args.length == 0) {
            System.out.println("rm: missing operand");
            return;
        }
        fs.removeFile(args[0]);
    }
    
    private void cp(String[] args) {
        if (args.length < 2) {
            System.out.println("cp: missing operand");
            return;
        }
        fs.copyFile(args[0], args[1]);
    }
    
    private void mv(String[] args) {
        if (args.length < 2) {
            System.out.println("mv: missing operand");
            return;
        }
        fs.moveFile(args[0], args[1]);
    }
    
    private void find(String[] args) {
        String pattern = args.length > 0 ? args[0] : "*";
        List<String> results = fs.find(pattern);
        for (String path : results) {
            System.out.println(path);
        }
    }
    
    private void grep(String[] args) {
        if (args.length < 2) {
            System.out.println("grep: missing operand");
            return;
        }
        String pattern = args[0];
        String filename = args[1];
        String content = fs.readFile(filename);
        if (content != null) {
            String[] lines = content.split("\n");
            for (String line : lines) {
                if (line.contains(pattern)) {
                    System.out.println(line);
                }
            }
        }
    }
    
    private void wc(String[] args) {
        if (args.length == 0) {
            System.out.println("wc: missing operand");
            return;
        }
        String content = fs.readFile(args[0]);
        if (content != null) {
            String[] lines = content.split("\n");
            int words = 0;
            for (String line : lines) {
                words += line.split("\\s+").length;
            }
            System.out.printf("%d %d %d %s\n", lines.length, words, content.length(), args[0]);
        }
    }
    
    private void ps() {
        System.out.println("  PID TTY      TIME CMD");
        System.out.println("    1 console  0:00 init");
        System.out.println("   42 tty01    0:01 sh");
        System.out.println("  156 tty01    0:00 xenix");
    }
    
    private void who() {
        System.out.println(currentUser + "     tty01    " + new Date());
    }
    
    private void date() {
        SimpleDateFormat sdf = new SimpleDateFormat("EEE MMM dd HH:mm:ss zzz yyyy");
        System.out.println(sdf.format(new Date()));
    }
    
    private void clear() {
        System.out.print("\033[H\033[2J");
        System.out.flush();
    }
    
    private void uname() {
        System.out.println("Xenix xenix 3.0 i386");
    }
    
    private void df() {
        System.out.println("Filesystem     1K-blocks  Used Available Use% Mounted on");
        System.out.println("/dev/hd0a        1048576 52428   996148   5% /");
        
        for (Map.Entry<String, MountPoint> entry : mountPoints.entrySet()) {
            System.out.printf("%-15s %8d %5d %9d  %2d%% %s\n",
                entry.getValue().device, 
                entry.getValue().totalBlocks,
                entry.getValue().usedBlocks,
                entry.getValue().totalBlocks - entry.getValue().usedBlocks,
                (entry.getValue().usedBlocks * 100) / entry.getValue().totalBlocks,
                entry.getKey());
        }
    }
    
    private void du(String[] args) {
        String path = args.length > 0 ? args[0] : ".";
        System.out.println("4\t" + path);
    }
    
    private void chmod(String[] args) {
        if (args.length < 2) {
            System.out.println("chmod: missing operand");
            return;
        }
        fs.chmod(args[1], args[0]);
    }
    
    private void chown(String[] args) {
        if (args.length < 2) {
            System.out.println("chown: missing operand");
            return;
        }
        fs.chown(args[1], args[0]);
    }
    
    private void vi(String[] args) {
        if (args.length == 0) {
            System.out.println("vi: missing filename");
            return;
        }
        
        String filename = args[0];
        String content = fs.readFile(filename);
        if (content == null) content = "";
        
        List<String> lines = new ArrayList<>(Arrays.asList(content.split("\n", -1)));
        if (lines.isEmpty()) lines.add("");
        
        System.out.println("\"" + filename + "\" " + lines.size() + " lines");
        System.out.println("Simple VI Editor - Commands: :w (save), :q (quit), :wq (save & quit), i (insert mode), ESC (command mode)");
        
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
            System.out.println("Usage: mount <device> <mountpoint>");
            System.out.println("       mount (show mounted filesystems)");
            if (args.length == 0) {
                for (Map.Entry<String, MountPoint> entry : mountPoints.entrySet()) {
                    System.out.println(entry.getValue().device + " on " + entry.getKey() + " type " + entry.getValue().fsType);
                }
            }
            return;
        }
        
        String device = args[0];
        String mountpoint = args[1];
        
        // Check if device file exists
        File deviceFile = new File(device);
        if (!deviceFile.exists()) {
            System.out.println("mount: " + device + ": No such file or directory");
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
            
            System.out.println("Mounted " + device + " on " + mountpoint);
        } catch (Exception e) {
            System.out.println("mount: " + e.getMessage());
        }
    }
    
    private void umount(String[] args) {
        if (args.length == 0) {
            System.out.println("umount: missing operand");
            return;
        }
        
        String mountpoint = args[0];
        if (mountPoints.containsKey(mountpoint)) {
            mountPoints.remove(mountpoint);
            System.out.println("Unmounted " + mountpoint);
        } else {
            System.out.println("umount: " + mountpoint + ": not mounted");
        }
    }
    
    private void help() {
        System.out.println("Available commands:");
        System.out.println("File: ls, cd, pwd, mkdir, rmdir, cat, echo, touch, rm, cp, mv, find");
        System.out.println("Text: grep, wc, vi, ed");
        System.out.println("System: ps, who, date, clear, uname, df, du, chmod, chown");
        System.out.println("Mount: mount, umount");
        System.out.println("Other: help, exit");
    }
    
    private void exit() {
        System.out.println("Saving filesystem state...");
        saveFileSystem();
        System.out.println("logout");
        running = false;
    }
    
    public static void main(String[] args) {
        XenixOS os = new XenixOS();
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
