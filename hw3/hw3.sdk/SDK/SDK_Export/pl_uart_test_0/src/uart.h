// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#ifndef _uart_h_
#define _uart_h_

#include <xuartps.h>
#include <xuartns550.h>

// A convenient struct to bundle axi uart information.
typedef struct {
    int id;
    int baud_rate;
    XUartNs550_Config *config;
    XUartNs550 device;
} uart_axi_t;

// Initialize the specified device.
int uart_axi_initialize(uart_axi_t *uart);

// Return non-zero if receive data is available, zero otherwise.
int uart_axi_recv_ready(uart_axi_t *uart);

// Blocking receive on the specified device.
u8 uart_axi_recv(uart_axi_t *uart);

// Send a character to the device.
void uart_axi_send(uart_axi_t *uart, const u8 data);

// A convenient struct to bundle xpsuart information.
typedef struct {
    int id;
    int baud_rate;
    XUartPs_Config *config;
    XUartPs device;
} uart_t;

// Initialize the specified uart.
int uart_initialize(uart_t *uart);

// Receive a byte from the specified uart.
u8 uart_recv(uart_t *uart);

// Returns non-zero if recv data is waiting, 0 otherwise.
int uart_recv_ready(uart_t *uart);

// Flush the uart receive buffer.
// Returns the number of bytes flushed.
int uart_recv_flush(uart_t *uart);

// Send the specified data.
void uart_send(uart_t *uart, const u8 data);
void uart_sendv(uart_t *uart, const u8 *data, int count);

#endif
