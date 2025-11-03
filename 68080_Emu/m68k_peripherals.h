#ifndef M68K_PERIPHERALS_H
#define M68K_PERIPHERALS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// M.2 NVMe Simulation
#define NVME_BLOCK_SIZE 4096
#define NVME_MAX_BLOCKS 262144  // 1GB drive
#define NVME_QUEUE_SIZE 64

typedef enum {
    NVME_CMD_READ = 0x02,
    NVME_CMD_WRITE = 0x01,
    NVME_CMD_FLUSH = 0x00,
    NVME_CMD_IDENTIFY = 0x06,
} NVMe_Command;

typedef struct {
    uint32_t lba;           // Logical Block Address
    uint16_t num_blocks;
    uint8_t command;
    uint8_t status;
    uint32_t buffer_addr;
    bool completed;
} NVMe_Request;

typedef struct {
    FILE* storage_file;
    char filename[256];
    uint64_t capacity;      // In bytes
    uint32_t block_count;
    
    NVMe_Request queue[NVME_QUEUE_SIZE];
    int queue_head;
    int queue_tail;
    
    uint64_t reads;
    uint64_t writes;
    uint64_t bytes_read;
    uint64_t bytes_written;
    
    bool initialized;
} M68K_NVMe;

// 2.5GbE Network Simulation
#define ETH_MTU 1500
#define ETH_QUEUE_SIZE 32

typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
    uint8_t data[ETH_MTU];
    uint16_t length;
    uint32_t timestamp;
} Ethernet_Packet;

typedef struct {
    uint8_t mac_address[6];
    uint32_t ip_address;
    
    Ethernet_Packet rx_queue[ETH_QUEUE_SIZE];
    int rx_head;
    int rx_tail;
    
    Ethernet_Packet tx_queue[ETH_QUEUE_SIZE];
    int tx_head;
    int tx_tail;
    
    uint64_t packets_sent;
    uint64_t packets_received;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    
    bool link_up;
    uint32_t link_speed;    // Mbps (2500 for 2.5GbE)
} M68K_Ethernet;

// HDMI Framebuffer
#define HDMI_WIDTH 1920
#define HDMI_HEIGHT 1080
#define HDMI_BPP 4  // 32-bit RGBA

typedef struct {
    uint32_t* framebuffer;  // RGBA pixels
    uint32_t width;
    uint32_t height;
    uint32_t pitch;         // Bytes per row
    
    uint32_t fb_base_addr;  // Memory-mapped address
    uint32_t fb_size;
    
    bool vsync;
    uint32_t frame_count;
    
    // Display timing
    uint32_t refresh_rate;  // Hz
    bool enabled;
} M68K_HDMI;

// GPIO Pins
#define GPIO_PINS 64

typedef enum {
    GPIO_INPUT,
    GPIO_OUTPUT,
    GPIO_ALTERNATE,
} GPIO_Mode;

typedef struct {
    GPIO_Mode mode[GPIO_PINS];
    bool state[GPIO_PINS];      // Current pin state
    bool output[GPIO_PINS];     // Output register
    uint32_t pull_up;           // Pull-up bitmap
    uint32_t pull_down;         // Pull-down bitmap
    
    // Interrupt configuration
    bool irq_enabled[GPIO_PINS];
    bool irq_pending[GPIO_PINS];
    uint8_t irq_mode[GPIO_PINS]; // 0=rising, 1=falling, 2=both
} M68K_GPIO;

// Combined peripheral system
typedef struct {
    M68K_NVMe nvme;
    M68K_Ethernet ethernet;
    M68K_HDMI hdmi;
    M68K_GPIO gpio;
    
    // Memory-mapped I/O base addresses
    uint32_t nvme_base;
    uint32_t eth_base;
    uint32_t hdmi_base;
    uint32_t gpio_base;
} M68K_Peripherals;

// NVMe functions
void nvme_init(M68K_NVMe* nvme, const char* storage_file);
void nvme_cleanup(M68K_NVMe* nvme);
bool nvme_read(M68K_NVMe* nvme, uint32_t lba, uint16_t count, uint8_t* buffer);
bool nvme_write(M68K_NVMe* nvme, uint32_t lba, uint16_t count, const uint8_t* buffer);
void nvme_submit_command(M68K_NVMe* nvme, NVMe_Request* req);
bool nvme_poll_completion(M68K_NVMe* nvme, NVMe_Request* req);

// Ethernet functions
void eth_init(M68K_Ethernet* eth);
void eth_send_packet(M68K_Ethernet* eth, const Ethernet_Packet* packet);
bool eth_receive_packet(M68K_Ethernet* eth, Ethernet_Packet* packet);
void eth_set_mac(M68K_Ethernet* eth, const uint8_t mac[6]);
void eth_process(M68K_Ethernet* eth);  // Simulate network activity

// HDMI functions
void hdmi_init(M68K_HDMI* hdmi, uint32_t width, uint32_t height);
void hdmi_cleanup(M68K_HDMI* hdmi);
void hdmi_clear(M68K_HDMI* hdmi, uint32_t color);
void hdmi_set_pixel(M68K_HDMI* hdmi, uint32_t x, uint32_t y, uint32_t color);
uint32_t hdmi_get_pixel(M68K_HDMI* hdmi, uint32_t x, uint32_t y);
void hdmi_draw_rect(M68K_HDMI* hdmi, uint32_t x, uint32_t y, 
                    uint32_t w, uint32_t h, uint32_t color);
void hdmi_vsync(M68K_HDMI* hdmi);

// GPIO functions
void gpio_init(M68K_GPIO* gpio);
void gpio_set_mode(M68K_GPIO* gpio, int pin, GPIO_Mode mode);
void gpio_write(M68K_GPIO* gpio, int pin, bool value);
bool gpio_read(M68K_GPIO* gpio, int pin);
void gpio_set_pull(M68K_GPIO* gpio, int pin, bool pull_up, bool pull_down);
void gpio_enable_irq(M68K_GPIO* gpio, int pin, uint8_t mode);

// Peripheral system
void peripherals_init(M68K_Peripherals* peripherals);
void peripherals_cleanup(M68K_Peripherals* peripherals);
void peripherals_reset(M68K_Peripherals* peripherals);

// Memory-mapped I/O
uint32_t peripherals_read(M68K_Peripherals* peripherals, uint32_t address, int size);
void peripherals_write(M68K_Peripherals* peripherals, uint32_t address, 
                      uint32_t value, int size);

#endif // M68K_PERIPHERALS_H
