#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdbool.h>

/* DL11 Serial interface registers */
#define RCSR  0177560  // Receiver Status Register
#define RBUF  0177562  // Receiver Buffer
#define XCSR  0177564  // Transmitter Status Register
#define XBUF  0177566  // Transmitter Buffer

/* Status register bits */
#define RCSR_DONE  0x0080  // Receiver done
#define RCSR_IE    0x0040  // Receiver interrupt enable
#define XCSR_READY 0x0080  // Transmitter ready
#define XCSR_IE    0x0040  // Transmitter interrupt enable

#define SERIAL_BUF_SIZE 4096

typedef struct {
    /* Receiver */
    uint16_t rcsr;
    uint16_t rbuf;
    uint8_t rx_buffer[SERIAL_BUF_SIZE];
    int rx_head;
    int rx_tail;
    
    /* Transmitter */
    uint16_t xcsr;
    uint8_t tx_buffer[SERIAL_BUF_SIZE];
    int tx_head;
    int tx_tail;
} serial_t;

/* Function prototypes */
serial_t* serial_init(void);
void serial_free(serial_t *serial);
uint16_t serial_read_register(serial_t *serial, uint16_t addr);
void serial_write_register(serial_t *serial, uint16_t addr, uint16_t value);
void serial_input_byte(serial_t *serial, uint8_t byte);
bool serial_has_output(serial_t *serial);
uint8_t serial_read_output(serial_t *serial);
void serial_service(serial_t *serial);

#endif // SERIAL_H
