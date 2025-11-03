#ifndef M68K_DISPLAY_H
#define M68K_DISPLAY_H

#include "m68k_cpu.h"
#include "m68k_peripherals.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

// Display tabs/modes
typedef enum {
    TAB_OVERVIEW,
    TAB_DISASSEMBLY,
    TAB_MEMORY,
    TAB_PERIPHERALS,
    TAB_HDMI,
    TAB_PERFORMANCE,
    TAB_COUNT
} DisplayTab;

// Color definitions
#define COLOR_BG         0x1E, 0x1E, 0x2E
#define COLOR_TEXT       0xE0, 0xE0, 0xE0
#define COLOR_HIGHLIGHT  0x4A, 0x9E, 0xFF
#define COLOR_ACTIVE     0xFF, 0x6B, 0x6B
#define COLOR_SUCCESS    0x50, 0xFA, 0x7B
#define COLOR_PANEL      0x28, 0x28, 0x38
#define COLOR_WARNING    0xFF, 0xB8, 0x6C
#define COLOR_BREAKPOINT 0xFF, 0x55, 0x55

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* hdmi_texture;  // For rendering HDMI output
    TTF_Font* font_normal;
    TTF_Font* font_small;
    TTF_Font* font_mono;
    
    bool running;
    bool paused;
    bool step_mode;
    DisplayTab current_tab;
    
    uint32_t fps;
    uint32_t frame_time;
    
    // Memory viewer
    uint32_t mem_view_offset;
    int mem_view_width;
    int mem_view_height;
    
    // Disassembly viewer
    uint32_t disasm_offset;
    int disasm_lines;
    
    // Performance tracking
    uint64_t last_cycles;
    uint64_t last_instructions;
    uint32_t last_time;
    float mips;          // Millions of instructions per second
    float mhz;           // Megahertz
    
} M68K_Display;

// Function prototypes
bool m68k_display_init(M68K_Display* display);
void m68k_display_cleanup(M68K_Display* display);
void m68k_display_render(M68K_Display* display, M68K_CPU* cpu, uint8_t* memory, 
                        size_t mem_size, M68K_Peripherals* peripherals);
bool m68k_display_handle_events(M68K_Display* display, M68K_CPU* cpu);

// Drawing helpers
void draw_text(M68K_Display* display, const char* text, int x, int y, 
              SDL_Color color, TTF_Font* font);
void draw_rect_filled(M68K_Display* display, int x, int y, int w, int h, SDL_Color color);
void draw_rect_outline(M68K_Display* display, int x, int y, int w, int h, SDL_Color color);
void draw_panel(M68K_Display* display, int x, int y, int w, int h, const char* title);

// Tab-specific rendering
void draw_overview_tab(M68K_Display* display, M68K_CPU* cpu, uint8_t* memory, 
                      size_t mem_size, M68K_Peripherals* peripherals);
void draw_disassembly_tab(M68K_Display* display, M68K_CPU* cpu, uint8_t* memory, 
                         size_t mem_size);
void draw_memory_tab(M68K_Display* display, uint8_t* memory, size_t mem_size);
void draw_peripherals_tab(M68K_Display* display, M68K_Peripherals* peripherals);
void draw_hdmi_tab(M68K_Display* display, M68K_Peripherals* peripherals);
void draw_performance_tab(M68K_Display* display, M68K_CPU* cpu);

// Component renderers
void draw_registers(M68K_Display* display, M68K_CPU* cpu, int x, int y);
void draw_apollo_regs(M68K_Display* display, M68K_CPU* cpu, int x, int y);
void draw_memory_view(M68K_Display* display, uint8_t* memory, size_t mem_size, 
                     int x, int y);
void draw_bus_activity(M68K_Display* display, M68K_CPU* cpu, int x, int y);
void draw_status_bar(M68K_Display* display, M68K_CPU* cpu, int x, int y);
void draw_tab_bar(M68K_Display* display, int x, int y);
void draw_breakpoints(M68K_Display* display, M68K_CPU* cpu, int x, int y);
void draw_disassembly(M68K_Display* display, M68K_CPU* cpu, uint8_t* memory,
                     size_t mem_size, int x, int y);
void draw_peripheral_status(M68K_Display* display, M68K_Peripherals* peripherals,
                           int x, int y);

#endif // M68K_DISPLAY_H
