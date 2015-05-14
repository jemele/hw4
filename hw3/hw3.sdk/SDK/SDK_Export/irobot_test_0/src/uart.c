#include <stdio.h>
#include <unistd.h>
#include "platform.h"
#include "uart.h"

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


// Send the specified data.
void uart_send(uart_t *uart, const u8 *data, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        XUartPs_SendByte(uart->config->BaseAddress, data[i]);
    }
}
