// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include "platform.h"
#include "irobot.h"
#include "uart.h"
#include "bbb.h"

int DEBUG_getc_IsReady()
{
    return XUartPs_IsReceiveData(STDIN_BASEADDRESS);
}

int DEBUG_getc_nowait()
{
    if (DEBUG_getc_IsReady()) {
        return XUartPs_ReadReg(STDIN_BASEADDRESS, XUARTPS_FIFO_OFFSET) & 0xff;
    }
    return EOF;
}

int main()
{
    init_platform();

    printf("initializing axi uart\n");

    int status;

    uart_axi_t uart = {
        .id = XPAR_UARTNS550_0_DEVICE_ID,
        .baud_rate = 230400,
    };
    status = uart_axi_initialize(&uart);
    if (status) {
        printf("uart_axi_initialize failed %d\n", status);
        return status;
    }

    // Configure the irobot serial device.
    irobot_t irobot = {
        .uart = {
            .id = XPAR_PS7_UART_0_DEVICE_ID,
            .baud_rate = 57600,
        },
    };
    status = irobot_initialize(&irobot);
    if (status) {
        printf("irobot_initialize failed %d\n", status);
        return status;
    }

    // Read a message from the serial.
    // Validate the message is well formed and process the message.
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
            uart_axi_send(&uart, c);
            putchar(c);
        }

        // process axi uart input.
        if (uart_axi_recv_ready(&uart)) {
            if (tx) {
                printf("--\npl-uart rx: ");
                tx = 0;
            }
            c = uart_axi_recv(&uart);
            putchar(c);
        }
    }

    return 0;
}
