// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#ifndef _uart_h_
#define _uart_h_

#include <xuartps.h>

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
