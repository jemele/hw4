// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include "platform.h"
#include <xuartns550.h>
#include <xuartps_hw.h>

#define DEBUG_UART_BASE_ADDRESS STDIN_BASEADDRESS

int DEBUG_getc_IsReady();

int DEBUG_getc_nowait()
{
    int r;
    if (DEBUG_getc_IsReady()) {
        r = XUartPs_ReadReg(DEBUG_UART_BASE_ADDRESS, XUARTPS_FIFO_OFFSET);
        r &= 0xff;
    } else {
        r = EOF;
    }
    return r;
}

int DEBUG_getc_IsReady()
{
    return XUartPs_IsReceiveData(DEBUG_UART_BASE_ADDRESS);
}

void pl_uart_tx(XUartNs550 *uart, u8 c)
{
    XUartNs550_SendByte(uart->BaseAddress, c);
}

int pl_uart_rx_nowait(XUartNs550 *uart)
{
    int c = XUartNs550_IsReceiveData(uart->BaseAddress);
    if (c) {
        c = XUartNs550_RecvByte(uart->BaseAddress);
        c &= 0xff;
    } else {
        c = EOF;
    }
    return c;
}

int main()
{
    init_platform();

    printf("initializing axi uart\n");

    int status;

    XUartNs550_Config *config = XUartNs550_LookupConfig(XPAR_UARTNS550_0_DEVICE_ID);
    if (!config) {
        printf("XUartNs550_LookupConfig failed\n");
        return XST_FAILURE;
    }

    XUartNs550 uart;
    status = XUartNs550_CfgInitialize(&uart, config, config->BaseAddress);
    if (status) {
        printf("XUartNs550_CfgInitialize failed %d\n", status);
        return status;
    }

    XUartNs550_SetBaud(uart.BaseAddress, uart.InputClockHz, 2*115200);

    status = XUartNs550_SelfTest(&uart);
    if (status) {
        printf("XUartNs550_SelfTest failed %d\n", status);
        return status;
    }

    // Pretty print tx/rx transitions.
    int tx = 0;
    for (;;) {

        // process local console input.
        int c = DEBUG_getc_nowait();
        if (EOF != c) {
            if (c == 'Q') {
                return 0;
            }
            if (!tx) {
                printf("--\npl-uart tx: ");
                tx = 1;
            }
            pl_uart_tx(&uart, c);
            putchar(c);
        }

        // process pl uart input.
        c = pl_uart_rx_nowait(&uart);
        if (c != EOF) {
            if (tx) {
                printf("--\npl-uart rx: ");
                tx = 0;
            }
            putchar(c);
        }
    }

    return 0;
}
