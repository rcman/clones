#include "m68k_display.h"
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
        "68000 Simulator - 1024-bit Bus Edition",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (!display->window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return false;
    }
    
    display->renderer = SDL_CreateRenderer(display->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!display->renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return false;
    }
    
    // Try to load a monospace font (common system fonts)
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
        if (display->font_normal && display->font_small) {
            font_loaded = true;
            break;
        }
    }
    
    if (!font_loaded) {
        printf("Warning: Could not load font. Text rendering will be disabled.\n");
    }
    
    display->running = true;
    display->paused = true;
    display->step_mode = false;
    display->mem_view_offset = 0;
    display->mem_view_width = 16;
    display->mem_view_height = 16;
    
    return true;
}

void m68k_display_cleanup(M68K_Display* display) {
    if (display->font_normal) TTF_CloseFont(display->font_normal);
    if (display->font_small) TTF_CloseFont(display->font_small);
    if (display->renderer) SDL_DestroyRenderer(display->renderer);
    if (display->window) SDL_DestroyWindow(display->window);
    
    TTF_Quit();
    SDL_Quit();
}

void draw_text(M68K_Display* display, const char* text, int x, int y, SDL_Color color, TTF_Font* font) {
    if (!font) return;
    
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

void draw_registers(M68K_Display* display, M68K_CPU* cpu, int x, int y) {
    draw_panel(display, x, y, 300, 450, "CPU Registers");
    
    SDL_Color text_color = {COLOR_TEXT, 255};
    SDL_Color active_color = {COLOR_SUCCESS, 255};
    
    int line_y = y + 35;
    int line_height = 20;
    char buffer[64];
    
    // Data registers
    for (int i = 0; i < 8; i++) {
        snprintf(buffer, sizeof(buffer), "D%d: 0x%08X  (%u)", i, cpu->d[i], cpu->d[i]);
        draw_text(display, buffer, x + 15, line_y, text_color, display->font_small);
        line_y += line_height;
    }
    
    line_y += 10;
    
    // Address registers
    for (int i = 0; i < 8; i++) {
        const char* label = (i == 7) ? "A7/SP" : "A%d";
        snprintf(buffer, sizeof(buffer), label, i);
        char full_buffer[64];
        snprintf(full_buffer, sizeof(full_buffer), "%s: 0x%08X", buffer, cpu->a[i]);
        draw_text(display, full_buffer, x + 15, line_y, 
                 (i == 7) ? active_color : text_color, display->font_small);
        line_y += line_height;
    }
    
    line_y += 10;
    
    // Program Counter
    snprintf(buffer, sizeof(buffer), "PC: 0x%08X", cpu->pc);
    draw_text(display, buffer, x + 15, line_y, active_color, display->font_small);
    line_y += line_height;
    
    // Status Register
    snprintf(buffer, sizeof(buffer), "SR: 0x%04X [%c%c%c%c%c]",
            cpu->sr,
            m68k_get_flag(cpu, SR_CARRY) ? 'C' : '-',
            m68k_get_flag(cpu, SR_OVERFLOW) ? 'V' : '-',
            m68k_get_flag(cpu, SR_ZERO) ? 'Z' : '-',
            m68k_get_flag(cpu, SR_NEGATIVE) ? 'N' : '-',
            m68k_get_flag(cpu, SR_EXTEND) ? 'X' : '-');
    draw_text(display, buffer, x + 15, line_y, text_color, display->font_small);
}

void draw_memory_view(M68K_Display* display, uint8_t* memory, size_t mem_size, int x, int y) {
    draw_panel(display, x, y, 550, 450, "Memory View (1024-bit Bus)");
    
    SDL_Color text_color = {COLOR_TEXT, 255};
    SDL_Color addr_color = {COLOR_HIGHLIGHT, 255};
    
    int line_y = y + 35;
    int line_height = 18;
    char buffer[128];
    
    for (int row = 0; row < display->mem_view_height; row++) {
        uint32_t addr = display->mem_view_offset + (row * display->mem_view_width);
        
        if (addr >= mem_size) break;
        
        // Address
        snprintf(buffer, sizeof(buffer), "%08X:", addr);
        draw_text(display, buffer, x + 15, line_y, addr_color, display->font_small);
        
        // Hex bytes
        char hex_part[64] = "";
        char ascii_part[20] = "";
        
        for (int col = 0; col < display->mem_view_width && (addr + col) < mem_size; col++) {
            uint8_t byte = memory[addr + col];
            char hex_byte[4];
            snprintf(hex_byte, sizeof(hex_byte), "%02X ", byte);
            strcat(hex_part, hex_byte);
            
            // ASCII representation
            char ascii_char[2];
            ascii_char[0] = (byte >= 32 && byte < 127) ? byte : '.';
            ascii_char[1] = '\0';
            strcat(ascii_part, ascii_char);
        }
        
        draw_text(display, hex_part, x + 100, line_y, text_color, display->font_small);
        draw_text(display, ascii_part, x + 450, line_y, text_color, display->font_small);
        
        line_y += line_height;
    }
    
    // Instructions
    draw_text(display, "UP/DOWN: Scroll | PgUp/PgDn: Fast scroll", 
             x + 15, y + 425, text_color, display->font_small);
}

void draw_bus_activity(M68K_Display* display, M68K_CPU* cpu, int x, int y) {
    draw_panel(display, x, y, 550, 200, "1024-bit Bus Activity");
    
    SDL_Color text_color = {COLOR_TEXT, 255};
    SDL_Color active_color = {COLOR_ACTIVE, 255};
    SDL_Color success_color = {COLOR_SUCCESS, 255};
    
    int line_y = y + 35;
    char buffer[128];
    
    // Bus status
    snprintf(buffer, sizeof(buffer), "Bus Active: %s", cpu->bus_active ? "YES" : "NO");
    draw_text(display, buffer, x + 15, line_y, 
             cpu->bus_active ? active_color : text_color, display->font_small);
    line_y += 25;
    
    // Current address being accessed
    snprintf(buffer, sizeof(buffer), "Bus Address: 0x%08X (128-byte aligned)", cpu->bus_address);
    draw_text(display, buffer, x + 15, line_y, text_color, display->font_small);
    line_y += 25;
    
    // Show first 32 bytes of bus data (representing the 1024-bit transfer)
    draw_text(display, "Bus Data (first 32 bytes of 128-byte burst):", x + 15, line_y, text_color, display->font_small);
    line_y += 20;
    
    for (int row = 0; row < 2; row++) {
        char hex_line[128] = "";
        for (int col = 0; col < 16; col++) {
            char hex_byte[4];
            snprintf(hex_byte, sizeof(hex_byte), "%02X ", cpu->bus_data[row * 16 + col]);
            strcat(hex_line, hex_byte);
        }
        draw_text(display, hex_line, x + 25, line_y, success_color, display->font_small);
        line_y += 18;
    }
    
    line_y += 10;
    
    snprintf(buffer, sizeof(buffer), "Bandwidth: ~100 GB/s (1024 bits × CPU clock)");
    draw_text(display, buffer, x + 15, line_y, text_color, display->font_small);
}

void draw_status_bar(M68K_Display* display, M68K_CPU* cpu, int x, int y) {
    SDL_Color bg_color = {COLOR_PANEL, 255};
    SDL_Color text_color = {COLOR_TEXT, 255};
    SDL_Color active_color = {COLOR_SUCCESS, 255};
    SDL_Color paused_color = {COLOR_ACTIVE, 255};
    
    draw_rect_filled(display, x, y, WINDOW_WIDTH, 60, bg_color);
    
    char buffer[256];
    
    // Status
    const char* status = display->paused ? "PAUSED" : "RUNNING";
    SDL_Color status_color = display->paused ? paused_color : active_color;
    draw_text(display, status, x + 15, y + 10, status_color, display->font_normal);
    
    // Cycle count
    snprintf(buffer, sizeof(buffer), "Cycles: %llu", (unsigned long long)cpu->cycle_count);
    draw_text(display, buffer, x + 15, y + 35, text_color, display->font_small);
    
    // Instructions
    const char* instructions = "SPACE: Run/Pause | S: Step | R: Reset | ESC: Quit";
    draw_text(display, instructions, x + 200, y + 20, text_color, display->font_small);
    
    // Halted indicator
    if (cpu->halted) {
        draw_text(display, "HALTED", x + WINDOW_WIDTH - 120, y + 20, paused_color, display->font_normal);
    }
}

void m68k_display_render(M68K_Display* display, M68K_CPU* cpu, uint8_t* memory, 
                        size_t mem_size, M68K_Peripherals* peripherals) {
    (void)peripherals;  // Not used in basic display mode
    
    // Clear screen
    SDL_Color bg_color = {COLOR_BG, 255};
    SDL_SetRenderDrawColor(display->renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderClear(display->renderer);
    
    // Draw panels
    draw_registers(display, cpu, 10, 10);
    draw_memory_view(display, memory, mem_size, 320, 10);
    draw_bus_activity(display, cpu, 320, 470);
    draw_status_bar(display, cpu, 0, WINDOW_HEIGHT - 60);
    
    // Present
    SDL_RenderPresent(display->renderer);
}

bool m68k_display_handle_events(M68K_Display* display, M68K_CPU* cpu) {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                display->running = false;
                return false;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        display->running = false;
                        return false;
                        
                    case SDLK_SPACE:
                        display->paused = !display->paused;
                        break;
                        
                    case SDLK_s:
                        display->step_mode = true;
                        break;
                        
                    case SDLK_r:
                        m68k_reset(cpu);
                        break;
                        
                    case SDLK_UP:
                        if (display->mem_view_offset >= 16) {
                            display->mem_view_offset -= 16;
                        }
                        break;
                        
                    case SDLK_DOWN:
                        display->mem_view_offset += 16;
                        break;
                        
                    case SDLK_PAGEUP:
                        if (display->mem_view_offset >= 256) {
                            display->mem_view_offset -= 256;
                        } else {
                            display->mem_view_offset = 0;
                        }
                        break;
                        
                    case SDLK_PAGEDOWN:
                        display->mem_view_offset += 256;
                        break;
                }
                break;
        }
    }
    
    return true;
}
