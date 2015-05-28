#include <stdio.h>
#include <unistd.h>
#include "platform.h"
#include "uart.h"

// Initialize the specified uart.
int uart_axi_initialize(uart_axi_t *uart)
{
    int status;

    uart->config = XUartNs550_LookupConfig(uart->id);
    if (!uart->config) {
        printf("XUartNs550_LookupConfig failed\n");
        return XST_FAILURE;
    }

    status = XUartNs550_CfgInitialize(&uart->device, uart->config,
            uart->config->BaseAddress);
    if (status) {
        printf("XUartNs550_CfgInitialize failed %d\n", status);
        return status;
    }

    XUartNs550_SetBaud(uart->device.BaseAddress, uart->device.InputClockHz,
            uart->baud_rate);

    status = XUartNs550_SelfTest(&uart->device);
    if (status) {
        printf("XUartNs550_SelfTest failed %d\n", status);
        return status;
    }

    return 0;
}

u8 uart_axi_recv(uart_axi_t *uart)
{
    return XUartNs550_RecvByte(uart->device.BaseAddress) & 0xff;
}

void uart_axi_send(uart_axi_t *uart, const u8 c)
{
    XUartNs550_SendByte(uart->device.BaseAddress, c);
}

void uart_axi_sendv(uart_axi_t *uart, const u8 *data, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        uart_axi_send(uart, data[i]);
    }
}

int uart_axi_recv_ready(uart_axi_t *uart)
{
    return XUartNs550_IsReceiveData(uart->device.BaseAddress);
}

int uart_axi_recv_flush(uart_axi_t *uart)
{
    int i = 0;
    while (uart_axi_recv_ready(uart)) {
        uart_axi_recv(uart);
        ++i;
    }
    return i;
}

// Initialize the specified uart.
int uart_initialize(uart_t *uart)
{
    int status;

    printf("uart lookup config\n");
    uart->config = XUartPs_LookupConfig(uart->id);
    if (!uart->config) {
        printf("XUartPs_LookupConfig failed for %d\n", uart->id);
        return 1;
    }
    printf("uart config initialize\n");
    status = XUartPs_CfgInitialize(&uart->device, uart->config,
            uart->config->BaseAddress);
    if (status) {
        printf("XUartPs_CfgInitialize failed %d\n", status);
        return status;
    }
    printf("uart set baud rate\n");
    status = XUartPs_SetBaudRate(&uart->device, uart->baud_rate);
    if (status) {
        printf("XUartPs_SetBaudRate %d %d\n", uart->baud_rate, status);
        return status;
    }
    return 0;
}


u8 uart_recv(uart_t *uart)
{
    return XUartPs_RecvByte(uart->config->BaseAddress);
}

// Send the specified data.
void uart_send(uart_t *uart, const u8 data)
{
    XUartPs_SendByte(uart->config->BaseAddress, data);
}

// Send the specified data.
void uart_sendv(uart_t *uart, const u8 *data, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        uart_send(uart, data[i]);
    }
}

// Returns non-zero if recv data is waiting, 0 otherwise.
int uart_recv_ready(uart_t *uart)
{
    return XUartPs_IsReceiveData(uart->config->BaseAddress);
}

// Flush the uart receive buffer.
// Returns the number of bytes flushed.
int uart_recv_flush(uart_t *uart)
{
    int i = 0;
    while (uart_recv_ready(uart)) {
        uart_recv(uart);
        ++i;
    }
    return i;
}


