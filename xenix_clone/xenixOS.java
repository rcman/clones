import java.util.*;
import java.io.*;
import java.nio.file.*;
import java.text.SimpleDateFormat;

public class XenixOS {
    private FileSystem fs;
    private String currentUser;
    private String hostname;
    private Scanner scanner;
    private boolean running;
    
    public XenixOS() {
        this.fs = new FileSystem();
        this.currentUser = "root";
        this.hostname = "xenix";
        this.scanner = new Scanner(System.in);
        this.running = true;
    }
    
    public void boot() {
        System.out.println("Xenix System V/386");
        System.out.println("Copyright (c) 1984-1989 Microsoft Corporation");
        System.out.println("All rights reserved.\n");
        System.out.println("Login: " + currentUser);
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
        System.out.println("/dev/hd0b         524288 10485   513803   2% /usr");
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
    
    private void help() {
        System.out.println("Available commands:");
        System.out.println("ls, cd, pwd, mkdir, rmdir, cat, echo, touch, rm, cp, mv");
        System.out.println("find, grep, wc, ps, who, date, clear, uname, df, du");
        System.out.println("chmod, chown, help, exit");
    }
    
    private void exit() {
        System.out.println("logout");
        running = false;
    }
    
    public static void main(String[] args) {
        XenixOS os = new XenixOS();
        os.boot();
    }
}

class FileNode {
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

class FileSystem {
    private FileNode root;
    private FileNode current;
    private List<String> pathStack;
    
    public FileSystem() {
        root = new FileNode("/", true);
        current = root;
        pathStack = new ArrayList<>();
        pathStack.add("/");
        
        // Create initial directory structure
        createInitialStructure();
    }
    
    private void createInitialStructure() {
        root.children.put("bin", new FileNode("bin", true));
        root.children.put("etc", new FileNode("etc", true));
        root.children.put("usr", new FileNode("usr", true));
        root.children.put("tmp", new FileNode("tmp", true));
        root.children.put("home", new FileNode("home", true));
        
        FileNode home = root.children.get("home");
        home.children.put("root", new FileNode("root", true));
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
        file.content = content;
        file.size = content.length();
    }
    
    public String readFile(String name) {
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
            // Simplified chmod
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
