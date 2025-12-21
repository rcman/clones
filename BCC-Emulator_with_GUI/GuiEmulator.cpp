/*
 * BCC-500 GUI Emulator with Assembler
 * Enhanced user interface with ncurses
 */

#include "EmulatorCore.h"
#include "Assembler.h"
#include <ncurses.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

class FileManager {
private:
    std::vector<std::string> files;
    std::string currentPath;
    int selectedIndex;

public:
    FileManager() : currentPath("."), selectedIndex(0) {}

    void scanDirectory(const std::string& path) {
        files.clear();
        currentPath = path;

        DIR* dir = opendir(path.c_str());
        if (!dir) return;

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == ".") continue;

            // Add directories and .asm files
            struct stat statbuf;
            std::string fullPath = path + "/" + name;
            if (stat(fullPath.c_str(), &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode)) {
                    files.push_back("[DIR] " + name);
                } else if (name.length() > 4 && name.substr(name.length() - 4) == ".asm") {
                    files.push_back(name);
                }
            }
        }
        closedir(dir);

        std::sort(files.begin(), files.end());
    }

    const std::vector<std::string>& getFiles() const { return files; }
    int getSelectedIndex() const { return selectedIndex; }
    void setSelectedIndex(int idx) { selectedIndex = std::max(0, std::min(idx, (int)files.size() - 1)); }
    std::string getCurrentPath() const { return currentPath; }

    std::string getSelectedFile() const {
        if (selectedIndex < 0 || selectedIndex >= (int)files.size()) return "";
        std::string file = files[selectedIndex];
        if (file.substr(0, 6) == "[DIR] ") {
            return file.substr(6);
        }
        return file;
    }

    bool isSelectedDirectory() const {
        if (selectedIndex < 0 || selectedIndex >= (int)files.size()) return false;
        return files[selectedIndex].substr(0, 6) == "[DIR] ";
    }
};

class NcursesGUI {
private:
    WINDOW *mainWin, *menuWin, *statusWin, *displayWin, *registerWin, *memoryWin;
    BCC500System& system;
    Assembler assembler;
    FileManager fileManager;
    int selectedMenu;
    bool running;
    bool programLoaded;
    std::string loadedFile;
    std::string statusMessage;
    bool needsResize;

    // Minimum terminal size requirements
    static constexpr int MIN_WIDTH = 80;
    static constexpr int MIN_HEIGHT = 24;

    void initWindows() {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        timeout(100);  // 100ms timeout for getch() to allow periodic updates

        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_BLACK, COLOR_CYAN);     // Menu bar
            init_pair(2, COLOR_WHITE, COLOR_BLUE);     // Menu selected
            init_pair(3, COLOR_GREEN, COLOR_BLACK);    // Status
            init_pair(4, COLOR_YELLOW, COLOR_BLACK);   // Headers
            init_pair(5, COLOR_RED, COLOR_BLACK);      // Errors
            init_pair(6, COLOR_WHITE, COLOR_BLACK);    // Normal
        }

        createWindows();
    }

    void createWindows() {
        int maxY, maxX;
        getmaxyx(stdscr, maxY, maxX);

        // Check if terminal is too small
        if (maxX < MIN_WIDTH || maxY < MIN_HEIGHT) {
            // Create a simple error window
            clear();
            mvprintw(maxY / 2, (maxX - 40) / 2, "Terminal too small!");
            mvprintw(maxY / 2 + 1, (maxX - 40) / 2, "Minimum: %dx%d  Current: %dx%d",
                     MIN_WIDTH, MIN_HEIGHT, maxX, maxY);
            mvprintw(maxY / 2 + 2, (maxX - 40) / 2, "Please resize your terminal window");
            refresh();
            return;
        }

        // Menu bar (top)
        menuWin = newwin(1, maxX, 0, 0);

        // Status bar (bottom)
        statusWin = newwin(1, maxX, maxY - 1, 0);

        // Calculate dimensions for content area
        int contentHeight = maxY - 2;  // Subtract menu and status bars
        int halfHeight = contentHeight / 2;
        int halfWidth = maxX / 2;

        // Terminal display (left half, top)
        displayWin = newwin(halfHeight, halfWidth, 1, 0);

        // Registers (right half, top)
        registerWin = newwin(halfHeight, maxX - halfWidth, 1, halfWidth);

        // Memory view (bottom half)
        memoryWin = newwin(contentHeight - halfHeight, maxX, 1 + halfHeight, 0);

        scrollok(displayWin, TRUE);
        scrollok(memoryWin, TRUE);
    }

    void destroyWindows() {
        if (menuWin) { delwin(menuWin); menuWin = nullptr; }
        if (statusWin) { delwin(statusWin); statusWin = nullptr; }
        if (displayWin) { delwin(displayWin); displayWin = nullptr; }
        if (registerWin) { delwin(registerWin); registerWin = nullptr; }
        if (memoryWin) { delwin(memoryWin); memoryWin = nullptr; }
    }

    void handleResize() {
        // Destroy existing windows
        destroyWindows();

        // Refresh ncurses to get new terminal size
        endwin();
        refresh();
        clear();

        // Recreate windows with new dimensions
        createWindows();

        // Force a complete redraw
        clearok(stdscr, TRUE);
        refresh();

        needsResize = false;
        statusMessage = "Terminal resized successfully";
    }

    bool checkMinimumSize() {
        int maxY, maxX;
        getmaxyx(stdscr, maxY, maxX);
        return (maxX >= MIN_WIDTH && maxY >= MIN_HEIGHT);
    }

    void drawMenu() {
        if (!menuWin) return;
        werase(menuWin);
        wbkgd(menuWin, COLOR_PAIR(1));

        const char* menus[] = {"[F1]Load", "[F2]Assemble", "[F3]Run", "[F4]Step", "[F5]Stop", "[F6]Reset", "[ESC]Quit"};
        int numMenus = 7;

        for (int i = 0; i < numMenus; i++) {
            if (i == selectedMenu) {
                wattron(menuWin, COLOR_PAIR(2) | A_BOLD);
            }
            mvwprintw(menuWin, 0, i * 13, " %s ", menus[i]);
            wattroff(menuWin, COLOR_PAIR(2) | A_BOLD);
        }

        wrefresh(menuWin);
    }

    void drawStatus() {
        if (!statusWin) return;
        werase(statusWin);
        wbkgd(statusWin, COLOR_PAIR(3));

        if (!statusMessage.empty()) {
            wattron(statusWin, A_BOLD);
            mvwprintw(statusWin, 0, 2, "%s", statusMessage.c_str());
            wattroff(statusWin, A_BOLD);
        }

        int maxY, maxX;
        getmaxyx(statusWin, maxY, maxX);
        (void)maxY; // Unused

        if (programLoaded) {
            mvwprintw(statusWin, 0, maxX - 30, " Loaded: %-15s ", loadedFile.c_str());
        }

        wrefresh(statusWin);
    }

    void drawDisplay() {
        if (!displayWin) return;
        werase(displayWin);
        box(displayWin, 0, 0);
        wattron(displayWin, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(displayWin, 0, 2, " Terminal Display ");
        wattroff(displayWin, COLOR_PAIR(4) | A_BOLD);

        // Get terminal device
        auto terminal = std::dynamic_pointer_cast<TerminalDisplay>(
            system.getIOController().getDevice("terminal"));

        if (terminal) {
            std::string screen = terminal->getScreenContents();
            std::istringstream iss(screen);
            std::string line;
            int y = 1;
            int maxY, maxX;
            getmaxyx(displayWin, maxY, maxX);

            while (std::getline(iss, line) && y < maxY - 1) {
                if (line.length() > (size_t)(maxX - 3)) {
                    line = line.substr(0, maxX - 3);
                }
                mvwprintw(displayWin, y++, 2, "%s", line.c_str());
            }
        }

        wrefresh(displayWin);
    }

    void drawRegisters() {
        if (!registerWin) return;
        werase(registerWin);
        box(registerWin, 0, 0);
        wattron(registerWin, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(registerWin, 0, 2, " Processor 0 State ");
        wattroff(registerWin, COLOR_PAIR(4) | A_BOLD);

        BCC500Processor& proc = system.getProcessor(0);

        int y = 2;
        wattron(registerWin, COLOR_PAIR(6));

        // State
        mvwprintw(registerWin, y++, 2, "State: ");
        switch (proc.getState()) {
            case ProcessorState::IDLE:       wprintw(registerWin, "IDLE"); break;
            case ProcessorState::RUNNING:    wprintw(registerWin, "RUNNING"); break;
            case ProcessorState::WAITING:    wprintw(registerWin, "WAITING"); break;
            case ProcessorState::HALTED:     wprintw(registerWin, "HALTED"); break;
            case ProcessorState::ERROR:      wprintw(registerWin, "ERROR"); break;
            case ProcessorState::BREAKPOINT: wprintw(registerWin, "BREAKPOINT"); break;
        }
        y++;

        // Registers
        mvwprintw(registerWin, y++, 2, "PC:  0x%05X", proc.getPC());
        mvwprintw(registerWin, y++, 2, "ACC: 0x%06X", proc.getACC());
        mvwprintw(registerWin, y++, 2, "B:   0x%06X", proc.getRegB());
        mvwprintw(registerWin, y++, 2, "X:   0x%06X", proc.getRegX());
        mvwprintw(registerWin, y++, 2, "SP:  0x%05X", proc.getSP());
        y++;

        // Flags
        const Flags& flags = proc.getFlags();
        mvwprintw(registerWin, y++, 2, "Flags: %c%c%c%c",
                  flags.zero ? 'Z' : '-',
                  flags.negative ? 'N' : '-',
                  flags.overflow ? 'O' : '-',
                  flags.carry ? 'C' : '-');
        y++;

        // Statistics
        mvwprintw(registerWin, y++, 2, "Cycles: %lu", proc.getCycles());
        mvwprintw(registerWin, y++, 2, "Instructions: %lu", proc.getInstructionsExecuted());

        wattroff(registerWin, COLOR_PAIR(6));
        wrefresh(registerWin);
    }

    void drawMemory() {
        if (!memoryWin) return;
        werase(memoryWin);
        box(memoryWin, 0, 0);
        wattron(memoryWin, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(memoryWin, 0, 2, " Memory View (0x0000-0x007F) ");
        wattroff(memoryWin, COLOR_PAIR(4) | A_BOLD);

        int y = 1;
        int maxY, maxX;
        getmaxyx(memoryWin, maxY, maxX);
        (void)maxX; // Unused

        for (Addr18 addr = 0; addr < 0x80 && y < maxY - 1; addr += 8) {
            mvwprintw(memoryWin, y, 2, "%04X:", addr);
            for (int i = 0; i < 8; i++) {
                Word24 value = system.getMemory().read(addr + i);
                wprintw(memoryWin, " %06X", value);
            }
            y++;
        }

        wrefresh(memoryWin);
    }

    void showFileBrowser() {
        int maxY, maxX;
        getmaxyx(stdscr, maxY, maxX);

        int winH = maxY - 6;
        int winW = maxX - 20;
        int winY = 3;
        int winX = 10;

        WINDOW* fileWin = newwin(winH, winW, winY, winX);
        box(fileWin, 0, 0);
        keypad(fileWin, TRUE);

        fileManager.scanDirectory("examples");

        int topIndex = 0;
        bool done = false;

        while (!done) {
            werase(fileWin);
            box(fileWin, 0, 0);
            wattron(fileWin, COLOR_PAIR(4) | A_BOLD);
            mvwprintw(fileWin, 0, 2, " Select Assembly File ");
            wattroff(fileWin, COLOR_PAIR(4) | A_BOLD);

            mvwprintw(fileWin, 1, 2, "Directory: %s", fileManager.getCurrentPath().c_str());
            mvwprintw(fileWin, winH - 2, 2, "↑/↓: Select  Enter: Load  ESC: Cancel");

            const auto& files = fileManager.getFiles();
            int displayLines = winH - 5;

            for (int i = 0; i < displayLines && (topIndex + i) < (int)files.size(); i++) {
                int fileIdx = topIndex + i;
                if (fileIdx == fileManager.getSelectedIndex()) {
                    wattron(fileWin, A_REVERSE | A_BOLD);
                }
                mvwprintw(fileWin, 3 + i, 2, "%-*s", winW - 4, files[fileIdx].c_str());
                if (fileIdx == fileManager.getSelectedIndex()) {
                    wattroff(fileWin, A_REVERSE | A_BOLD);
                }
            }

            wrefresh(fileWin);

            int ch = wgetch(fileWin);
            switch (ch) {
                case KEY_UP:
                    if (fileManager.getSelectedIndex() > 0) {
                        fileManager.setSelectedIndex(fileManager.getSelectedIndex() - 1);
                        if (fileManager.getSelectedIndex() < topIndex) {
                            topIndex--;
                        }
                    }
                    break;
                case KEY_DOWN:
                    if (fileManager.getSelectedIndex() < (int)files.size() - 1) {
                        fileManager.setSelectedIndex(fileManager.getSelectedIndex() + 1);
                        if (fileManager.getSelectedIndex() >= topIndex + displayLines) {
                            topIndex++;
                        }
                    }
                    break;
                case '\n':
                case KEY_ENTER:
                    if (fileManager.isSelectedDirectory()) {
                        std::string newPath = fileManager.getCurrentPath() + "/" + fileManager.getSelectedFile();
                        fileManager.scanDirectory(newPath);
                        fileManager.setSelectedIndex(0);
                        topIndex = 0;
                    } else {
                        done = true;
                    }
                    break;
                case 27: // ESC
                    delwin(fileWin);
                    return;
            }
        }

        // Load the selected file
        std::string filename = fileManager.getCurrentPath() + "/" + fileManager.getSelectedFile();
        loadAndAssemble(filename);

        delwin(fileWin);
    }

    void loadAndAssemble(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            statusMessage = "Error: Cannot open file!";
            return;
        }

        std::string source((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        if (assembler.assemble(source)) {
            const auto& machineCode = assembler.getMachineCode();
            system.reset();
            system.loadProgram(machineCode, 0, 0);
            programLoaded = true;
            loadedFile = filename.substr(filename.find_last_of("/") + 1);
            statusMessage = "Program assembled and loaded successfully!";
        } else {
            const auto& errors = assembler.getErrors();
            statusMessage = "Assembly errors: " + errors[0];
            programLoaded = false;
        }
    }

    void runProgram() {
        if (!programLoaded) {
            statusMessage = "No program loaded!";
            return;
        }

        system.getProcessor(0).setState(ProcessorState::RUNNING);

        // Run in separate thread
        std::thread runThread([this]() {
            system.runProcessor(0, 10000); // Max 10000 cycles
        });

        // Update display while running
        int refreshCount = 0;
        while (system.getProcessor(0).getState() == ProcessorState::RUNNING) {
            if (refreshCount++ % 100 == 0) {
                drawDisplay();
                drawRegisters();
                drawMemory();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        runThread.join();
        statusMessage = "Program execution completed.";
    }

    void stepProgram() {
        if (!programLoaded) {
            statusMessage = "No program loaded!";
            return;
        }

        BCC500Processor& proc = system.getProcessor(0);
        if (proc.getState() != ProcessorState::RUNNING) {
            proc.setState(ProcessorState::RUNNING);
        }
        proc.step();
        statusMessage = "Single step executed.";
    }

    void stopProgram() {
        system.stopAllProcessors();
        statusMessage = "Execution stopped.";
    }

    void resetProgram() {
        system.reset();
        if (programLoaded && !loadedFile.empty()) {
            std::string fullPath = fileManager.getCurrentPath() + "/" + loadedFile;
            loadAndAssemble(fullPath);
        }
        statusMessage = "System reset.";
    }

public:
    NcursesGUI(BCC500System& sys)
        : mainWin(nullptr), menuWin(nullptr), statusWin(nullptr),
          displayWin(nullptr), registerWin(nullptr), memoryWin(nullptr),
          system(sys), selectedMenu(0), running(true), programLoaded(false),
          needsResize(false) {
        initWindows();
        statusMessage = "Welcome to BCC-500 GUI Emulator! Press F1 to load a program.";
    }

    ~NcursesGUI() {
        destroyWindows();
        if (mainWin) delwin(mainWin);
        endwin();
    }

    void run() {
        while (running) {
            // Handle resize if needed
            if (needsResize) {
                handleResize();
            }

            // Check if terminal meets minimum size requirements
            if (!checkMinimumSize()) {
                clear();
                int maxY, maxX;
                getmaxyx(stdscr, maxY, maxX);
                mvprintw(maxY / 2, (maxX - 40) / 2, "Terminal too small!");
                mvprintw(maxY / 2 + 1, (maxX - 40) / 2, "Minimum: %dx%d  Current: %dx%d",
                         MIN_WIDTH, MIN_HEIGHT, maxX, maxY);
                mvprintw(maxY / 2 + 2, (maxX - 40) / 2, "Please resize your terminal window");
                mvprintw(maxY / 2 + 4, (maxX - 40) / 2, "Press ESC or 'q' to quit");
                refresh();

                int ch = getch();
                if (ch == KEY_RESIZE) {
                    needsResize = true;
                } else if (ch == 27 || ch == 'q' || ch == 'Q') {  // 27 = ESC
                    running = false;
                }
                continue;
            }

            drawMenu();
            drawStatus();
            drawDisplay();
            drawRegisters();
            drawMemory();

            int ch = getch();

            // Handle timeout (ERR means no key pressed)
            if (ch == ERR) {
                continue;
            }

            switch (ch) {
                case KEY_RESIZE:
                    needsResize = true;
                    break;
                case KEY_F(1):  // Load
                    showFileBrowser();
                    break;
                case KEY_F(2):  // Assemble (reload current)
                    if (!loadedFile.empty()) {
                        std::string fullPath = fileManager.getCurrentPath() + "/" + loadedFile;
                        loadAndAssemble(fullPath);
                    }
                    break;
                case KEY_F(3):  // Run
                    runProgram();
                    break;
                case KEY_F(4):  // Step
                    stepProgram();
                    break;
                case KEY_F(5):  // Stop
                    stopProgram();
                    break;
                case KEY_F(6):  // Reset
                    resetProgram();
                    break;
                case 27:  // ESC
                case 'q':
                case 'Q':
                    running = false;
                    break;
            }
        }
    }
};

int main() {
    BCC500System system;

    NcursesGUI gui(system);
    gui.run();

    return 0;
}
