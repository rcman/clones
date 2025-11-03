#include "m68k_peripherals.h"
#include <string.h>
#include <stdlib.h>

// ============================================================================
// NVMe M.2 Simulation
// ============================================================================

void nvme_init(M68K_NVMe* nvme, const char* storage_file) {
    memset(nvme, 0, sizeof(M68K_NVMe));
    
    strncpy(nvme->filename, storage_file, sizeof(nvme->filename) - 1);
    nvme->capacity = (uint64_t)NVME_MAX_BLOCKS * NVME_BLOCK_SIZE;
    nvme->block_count = NVME_MAX_BLOCKS;
    
    // Try to open existing file, create if doesn't exist
    nvme->storage_file = fopen(storage_file, "rb+");
    if (!nvme->storage_file) {
        nvme->storage_file = fopen(storage_file, "wb+");
        if (nvme->storage_file) {
            // Initialize with zeros
            uint8_t zero_block[NVME_BLOCK_SIZE] = {0};
            for (uint32_t i = 0; i < NVME_MAX_BLOCKS; i++) {
                fwrite(zero_block, 1, NVME_BLOCK_SIZE, nvme->storage_file);
            }
            fflush(nvme->storage_file);
            printf("Created new NVMe storage: %s (%llu MB)\n", 
                   storage_file, nvme->capacity / (1024*1024));
        }
    } else {
        printf("Opened existing NVMe storage: %s\n", storage_file);
    }
    
    nvme->initialized = (nvme->storage_file != NULL);
}

void nvme_cleanup(M68K_NVMe* nvme) {
    if (nvme->storage_file) {
        fclose(nvme->storage_file);
        nvme->storage_file = NULL;
    }
    printf("NVMe stats: %llu reads, %llu writes, %llu MB read, %llu MB written\n",
           nvme->reads, nvme->writes, 
           nvme->bytes_read / (1024*1024), nvme->bytes_written / (1024*1024));
}

bool nvme_read(M68K_NVMe* nvme, uint32_t lba, uint16_t count, uint8_t* buffer) {
    if (!nvme->initialized || lba + count > nvme->block_count) {
        return false;
    }
    
    fseek(nvme->storage_file, lba * NVME_BLOCK_SIZE, SEEK_SET);
    size_t bytes = fread(buffer, 1, count * NVME_BLOCK_SIZE, nvme->storage_file);
    
    nvme->reads++;
    nvme->bytes_read += bytes;
    
    return bytes == (count * NVME_BLOCK_SIZE);
}

bool nvme_write(M68K_NVMe* nvme, uint32_t lba, uint16_t count, const uint8_t* buffer) {
    if (!nvme->initialized || lba + count > nvme->block_count) {
        return false;
    }
    
    fseek(nvme->storage_file, lba * NVME_BLOCK_SIZE, SEEK_SET);
    size_t bytes = fwrite(buffer, 1, count * NVME_BLOCK_SIZE, nvme->storage_file);
    fflush(nvme->storage_file);
    
    nvme->writes++;
    nvme->bytes_written += bytes;
    
    return bytes == (count * NVME_BLOCK_SIZE);
}

void nvme_submit_command(M68K_NVMe* nvme, NVMe_Request* req) {
    int next_tail = (nvme->queue_tail + 1) % NVME_QUEUE_SIZE;
    if (next_tail != nvme->queue_head) {
        nvme->queue[nvme->queue_tail] = *req;
        nvme->queue[nvme->queue_tail].completed = false;
        nvme->queue_tail = next_tail;
    }
}

bool nvme_poll_completion(M68K_NVMe* nvme, NVMe_Request* req) {
    if (nvme->queue_head == nvme->queue_tail) {
        return false;
    }
    
    *req = nvme->queue[nvme->queue_head];
    if (req->completed) {
        nvme->queue_head = (nvme->queue_head + 1) % NVME_QUEUE_SIZE;
        return true;
    }
    
    return false;
}

// ============================================================================
// 2.5GbE Ethernet Simulation
// ============================================================================

void eth_init(M68K_Ethernet* eth) {
    memset(eth, 0, sizeof(M68K_Ethernet));
    
    // Default MAC address
    eth->mac_address[0] = 0x02;  // Locally administered
    eth->mac_address[1] = 0x68;  // '68' for 68000
    eth->mac_address[2] = 0x00;
    eth->mac_address[3] = 0x08;
    eth->mac_address[4] = 0x00;
    eth->mac_address[5] = 0x00;
    
    eth->link_up = true;
    eth->link_speed = 2500;  // 2.5 Gbps
    
    printf("Ethernet initialized: MAC %02X:%02X:%02X:%02X:%02X:%02X, 2.5GbE\n",
           eth->mac_address[0], eth->mac_address[1], eth->mac_address[2],
           eth->mac_address[3], eth->mac_address[4], eth->mac_address[5]);
}

void eth_send_packet(M68K_Ethernet* eth, const Ethernet_Packet* packet) {
    int next_tail = (eth->tx_tail + 1) % ETH_QUEUE_SIZE;
    if (next_tail != eth->tx_head) {
        eth->tx_queue[eth->tx_tail] = *packet;
        eth->tx_tail = next_tail;
        eth->packets_sent++;
        eth->bytes_sent += packet->length;
    }
}

bool eth_receive_packet(M68K_Ethernet* eth, Ethernet_Packet* packet) {
    if (eth->rx_head == eth->rx_tail) {
        return false;
    }
    
    *packet = eth->rx_queue[eth->rx_head];
    eth->rx_head = (eth->rx_head + 1) % ETH_QUEUE_SIZE;
    return true;
}

void eth_set_mac(M68K_Ethernet* eth, const uint8_t mac[6]) {
    memcpy(eth->mac_address, mac, 6);
}

void eth_process(M68K_Ethernet* eth) {
    // Simulate loopback for testing
    while (eth->tx_head != eth->tx_tail) {
        Ethernet_Packet* pkt = &eth->tx_queue[eth->tx_head];
        eth->tx_head = (eth->tx_head + 1) % ETH_QUEUE_SIZE;
        
        // Loopback if destination is our MAC
        if (memcmp(pkt->dest_mac, eth->mac_address, 6) == 0) {
            int next_tail = (eth->rx_tail + 1) % ETH_QUEUE_SIZE;
            if (next_tail != eth->rx_head) {
                eth->rx_queue[eth->rx_tail] = *pkt;
                eth->rx_tail = next_tail;
                eth->packets_received++;
                eth->bytes_received += pkt->length;
            }
        }
    }
}

// ============================================================================
// HDMI Framebuffer
// ============================================================================

void hdmi_init(M68K_HDMI* hdmi, uint32_t width, uint32_t height) {
    memset(hdmi, 0, sizeof(M68K_HDMI));
    
    hdmi->width = width;
    hdmi->height = height;
    hdmi->pitch = width * HDMI_BPP;
    hdmi->fb_size = width * height * HDMI_BPP;
    
    hdmi->framebuffer = (uint32_t*)calloc(width * height, sizeof(uint32_t));
    hdmi->refresh_rate = 60;
    hdmi->enabled = true;
    hdmi->fb_base_addr = 0x10000000;  // Memory-mapped at 256MB
    
    printf("HDMI framebuffer initialized: %ux%u @ %u Hz\n", 
           width, height, hdmi->refresh_rate);
}

void hdmi_cleanup(M68K_HDMI* hdmi) {
    if (hdmi->framebuffer) {
        free(hdmi->framebuffer);
        hdmi->framebuffer = NULL;
    }
    printf("HDMI rendered %u frames\n", hdmi->frame_count);
}

void hdmi_clear(M68K_HDMI* hdmi, uint32_t color) {
    for (uint32_t i = 0; i < hdmi->width * hdmi->height; i++) {
        hdmi->framebuffer[i] = color;
    }
}

void hdmi_set_pixel(M68K_HDMI* hdmi, uint32_t x, uint32_t y, uint32_t color) {
    if (x < hdmi->width && y < hdmi->height) {
        hdmi->framebuffer[y * hdmi->width + x] = color;
    }
}

uint32_t hdmi_get_pixel(M68K_HDMI* hdmi, uint32_t x, uint32_t y) {
    if (x < hdmi->width && y < hdmi->height) {
        return hdmi->framebuffer[y * hdmi->width + x];
    }
    return 0;
}

void hdmi_draw_rect(M68K_HDMI* hdmi, uint32_t x, uint32_t y, 
                    uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t dy = 0; dy < h; dy++) {
        for (uint32_t dx = 0; dx < w; dx++) {
            hdmi_set_pixel(hdmi, x + dx, y + dy, color);
        }
    }
}

void hdmi_vsync(M68K_HDMI* hdmi) {
    hdmi->vsync = true;
    hdmi->frame_count++;
}

// ============================================================================
// GPIO
// ============================================================================

void gpio_init(M68K_GPIO* gpio) {
    memset(gpio, 0, sizeof(M68K_GPIO));
    
    // Default all pins to input
    for (int i = 0; i < GPIO_PINS; i++) {
        gpio->mode[i] = GPIO_INPUT;
    }
    
    printf("GPIO initialized: %d pins available\n", GPIO_PINS);
}

void gpio_set_mode(M68K_GPIO* gpio, int pin, GPIO_Mode mode) {
    if (pin >= 0 && pin < GPIO_PINS) {
        gpio->mode[pin] = mode;
    }
}

void gpio_write(M68K_GPIO* gpio, int pin, bool value) {
    if (pin >= 0 && pin < GPIO_PINS && gpio->mode[pin] == GPIO_OUTPUT) {
        gpio->output[pin] = value;
        gpio->state[pin] = value;
    }
}

bool gpio_read(M68K_GPIO* gpio, int pin) {
    if (pin >= 0 && pin < GPIO_PINS) {
        return gpio->state[pin];
    }
    return false;
}

void gpio_set_pull(M68K_GPIO* gpio, int pin, bool pull_up, bool pull_down) {
    if (pin >= 0 && pin < GPIO_PINS) {
        if (pull_up) {
            gpio->pull_up |= (1 << (pin % 32));
        } else {
            gpio->pull_up &= ~(1 << (pin % 32));
        }
        
        if (pull_down) {
            gpio->pull_down |= (1 << (pin % 32));
        } else {
            gpio->pull_down &= ~(1 << (pin % 32));
        }
    }
}

void gpio_enable_irq(M68K_GPIO* gpio, int pin, uint8_t mode) {
    if (pin >= 0 && pin < GPIO_PINS) {
        gpio->irq_enabled[pin] = true;
        gpio->irq_mode[pin] = mode;
    }
}

// ============================================================================
// Peripheral System Integration
// ============================================================================

void peripherals_init(M68K_Peripherals* peripherals) {
    memset(peripherals, 0, sizeof(M68K_Peripherals));
    
    // Set memory-mapped I/O addresses
    peripherals->nvme_base = 0xF0000000;
    peripherals->eth_base  = 0xF1000000;
    peripherals->hdmi_base = 0x10000000;  // 256MB - framebuffer
    peripherals->gpio_base = 0xF2000000;
    
    nvme_init(&peripherals->nvme, "nvme_storage.img");
    eth_init(&peripherals->ethernet);
    hdmi_init(&peripherals->hdmi, 1920, 1080);
    gpio_init(&peripherals->gpio);
    
    printf("\nPeripheral System initialized:\n");
    printf("  NVMe  @ 0x%08X\n", peripherals->nvme_base);
    printf("  Ethernet @ 0x%08X\n", peripherals->eth_base);
    printf("  HDMI  @ 0x%08X\n", peripherals->hdmi_base);
    printf("  GPIO  @ 0x%08X\n", peripherals->gpio_base);
}

void peripherals_cleanup(M68K_Peripherals* peripherals) {
    nvme_cleanup(&peripherals->nvme);
    hdmi_cleanup(&peripherals->hdmi);
}

void peripherals_reset(M68K_Peripherals* peripherals) {
    gpio_init(&peripherals->gpio);
    hdmi_clear(&peripherals->hdmi, 0xFF000000);  // Black
}

uint32_t peripherals_read(M68K_Peripherals* peripherals, uint32_t address, int size) {
    // NVMe registers
    if (address >= peripherals->nvme_base && 
        address < peripherals->nvme_base + 0x1000) {
        // Simplified register access
        return 0;
    }
    
    // Ethernet registers
    if (address >= peripherals->eth_base && 
        address < peripherals->eth_base + 0x1000) {
        return 0;
    }
    
    // HDMI framebuffer
    if (address >= peripherals->hdmi_base && 
        address < peripherals->hdmi_base + peripherals->hdmi.fb_size) {
        uint32_t offset = (address - peripherals->hdmi_base) / 4;
        if (offset < peripherals->hdmi.width * peripherals->hdmi.height) {
            return peripherals->hdmi.framebuffer[offset];
        }
    }
    
    // GPIO registers
    if (address >= peripherals->gpio_base && 
        address < peripherals->gpio_base + 0x1000) {
        return 0;
    }
    
    return 0;
}

void peripherals_write(M68K_Peripherals* peripherals, uint32_t address, 
                      uint32_t value, int size) {
    // NVMe registers
    if (address >= peripherals->nvme_base && 
        address < peripherals->nvme_base + 0x1000) {
        return;
    }
    
    // Ethernet registers
    if (address >= peripherals->eth_base && 
        address < peripherals->eth_base + 0x1000) {
        return;
    }
    
    // HDMI framebuffer
    if (address >= peripherals->hdmi_base && 
        address < peripherals->hdmi_base + peripherals->hdmi.fb_size) {
        uint32_t offset = (address - peripherals->hdmi_base) / 4;
        if (offset < peripherals->hdmi.width * peripherals->hdmi.height) {
            peripherals->hdmi.framebuffer[offset] = value;
        }
    }
    
    // GPIO registers
    if (address >= peripherals->gpio_base && 
        address < peripherals->gpio_base + 0x1000) {
        return;
    }
}
