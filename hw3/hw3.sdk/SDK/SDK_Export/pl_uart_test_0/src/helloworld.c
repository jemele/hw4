// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include <stdlib.h>
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

// Process console input.
static void process_console()
{
    if (!XUartPs_IsReceiveData(STDIN_BASEADDRESS)) {
        return;
    }
    const u8 c = XUartPs_ReadReg(STDIN_BASEADDRESS, XUARTPS_FIFO_OFFSET) & 0xff;
    putchar(c);
    switch (c) {
    case 'Q': printf("goodbye\n"); exit(0);
    }
}

// Process bbb input.
static void process_bbb(uart_axi_t *device)
{
    if (uart_axi_recv_ready(device)) {
        const u8 c = uart_axi_recv(device);
        putchar(c);
    }
}

// Process irobot tasks.
// XXX Add sensor poll task.
static void process_irobot(irobot_t *device)
{
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

    // Read and process messages from the serial until told to quit.
    for (;;) {

        // process console input.
        process_console();

        // process bbb input.
        process_bbb(&uart);

        // process irobot tasks.
        process_irobot(&irobot);
    }

    return 0;
}
