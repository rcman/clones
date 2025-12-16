#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <memory>
#include <thread>
#include <fstream>

// Network includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

// JSON library - using simple string-based approach for portability
#include <sstream>

const int TCP_PORT = 2323;
const std::string SAVE_FILE = "/var/lib/xenix/xenix_state.json";

// Forward declarations
class FileNode;
class FileSystem;
class NetworkTerminal;
class XenixOS;

// Utility function to get timestamp
std::string get_timestamp() {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%s %2d %02d:%02d",
             months[t->tm_mon], t->tm_mday, t->tm_hour, t->tm_min);
    return std::string(buffer);
}

// FileNode class
class FileNode {
public:
    std::string name;
    bool is_directory;
    std::string content;
    std::string permissions;
    std::string owner;
    std::string group;
    size_t size;
    std::string modified;
    std::map<std::string, std::shared_ptr<FileNode>> children;

    FileNode(const std::string& name, bool is_directory = false)
        : name(name), is_directory(is_directory), content(""),
          owner("root"), group("root"), size(0) {
        permissions = is_directory ? "drwxr-xr-x" : "-rw-r--r--";
        modified = get_timestamp();
        if (is_directory) {
            children.clear();
        }
    }
};

// FileSystem class
class FileSystem {
private:
    std::shared_ptr<FileNode> root;
    std::shared_ptr<FileNode> current;
    std::vector<std::string> path_stack;

    void create_initial_structure() {
        std::vector<std::string> dirs = {"bin", "etc", "usr", "tmp", "home", "mnt", "dev", "var"};
        for (const auto& d : dirs) {
            root->children[d] = std::make_shared<FileNode>(d, true);
        }

        // Create usr subdirectories
        auto usr = root->children["usr"];
        for (const auto& d : std::vector<std::string>{"bin", "lib", "spool"}) {
            usr->children[d] = std::make_shared<FileNode>(d, true);
        }

        // Create home/root
        auto home = root->children["home"];
        home->children["root"] = std::make_shared<FileNode>("root", true);

        // Create var/log
        auto var = root->children["var"];
        var->children["log"] = std::make_shared<FileNode>("log", true);

        // Create MOTD
        auto etc = root->children["etc"];
        auto motd = std::make_shared<FileNode>("motd", false);
        motd->content = R"(
                     RESTRICTED RIGHTS LEGEND

Use, duplication, or disclosure is subject to restrictions as set forth
in subparagraph (c)(1)(ii) of the Rights in Technical Data and Computer
Software clause at DFARS 52.227-7013.

Microsoft Corporation
One Microsoft Way
Redmond, Washington  98052-6399
)";
        motd->size = motd->content.length();
        etc->children["motd"] = motd;

        // Create passwd file
        auto passwd = std::make_shared<FileNode>("passwd", false);
        passwd->content = "root:x:0:0:root:/home/root:/bin/sh\n";
        passwd->size = passwd->content.length();
        etc->children["passwd"] = passwd;
    }

    std::pair<std::shared_ptr<FileNode>, std::vector<std::string>> 
    resolve_path(const std::string& path) {
        std::vector<std::string> parts;
        std::vector<std::string> traversed;
        std::shared_ptr<FileNode> node;

        // Split path
        std::stringstream ss(path);
        std::string part;
        while (std::getline(ss, part, '/')) {
            if (!part.empty()) {
                parts.push_back(part);
            }
        }

        if (path[0] == '/') {
            node = root;
            traversed = {"/"};
        } else {
            node = current;
            traversed = path_stack;
        }

        for (const auto& p : parts) {
            if (p == ".") {
                continue;
            } else if (p == "..") {
                if (traversed.size() > 1) {
                    traversed.pop_back();
                    node = navigate_to_path(traversed);
                }
            } else if (node->is_directory && node->children.count(p)) {
                node = node->children[p];
                traversed.push_back(p);
            } else {
                return {nullptr, {}};
            }
        }

        return {node, traversed};
    }

    std::shared_ptr<FileNode> navigate_to_path(const std::vector<std::string>& path) {
        auto node = root;
        for (size_t i = 1; i < path.size(); i++) {
            if (node->children.count(path[i])) {
                node = node->children[path[i]];
            } else {
                return root;
            }
        }
        return node;
    }

public:
    FileSystem() {
        root = std::make_shared<FileNode>("/", true);
        current = root;
        path_stack = {"/"};
        create_initial_structure();
    }

    std::string get_current_path() {
        if (path_stack.size() == 1) {
            return "/";
        }
        std::string result;
        for (const auto& p : path_stack) {
            if (p != "/") {
                result += "/" + p;
            }
        }
        return result.empty() ? "/" : result;
    }

    std::vector<std::shared_ptr<FileNode>> list_files() {
        std::vector<std::shared_ptr<FileNode>> files;
        for (const auto& pair : current->children) {
            files.push_back(pair.second);
        }
        return files;
    }

    std::string change_directory(const std::string& path) {
        std::string p = path;
        if (p == "~" || p.empty()) {
            p = "/home/root";
        }

        auto [node, new_path] = resolve_path(p);

        if (!node) {
            return "cd: " + path + ": No such file or directory";
        }
        if (!node->is_directory) {
            return "cd: " + path + ": Not a directory";
        }

        current = node;
        path_stack = new_path;
        return "";
    }

    std::string create_directory(const std::string& name) {
        if (name.find('/') != std::string::npos) {
            return "mkdir: cannot create directory '" + name + "': Invalid name";
        }
        if (current->children.count(name)) {
            return "mkdir: cannot create directory '" + name + "': File exists";
        }
        current->children[name] = std::make_shared<FileNode>(name, true);
        return "";
    }

    std::string remove_directory(const std::string& name) {
        if (!current->children.count(name)) {
            return "rmdir: failed to remove '" + name + "': No such file or directory";
        }
        auto node = current->children[name];
        if (!node->is_directory) {
            return "rmdir: failed to remove '" + name + "': Not a directory";
        }
        if (!node->children.empty()) {
            return "rmdir: failed to remove '" + name + "': Directory not empty";
        }
        current->children.erase(name);
        return "";
    }

    std::string create_file(const std::string& name) {
        if (!current->children.count(name)) {
            current->children[name] = std::make_shared<FileNode>(name, false);
        } else {
            current->children[name]->modified = get_timestamp();
        }
        return "";
    }

    std::string write_file(const std::string& name, const std::string& content, bool append = false) {
        std::shared_ptr<FileNode> file_node;
        
        if (current->children.count(name)) {
            file_node = current->children[name];
            if (file_node->is_directory) {
                return "cannot write to '" + name + "': Is a directory";
            }
        } else {
            file_node = std::make_shared<FileNode>(name, false);
            current->children[name] = file_node;
        }

        if (append) {
            file_node->content += content;
        } else {
            file_node->content = content;
        }
        file_node->size = file_node->content.length();
        file_node->modified = get_timestamp();
        return "";
    }

    std::pair<std::string, std::string> read_file(const std::string& name) {
        auto [node, _] = resolve_path(name);

        if (!node) {
            return {"", "cat: " + name + ": No such file or directory"};
        }
        if (node->is_directory) {
            return {"", "cat: " + name + ": Is a directory"};
        }

        return {node->content, ""};
    }

    std::string remove_file(const std::string& name, bool force = false, bool recursive = false) {
        if (!current->children.count(name)) {
            if (force) return "";
            return "rm: cannot remove '" + name + "': No such file or directory";
        }

        auto node = current->children[name];
        if (node->is_directory && !recursive) {
            return "rm: cannot remove '" + name + "': Is a directory";
        }

        current->children.erase(name);
        return "";
    }

    std::string copy_file(const std::string& src, const std::string& dest) {
        auto [src_node, _] = resolve_path(src);

        if (!src_node) {
            return "cp: cannot stat '" + src + "': No such file or directory";
        }
        if (src_node->is_directory) {
            return "cp: -r not specified; omitting directory '" + src + "'";
        }

        auto copy = std::make_shared<FileNode>(dest, false);
        copy->content = src_node->content;
        copy->size = src_node->size;
        current->children[dest] = copy;
        return "";
    }

    std::string move_file(const std::string& src, const std::string& dest) {
        if (!current->children.count(src)) {
            return "mv: cannot stat '" + src + "': No such file or directory";
        }

        auto source = current->children[src];
        current->children.erase(src);
        source->name = dest;
        current->children[dest] = source;
        return "";
    }

    std::string chmod_file(const std::string& mode, const std::string& name) {
        if (!current->children.count(name)) {
            return "chmod: cannot access '" + name + "': No such file or directory";
        }

        auto node = current->children[name];
        try {
            int mode_int = std::stoi(mode, nullptr, 8);
            std::string perms = node->is_directory ? "d" : "-";
            for (int i = 0; i < 3; i++) {
                int bits = (mode_int >> (6 - i * 3)) & 7;
                perms += (bits & 4) ? "r" : "-";
                perms += (bits & 2) ? "w" : "-";
                perms += (bits & 1) ? "x" : "-";
            }
            node->permissions = perms;
        } catch (...) {
            return "chmod: invalid mode: '" + mode + "'";
        }
        return "";
    }

    void find_recursive(std::shared_ptr<FileNode> node, const std::string& path,
                       const std::string& pattern, std::vector<std::string>& results) {
        for (const auto& pair : node->children) {
            std::string child_path = (path == "/" ? "/" : path + "/") + pair.first;
            if (pattern == "*" || pair.first.find(pattern) != std::string::npos) {
                results.push_back(child_path);
            }
            if (pair.second->is_directory) {
                find_recursive(pair.second, child_path, pattern, results);
            }
        }
    }

    std::vector<std::string> find(const std::string& pattern, const std::string& start_path = ".") {
        std::vector<std::string> results;
        std::shared_ptr<FileNode> start_node;
        std::string search_path;

        if (start_path == ".") {
            start_node = current;
            search_path = get_current_path();
        } else {
            auto [node, _] = resolve_path(start_path);
            if (!node) return results;
            start_node = node;
            search_path = start_path;
        }

        if (start_node->is_directory) {
            find_recursive(start_node, search_path, pattern, results);
        }

        return results;
    }

    bool save(const std::string& filename) {
        // Simple JSON-like save (simplified for this example)
        std::ofstream file(filename);
        if (!file.is_open()) return false;
        // Implement JSON serialization here if needed
        file.close();
        return true;
    }

    bool load(const std::string& filename) {
        // Simple JSON-like load (simplified for this example)
        std::ifstream file(filename);
        if (!file.is_open()) return false;
        // Implement JSON deserialization here if needed
        file.close();
        return true;
    }
};

// NetworkTerminal class
class NetworkTerminal {
private:
    int socket_fd;
    std::string client_addr;
    bool running;
    std::vector<std::string> history;
    size_t history_index;
    std::string recv_buffer;

    bool recv_byte(char& byte, int timeout_ms = 100) {
        if (!recv_buffer.empty()) {
            byte = recv_buffer[0];
            recv_buffer.erase(0, 1);
            return true;
        }

        struct pollfd pfd;
        pfd.fd = socket_fd;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, timeout_ms);
        if (ret > 0 && (pfd.revents & POLLIN)) {
            char buf[64];
            ssize_t n = recv(socket_fd, buf, sizeof(buf), 0);
            if (n > 0) {
                recv_buffer.append(buf, n);
                byte = recv_buffer[0];
                recv_buffer.erase(0, 1);
                return true;
            } else if (n == 0) {
                running = false;
            }
        }
        return false;
    }

public:
    NetworkTerminal(int socket_fd, const std::string& client_addr)
        : socket_fd(socket_fd), client_addr(client_addr), 
          running(true), history_index(0) {
        // Set non-blocking mode
        int flags = fcntl(socket_fd, F_GETFL, 0);
        fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    }

    ~NetworkTerminal() {
        close(socket_fd);
    }

    void write(const std::string& text) {
        std::string output = text;
        // Replace \n with \r\n for telnet
        size_t pos = 0;
        while ((pos = output.find('\n', pos)) != std::string::npos) {
            output.replace(pos, 1, "\r\n");
            pos += 2;
        }
        send(socket_fd, output.c_str(), output.length(), 0);
    }

    void writeln(const std::string& text = "") {
        write(text + "\n");
    }

    std::string read_line() {
        std::string line;
        history_index = history.size();

        while (running) {
            char ch;
            if (!recv_byte(ch, 30000)) {  // 30 second timeout
                continue;
            }

            // Handle telnet IAC sequences
            if (ch == '\xff') {
                char dummy;
                recv_byte(dummy, 100);
                recv_byte(dummy, 100);
                continue;
            }

            if (ch == '\r' || ch == '\n') {
                if (ch == '\r') {
                    char next;
                    if (recv_byte(next, 50) && next != '\n') {
                        recv_buffer = next + recv_buffer;
                    }
                }

                writeln();
                if (!line.empty()) {
                    history.push_back(line);
                    if (history.size() > 50) {
                        history.erase(history.begin());
                    }
                }
                return line;
            } else if (ch == '\x7f' || ch == '\x08') {  // Backspace
                if (!line.empty()) {
                    line.pop_back();
                    write("\b \b");
                }
            } else if (ch == '\x03') {  // Ctrl+C
                writeln("^C");
                return "";
            } else if (ch == '\x04') {  // Ctrl+D
                if (line.empty()) {
                    writeln("logout");
                    return "exit";
                }
            } else if (ch == '\x15') {  // Ctrl+U
                while (!line.empty()) {
                    write("\b \b");
                    line.pop_back();
                }
            } else if (ch == '\x1b') {  // Escape sequence
                char seq1, seq2;
                if (recv_byte(seq1, 50) && seq1 == '[' && recv_byte(seq2, 50)) {
                    if (seq2 == 'A' && !history.empty() && history_index > 0) {  // Up arrow
                        while (!line.empty()) {
                            write("\b \b");
                            line.pop_back();
                        }
                        history_index--;
                        line = history[history_index];
                        write(line);
                    } else if (seq2 == 'B') {  // Down arrow
                        while (!line.empty()) {
                            write("\b \b");
                            line.pop_back();
                        }
                        if (history_index < history.size() - 1) {
                            history_index++;
                            line = history[history_index];
                            write(line);
                        } else {
                            history_index = history.size();
                        }
                    }
                }
            } else if (ch >= 32 && ch < 127) {
                line += ch;
                write(std::string(1, ch));
            }
        }

        return "exit";
    }

    void clear_screen() {
        write("\x1b[2J\x1b[H");
    }

    bool is_running() const { return running; }
    const std::vector<std::string>& get_history() const { return history; }
};

// XenixOS class
class XenixOS {
private:
    std::unique_ptr<FileSystem> fs;
    std::string current_user;
    std::string hostname;
    std::shared_ptr<NetworkTerminal> terminal;
    bool running;
    std::string save_file;
    std::map<std::string, std::string> env;

    void show_login_time() {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Last login: %s %s %2d %02d:%02d on tty01",
                days[t->tm_wday], months[t->tm_mon], t->tm_mday, t->tm_hour, t->tm_min);
        terminal->writeln(buffer);
    }

    std::vector<std::string> split_args(const std::string& cmd_line) {
        std::vector<std::string> args;
        std::stringstream ss(cmd_line);
        std::string arg;
        while (ss >> arg) {
            args.push_back(arg);
        }
        return args;
    }

public:
    XenixOS(std::shared_ptr<NetworkTerminal> terminal)
        : terminal(terminal), current_user("root"), hostname("pico"),
          running(true), save_file(SAVE_FILE) {
        fs = std::make_unique<FileSystem>();
        env["PATH"] = "/bin:/usr/bin";
        env["HOME"] = "/home/root";
        env["SHELL"] = "/bin/sh";
        env["USER"] = "root";
        env["TERM"] = "vt100";
        
        fs->load(save_file);
    }

    void boot() {
        terminal->clear_screen();
        terminal->writeln();
        terminal->writeln("The XENIX System");
        terminal->writeln();
        terminal->writeln();
        terminal->writeln("Copyright (c) 1984, 1985, 1986, 1987, 1988");
        terminal->writeln("The Santa Cruz Operation, Inc.");
        terminal->writeln("Microsoft Corporation");
        terminal->writeln("AT&T");
        terminal->writeln("All Rights Reserved");
        terminal->writeln();
        terminal->writeln();

        terminal->write("login: ");
        terminal->writeln(current_user);
        terminal->writeln();

        auto [content, err] = fs->read_file("/etc/motd");
        if (err.empty() && !content.empty()) {
            terminal->write(content);
        }

        terminal->writeln();
        show_login_time();
        terminal->writeln();

        while (running && terminal->is_running()) {
            std::string prompt = (current_user == "root") ? "# " : "$ ";
            terminal->write(prompt);
            std::string cmd_line = terminal->read_line();

            if (!cmd_line.empty()) {
                execute_command(cmd_line);
            }
        }
    }

    void execute_command(const std::string& cmd_line) {
        auto args = split_args(cmd_line);
        if (args.empty()) return;

        std::string cmd = args[0];
        args.erase(args.begin());

        // Command routing
        if (cmd == "ls") cmd_ls(args);
        else if (cmd == "cd") cmd_cd(args);
        else if (cmd == "pwd") cmd_pwd(args);
        else if (cmd == "mkdir") cmd_mkdir(args);
        else if (cmd == "rmdir") cmd_rmdir(args);
        else if (cmd == "cat") cmd_cat(args);
        else if (cmd == "echo") cmd_echo(args);
        else if (cmd == "touch") cmd_touch(args);
        else if (cmd == "rm") cmd_rm(args);
        else if (cmd == "cp") cmd_cp(args);
        else if (cmd == "mv") cmd_mv(args);
        else if (cmd == "chmod") cmd_chmod(args);
        else if (cmd == "find") cmd_find(args);
        else if (cmd == "ps") cmd_ps(args);
        else if (cmd == "who") cmd_who(args);
        else if (cmd == "whoami") cmd_whoami(args);
        else if (cmd == "date") cmd_date(args);
        else if (cmd == "clear") cmd_clear(args);
        else if (cmd == "help") cmd_help(args);
        else if (cmd == "exit" || cmd == "logout") cmd_exit(args);
        else if (cmd == "uname") cmd_uname(args);
        else terminal->writeln(cmd + ": not found");
    }

    // Command implementations
    void cmd_ls(const std::vector<std::string>& args) {
        auto files = fs->list_files();
        bool long_format = std::find(args.begin(), args.end(), "-l") != args.end();

        if (long_format) {
            terminal->writeln("total " + std::to_string(files.size()));
            for (const auto& f : files) {
                char line[256];
                snprintf(line, sizeof(line), "%s %2d %-8s %-8s %8zu %s %s",
                        f->permissions.c_str(), 1, f->owner.c_str(), f->group.c_str(),
                        f->size, f->modified.c_str(), f->name.c_str());
                terminal->writeln(line);
            }
        } else {
            std::string line;
            for (const auto& f : files) {
                line += f->name + "  ";
            }
            if (!line.empty()) terminal->writeln(line);
        }
    }

    void cmd_cd(const std::vector<std::string>& args) {
        std::string path = args.empty() ? "~" : args[0];
        std::string err = fs->change_directory(path);
        if (!err.empty()) terminal->writeln(err);
    }

    void cmd_pwd(const std::vector<std::string>& args) {
        terminal->writeln(fs->get_current_path());
    }

    void cmd_mkdir(const std::vector<std::string>& args) {
        if (args.empty()) {
            terminal->writeln("usage: mkdir directory ...");
            return;
        }
        for (const auto& name : args) {
            std::string err = fs->create_directory(name);
            if (!err.empty()) terminal->writeln(err);
        }
    }

    void cmd_rmdir(const std::vector<std::string>& args) {
        if (args.empty()) {
            terminal->writeln("usage: rmdir directory ...");
            return;
        }
        for (const auto& name : args) {
            std::string err = fs->remove_directory(name);
            if (!err.empty()) terminal->writeln(err);
        }
    }

    void cmd_cat(const std::vector<std::string>& args) {
        if (args.empty()) {
            terminal->writeln("usage: cat file ...");
            return;
        }
        for (const auto& filename : args) {
            auto [content, err] = fs->read_file(filename);
            if (!err.empty()) {
                terminal->writeln(err);
            } else if (!content.empty()) {
                if (content.back() == '\n') {
                    terminal->write(content);
                } else {
                    terminal->writeln(content);
                }
            }
        }
    }

    void cmd_echo(const std::vector<std::string>& args) {
        std::string text;
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) text += " ";
            text += args[i];
        }
        terminal->writeln(text);
    }

    void cmd_touch(const std::vector<std::string>& args) {
        if (args.empty()) {
            terminal->writeln("usage: touch file ...");
            return;
        }
        for (const auto& name : args) {
            fs->create_file(name);
        }
    }

    void cmd_rm(const std::vector<std::string>& args) {
        if (args.empty()) {
            terminal->writeln("usage: rm [-rf] file ...");
            return;
        }
        bool force = std::find(args.begin(), args.end(), "-f") != args.end();
        bool recursive = std::find(args.begin(), args.end(), "-r") != args.end() ||
                        std::find(args.begin(), args.end(), "-rf") != args.end();
        
        for (const auto& name : args) {
            if (name[0] != '-') {
                std::string err = fs->remove_file(name, force, recursive);
                if (!err.empty()) terminal->writeln(err);
            }
        }
    }

    void cmd_cp(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            terminal->writeln("usage: cp source dest");
            return;
        }
        std::string err = fs->copy_file(args[0], args[1]);
        if (!err.empty()) terminal->writeln(err);
    }

    void cmd_mv(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            terminal->writeln("usage: mv source dest");
            return;
        }
        std::string err = fs->move_file(args[0], args[1]);
        if (!err.empty()) terminal->writeln(err);
    }

    void cmd_chmod(const std::vector<std::string>& args) {
        if (args.size() < 2) {
            terminal->writeln("usage: chmod mode file");
            return;
        }
        std::string err = fs->chmod_file(args[0], args[1]);
        if (!err.empty()) terminal->writeln(err);
    }

    void cmd_find(const std::vector<std::string>& args) {
        std::string pattern = "*";
        std::string path = ".";
        
        for (size_t i = 0; i < args.size(); i++) {
            if (args[i] == "-name" && i + 1 < args.size()) {
                pattern = args[i + 1];
                i++;
            } else if (args[i][0] != '-') {
                path = args[i];
            }
        }
        
        auto results = fs->find(pattern, path);
        for (const auto& r : results) {
            terminal->writeln(r);
        }
    }

    void cmd_ps(const std::vector<std::string>& args) {
        terminal->writeln("  PID TTY          TIME CMD");
        terminal->writeln("    1 tty01    00:00:00 sh");
        terminal->writeln("   42 tty01    00:00:00 ps");
    }

    void cmd_who(const std::vector<std::string>& args) {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%-8s tty01     %s %2d %02d:%02d",
                current_user.c_str(), months[t->tm_mon], t->tm_mday, t->tm_hour, t->tm_min);
        terminal->writeln(buffer);
    }

    void cmd_whoami(const std::vector<std::string>& args) {
        terminal->writeln(current_user);
    }

    void cmd_date(const std::vector<std::string>& args) {
        time_t now = time(nullptr);
        struct tm* t = localtime(&now);
        const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%s %s %2d %02d:%02d:%02d UTC %d",
                days[t->tm_wday], months[t->tm_mon], t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec, t->tm_year + 1900);
        terminal->writeln(buffer);
    }

    void cmd_clear(const std::vector<std::string>& args) {
        terminal->clear_screen();
    }

    void cmd_uname(const std::vector<std::string>& args) {
        if (!args.empty() && args[0] == "-a") {
            terminal->writeln("XENIX pico 2.3.4 3.2v5.0.6c i386");
        } else {
            terminal->writeln("XENIX");
        }
    }

    void cmd_help(const std::vector<std::string>& args) {
        terminal->writeln("XENIX Commands:");
        terminal->writeln();
        terminal->writeln("Files:    ls cd pwd mkdir rmdir cat touch rm cp mv chmod");
        terminal->writeln("Search:   find");
        terminal->writeln("System:   ps who whoami date clear uname help exit");
    }

    void cmd_exit(const std::vector<std::string>& args) {
        terminal->writeln("Saving...");
        fs->save(save_file);
        terminal->writeln();
        terminal->writeln("logout");
        running = false;
    }
};

// Server class
class XenixServer {
private:
    int port;
    int server_socket;
    bool running;

    void handle_client(int client_socket, const std::string& client_addr) {
        auto terminal = std::make_shared<NetworkTerminal>(client_socket, client_addr);
        auto xenix = std::make_unique<XenixOS>(terminal);
        
        try {
            xenix->boot();
        } catch (const std::exception& e) {
            std::cerr << "Session error: " << e.what() << std::endl;
        }
        
        std::cout << "Connection closed: " << client_addr << std::endl;
    }

public:
    XenixServer(int port = TCP_PORT) : port(port), server_socket(-1), running(false) {}

    ~XenixServer() {
        stop();
    }

    bool start() {
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }

        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to bind socket" << std::endl;
            close(server_socket);
            return false;
        }

        if (listen(server_socket, 5) < 0) {
            std::cerr << "Failed to listen" << std::endl;
            close(server_socket);
            return false;
        }

        running = true;

        std::cout << "\n" << std::string(50, '=') << std::endl;
        std::cout << "XENIX TCP Server Started" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        std::cout << "Port: " << port << std::endl;
        std::cout << "\nConnect using:" << std::endl;
        std::cout << "  telnet localhost " << port << std::endl;
        std::cout << "  or: nc localhost " << port << std::endl;
        std::cout << std::string(50, '=') << "\n" << std::endl;

        return true;
    }

    void run() {
        if (!running && !start()) {
            return;
        }

        std::cout << "Waiting for connections..." << std::endl;

        while (running) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                if (running) {
                    std::cerr << "Accept failed" << std::endl;
                }
                continue;
            }

            char addr_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str));
            std::string client_addr_str = std::string(addr_str) + ":" + 
                                         std::to_string(ntohs(client_addr.sin_port));

            std::cout << "\nConnection from: " << client_addr_str << std::endl;

            // Handle client in a new thread
            std::thread client_thread(&XenixServer::handle_client, this, 
                                     client_socket, client_addr_str);
            client_thread.detach();
        }
    }

    void stop() {
        running = false;
        if (server_socket >= 0) {
            close(server_socket);
            server_socket = -1;
        }
    }
};

int main(int argc, char* argv[]) {
    // Create state directory if it doesn't exist
    system("mkdir -p /var/lib/xenix");

    XenixServer server(TCP_PORT);
    
    std::cout << "Starting XENIX Server..." << std::endl;
    
    server.run();

    return 0;
}
