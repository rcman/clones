#include "serial.h"
#include <stdlib.h>
#include <string.h>

/* Initialize serial interface */
serial_t* serial_init(void) {
    serial_t *serial = calloc(1, sizeof(serial_t));
    if (!serial) return NULL;
    
    serial->xcsr = XCSR_READY;  // Always ready to transmit
    serial->rcsr = 0;
    
    return serial;
}

/* Free serial resources */
void serial_free(serial_t *serial) {
    if (serial) {
        free(serial);
    }
}

/* Read serial register */
uint16_t serial_read_register(serial_t *serial, uint16_t addr) {
    switch (addr) {
        case RCSR:
            return serial->rcsr;
            
        case RBUF:
            // Reading RBUF clears DONE bit
            serial->rcsr &= ~RCSR_DONE;
            return serial->rbuf;
            
        case XCSR:
            return serial->xcsr;
            
        case XBUF:
            return 0;  // Write-only
            
        default:
            return 0;
    }
}

/* Write serial register */
void serial_write_register(serial_t *serial, uint16_t addr, uint16_t value) {
    switch (addr) {
        case RCSR:
            serial->rcsr = (serial->rcsr & ~RCSR_IE) | (value & RCSR_IE);
            break;
            
        case XCSR:
            serial->xcsr = (serial->xcsr & ~XCSR_IE) | (value & XCSR_IE);
            break;
            
        case XBUF:
            // Add to transmit buffer
            serial->tx_buffer[serial->tx_head] = value & 0x7F;
            serial->tx_head = (serial->tx_head + 1) % SERIAL_BUF_SIZE;
            break;
    }
}

/* Input a byte to the receiver (from external source like telnet) */
void serial_input_byte(serial_t *serial, uint8_t byte) {
    serial->rx_buffer[serial->rx_head] = byte;
    serial->rx_head = (serial->rx_head + 1) % SERIAL_BUF_SIZE;
}

/* Check if serial has output data */
bool serial_has_output(serial_t *serial) {
    return serial->tx_head != serial->tx_tail;
}

/* Read output byte */
uint8_t serial_read_output(serial_t *serial) {
    if (serial->tx_head == serial->tx_tail) {
        return 0;
    }
    
    uint8_t byte = serial->tx_buffer[serial->tx_tail];
    serial->tx_tail = (serial->tx_tail + 1) % SERIAL_BUF_SIZE;
    return byte;
}

/* Service serial interface (called periodically) */
void serial_service(serial_t *serial) {
    // Check if we have received data to present to CPU
    if (serial->rx_head != serial->rx_tail && !(serial->rcsr & RCSR_DONE)) {
        serial->rbuf = serial->rx_buffer[serial->rx_tail];
        serial->rx_tail = (serial->rx_tail + 1) % SERIAL_BUF_SIZE;
        serial->rcsr |= RCSR_DONE;
    }
    
    // Transmitter is always ready
    serial->xcsr |= XCSR_READY;
}
