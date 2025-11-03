#include "m68k_display.h"
#include "m68k_peripherals.h"
#include <stdio.h>
#include <string.h>

bool m68k_display_init(M68K_Display* display) {
    memset(display, 0, sizeof(M68K_Display));
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }
    
    if (TTF_Init() < 0) {
        printf("TTF initialization failed: %s\n", TTF_GetError());
        return false;
    }
    
    display->window = SDL_CreateWindow(
        "68000 Simulator - Complete Edition with Apollo 68080 & Peripherals",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!display->window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return false;
    }
    
    display->renderer = SDL_CreateRenderer(display->window, -1, 
                                          SDL_RENDERER_ACCELERATED | 
                                          SDL_RENDERER_PRESENTVSYNC);
    
    if (!display->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return false;
    }
    
    // Create texture for HDMI output
    display->hdmi_texture = SDL_CreateTexture(display->renderer,
                                             SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             1920, 1080);
    
    // Try to load fonts
    const char* font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/System/Library/Fonts/Courier.dfont",
        "C:\\Windows\\Fonts\\consola.ttf",
        NULL
    };
    
    bool font_loaded = false;
    for (int i = 0; font_paths[i] != NULL; i++) {
        display->font_normal = TTF_OpenFont(font_paths[i], 16);
        display->font_small = TTF_OpenFont(font_paths[i], 12);
        display->font_mono = TTF_OpenFont(font_paths[i], 14);
        if (display->font_normal && display->font_small && display->font_mono) {
            font_loaded = true;
            break;
        }
    }
    
    if (!font_loaded) {
        printf("Warning: Could not load fonts\n");
    }
    
    display->running = true;
    display->paused = true;
    display->step_mode = false;
    display->current_tab = TAB_OVERVIEW;
    display->mem_view_offset = 0;
    display->mem_view_width = 16;
    display->mem_view_height = 32;
    display->disasm_offset = 0x1000;
    display->disasm_lines = 40;
    
    return true;
}

void m68k_display_cleanup(M68K_Display* display) {
    if (display->hdmi_texture) SDL_DestroyTexture(display->hdmi_texture);
    if (display->font_normal) TTF_CloseFont(display->font_normal);
    if (display->font_small) TTF_CloseFont(display->font_small);
    if (display->font_mono) TTF_CloseFont(display->font_mono);
    if (display->renderer) SDL_DestroyRenderer(display->renderer);
    if (display->window) SDL_DestroyWindow(display->window);
    
    TTF_Quit();
    SDL_Quit();
}

void draw_text(M68K_Display* display, const char* text, int x, int y, 
              SDL_Color color, TTF_Font* font) {
    if (!font || !text || text[0] == '\0') return;
    
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(display->renderer, surface);
    if (texture) {
        SDL_Rect rect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(display->renderer, texture, NULL, &rect);
        SDL_DestroyTexture(texture);
    }
    
    SDL_FreeSurface(surface);
}

void draw_rect_filled(M68K_Display* display, int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(display->renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(display->renderer, &rect);
}

void draw_rect_outline(M68K_Display* display, int x, int y, int w, int h, SDL_Color color) {
    SDL_SetRenderDrawColor(display->renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(display->renderer, &rect);
}

void draw_panel(M68K_Display* display, int x, int y, int w, int h, const char* title) {
    SDL_Color panel_color = {COLOR_PANEL, 255};
    SDL_Color border_color = {COLOR_HIGHLIGHT, 255};
    SDL_Color text_color = {COLOR_TEXT, 255};
    
    draw_rect_filled(display, x, y, w, h, panel_color);
    draw_rect_outline(display, x, y, w, h, border_color);
    
    if (title && display->font_normal) {
        draw_text(display, title, x + 10, y + 5, text_color, display->font_normal);
    }
}

void draw_tab_bar(M68K_Display* display, int x, int y) {
    const char* tab_names[] = {
        "Overview", "Disassembly", "Memory", "Peripherals", "HDMI", "Performance"
    };
    
    int tab_width = 150;
    int tab_height = 35;
    
    SDL_Color active_color = {COLOR_HIGHLIGHT, 255};
    SDL_Color inactive_color = {COLOR_PANEL, 255};
    SDL_Color text_color = {COLOR_TEXT, 255};
    
    for (int i = 0; i < TAB_COUNT; i++) {
        SDL_Color bg = (i == display->current_tab) ? active_color : inactive_color;
        draw_rect_filled(display, x + i * tab_width, y, tab_width - 2, tab_height, bg);
        draw_rect_outline(display, x + i * tab_width, y, tab_width - 2, tab_height, active_color);
        draw_text(display, tab_names[i], x + i * tab_width + 10, y + 10, 
                 text_color, display->font_small);
    }
}

void draw_breakpoints(M68K_Display* display, M68K_CPU* cpu, int x, int y) {
    draw_panel(display, x, y, 350, 200, "Breakpoints");
    
    SDL_Color text_color = {COLOR_TEXT, 255};
    SDL_Color bp_color = {COLOR_BREAKPOINT, 255};
    
    int line_y = y + 35;
    char buffer[128];
    
    if (cpu->num_breakpoints == 0) {
        draw_text(display, "No breakpoints set", x + 15, line_y, text_color, display->font_small);
        line_y += 18;
        draw_text(display, "Press 'B' to add breakpoint at PC", x + 15, line_y, 
                 text_color, display->font_small);
    } else {
        for (int i = 0; i < cpu->num_breakpoints && i < 8; i++) {
            snprintf(buffer, sizeof(buffer), "%c 0x%08X  (hits: %llu)",
                    cpu->breakpoints[i].enabled ? '*' : ' ',
                    cpu->breakpoints[i].address,
                    (unsigned long long)cpu->breakpoints[i].hit_count);
            draw_text(display, buffer, x + 15, line_y, 
                     cpu->breakpoints[i].enabled ? bp_color : text_color, 
                     display->font_small);
            line_y += 18;
        }
    }
}

void draw_disassembly(M68K_Display* display, M68K_CPU* cpu, uint8_t* memory,
                     size_t mem_size, int x, int y) {
    draw_panel(display, x, y, 900, 800, "Disassembly");
    
    SDL_Color text_color = {COLOR_TEXT, 255};
    SDL_Color pc_color = {COLOR_SUCCESS, 255};
    SDL_Color bp_color = {COLOR_BREAKPOINT, 255};
    
    int line_y = y + 35;
    uint32_t addr = display->disasm_offset;
    
    for (int i = 0; i < display->disasm_lines && addr < mem_size - 1; i++) {
        M68K_Disassembly dis;
        m68k_disassemble(cpu, memory, mem_size, addr, &dis);
        
        // Check for breakpoint
        bool has_bp = false;
        for (int j = 0; j < cpu->num_breakpoints; j++) {
            if (cpu->breakpoints[j].address == addr) {
                has_bp = true;
                break;
            }
        }
        
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%c %s 0x%08X: %04X  %-8s %s",
                has_bp ? '*' : ' ',
                (addr == cpu->pc) ? ">" : " ",
                addr, dis.opcode, dis.mnemonic, dis.operands);
        
        SDL_Color color = (addr == cpu->pc) ? pc_color : 
                         has_bp ? bp_color : text_color;
        draw_text(display, buffer, x + 15, line_y, color, display->font_mono);
        
        line_y += 18;
        addr = dis.next_pc;
    }
}

void draw_peripheral_status(M68K_Display* display, M68K_Peripherals* peripherals,
                           int x, int y) {
    SDL_Color text_color = {COLOR_TEXT, 255};
    SDL_Color active_color = {COLOR_SUCCESS, 255};
    
    int line_y = y;
    char buffer[128];
    
    // NVMe Status
    draw_panel(display, x, line_y, 450, 180, "NVMe M.2 SSD");
    line_y += 35;
    
    snprintf(buffer, sizeof(buffer), "Capacity: %llu GB", 
            peripherals->nvme.capacity / (1024*1024*1024));
    draw_text(display, buffer, x + 15, line_y, text_color, display->font_small);
    line_y += 20;
    
    snprintf(buffer, sizeof(buffer), "Reads: %llu  Writes: %llu",
            peripherals->nvme.reads, peripherals->nvme.writes);
    draw_text(display, buffer, x + 15, line_y, text_color, display->font_small);
    line_y += 20;
    
    snprintf(buffer, sizeof(buffer), "Data Read: %llu MB  Written: %llu MB",
            peripherals->nvme.bytes_read / (1024*1024),
            peripherals->nvme.bytes_written / (1024*1024));
    draw_text(display, buffer, x + 15, line_y, text_color, display->font_small);
    line_y += 20;
    
    snprintf(buffer, sizeof(buffer), "Status: %s",
            peripherals->nvme.initialized ? "Ready" : "Not initialized");
    draw_text(display, buffer, x + 15, line_y, 
             peripherals->nvme.initialized ? active_color : text_color, 
             display->font_small);
    line_y += 40;
    
    // Ethernet Status
    draw_panel(display, x, line_y, 450, 180, "2.5GbE Ethernet");
    line_y += 35;
    
    snprintf(buffer, sizeof(buffer), "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
            peripherals->ethernet.mac_address[0], peripherals->ethernet.mac_address[1],
            peripherals->ethernet.mac_address[2], peripherals->ethernet.mac_address[3],
            peripherals->ethernet.mac_address[4], peripherals->ethernet.mac_address[5]);
    draw_text(display, buffer, x + 15, line_y, text_color, display->font_small);
    line_y += 20;
    
    snprintf(buffer, sizeof(buffer), "Link: %s @ %u Mbps",
            peripherals->ethernet.link_up ? "Up" : "Down",
            peripherals->ethernet.link_speed);
    draw_text(display, buffer, x + 15, line_y, 
             peripherals->ethernet.link_up ? active_color : text_color,
             display->font_small);
    line_y += 20;
    
    snprintf(buffer, sizeof(buffer), "TX: %llu packets  RX: %llu packets",
            peripherals->ethernet.packets_sent, peripherals->ethernet.packets_received);
    draw_text(display, buffer, x + 15, line_y, text_color, display->font_small);
    line_y += 20;
    
    snprintf(buffer, sizeof(buffer), "Bandwidth: TX %llu MB  RX %llu MB",
            peripherals->ethernet.bytes_sent / (1024*1024),
            peripherals->ethernet.bytes_received / (1024*1024));
    draw_text(display, buffer, x + 15, line_y, text_color, display->font_small);
    line_y += 40;
    
    // GPIO Status
    draw_panel(display, x, line_y, 450, 120, "GPIO (64 pins)");
    line_y += 35;
    
    int pins_high = 0;
    for (int i = 0; i < GPIO_PINS; i++) {
        if (peripherals->gpio.state[i]) pins_high++;
    }
    
    snprintf(buffer, sizeof(buffer), "Pins High: %d / %d", pins_high, GPIO_PINS);
    draw_text(display, buffer, x + 15, line_y, text_color, display->font_small);
    line_y += 20;
    
    // Show first 16 pins state
    snprintf(buffer, sizeof(buffer), "Pin[0-15]: ");
    for (int i = 0; i < 16; i++) {
        strcat(buffer, peripherals->gpio.state[i] ? "1" : "0");
    }
    draw_text(display, buffer, x + 15, line_y, text_color, display->font_mono);
}

void draw_performance_tab(M68K_Display* display, M68K_CPU* cpu) {
    draw_panel(display, 50, 100, 1820, 900, "Performance Metrics");
    
    SDL_Color text_color = {COLOR_TEXT, 255};
    SDL_Color highlight_color = {COLOR_HIGHLIGHT, 255};
    
    int col1_x = 100;
    int col2_x = 800;
    int line_y = 150;
    char buffer[128];
    
    // CPU Performance
    snprintf(buffer, sizeof(buffer), "Instructions Executed: %llu", 
            (unsigned long long)cpu->instruction_count);
    draw_text(display, buffer, col1_x, line_y, text_color, display->font_normal);
    line_y += 30;
    
    snprintf(buffer, sizeof(buffer), "CPU Cycles: %llu", 
            (unsigned long long)cpu->cycle_count);
    draw_text(display, buffer, col1_x, line_y, text_color, display->font_normal);
    line_y += 30;
    
    snprintf(buffer, sizeof(buffer), "Performance: %.2f MIPS @ %.2f MHz", 
            display->mips, display->mhz);
    draw_text(display, buffer, col1_x, line_y, highlight_color, display->font_normal);
    line_y += 40;
    
    // Branch Statistics
    snprintf(buffer, sizeof(buffer), "Branches: %llu  Taken: %llu (%.1f%%)",
            (unsigned long long)cpu->branch_count,
            (unsigned long long)cpu->branch_taken,
            cpu->branch_count > 0 ? 
                (100.0 * cpu->branch_taken / cpu->branch_count) : 0.0);
    draw_text(display, buffer, col1_x, line_y, text_color, display->font_normal);
    line_y += 30;
    
    // Memory Statistics
    snprintf(buffer, sizeof(buffer), "Loads: %llu  Stores: %llu",
            (unsigned long long)cpu->load_count,
            (unsigned long long)cpu->store_count);
    draw_text(display, buffer, col1_x, line_y, text_color, display->font_normal);
    line_y += 30;
    
    snprintf(buffer, sizeof(buffer), "Bus Transfers: %u (1024-bit bursts)",
            cpu->bus_transfers);
    draw_text(display, buffer, col1_x, line_y, text_color, display->font_normal);
    line_y += 40;
    
    // Cache Statistics
    line_y = 150;
    snprintf(buffer, sizeof(buffer), "Cache Hits: %llu",
            (unsigned long long)cpu->cache_hits);
    draw_text(display, buffer, col2_x, line_y, text_color, display->font_normal);
    line_y += 30;
    
    snprintf(buffer, sizeof(buffer), "Cache Misses: %llu",
            (unsigned long long)cpu->cache_misses);
    draw_text(display, buffer, col2_x, line_y, text_color, display->font_normal);
    line_y += 30;
    
    float hit_rate = (cpu->cache_hits + cpu->cache_misses) > 0 ?
        (100.0f * cpu->cache_hits / (cpu->cache_hits + cpu->cache_misses)) : 0.0f;
    snprintf(buffer, sizeof(buffer), "Hit Rate: %.2f%%", hit_rate);
    draw_text(display, buffer, col2_x, line_y, highlight_color, display->font_normal);
    line_y += 40;
    
    // Apollo 68080 Status
    snprintf(buffer, sizeof(buffer), "Apollo 68080 Mode: %s",
            cpu->apollo_mode ? "Enabled" : "Disabled");
    draw_text(display, buffer, col2_x, line_y, 
             cpu->apollo_mode ? highlight_color : text_color, display->font_normal);
}

// This will be continued in the next part...
