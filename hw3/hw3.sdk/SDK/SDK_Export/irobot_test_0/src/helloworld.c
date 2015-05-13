/*
 * Copyright (c) 2009-2012 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include <unistd.h>
#include "platform.h"
#include <xuartps.h>

// A convenient struct to bundle xpsuart information.
typedef struct {
    int id;
    int baud_rate;
    XUartPs_Config *config;
    XUartPs device;
} uart_t;

int main()
{
    init_platform();

    printf("initialize irobot");

    int status;

    // Configure the serial settings to 57600 baud, 8 data bits, 1 stop bit,
    // and no flow control.
    uart_t uart0 = {
        .id = XPAR_PS7_UART_0_DEVICE_ID,
        .baud_rate = 57600,
        .config = 0,
    };
    printf("uart0 lookup config\n");
    uart0.config = XUartPs_LookupConfig(uart0.id);
    if (!uart0.config) {
        printf("XUartPs_LookupConfig failed for %d\n", uart0.id);
        return 1;
    }
    printf("uart0 config initialize\n");
    status = XUartPs_CfgInitialize(&uart0.device, uart0.config,
            uart0.config->BaseAddress);
    if (status) {
        printf("XUartPs_CfgInitialize failed %d\n", status);
        return status;
    }
    printf("uart0 set baud rate\n");
    status = XUartPs_SetBaudRate(&uart0.device, uart0.baud_rate);
    if (status) {
        printf("XUartPs_SetBaudRate %d %d\n", uart0.baud_rate, status);
        return status;
    }

    printf("uart0 issuing play commands\n");
    const u8 cmd_mode_full[] = {128,132}; // (Puts the robot in Full mode)
    const u8 cmd_song_program[] = {140,0,4,62,12,66,12,69,12,74,36}; // (Defines the song)
    const u8 cmd_song_play[] = {141,0}; // (Plays the song)

    // Note: this prefers XUartPs_SendByte over XUartPs_Send because the latter
    // tries to coexist with interrupts -- an unnecessary operation given the
    // current configuration.
    int i;
    printf("uart0 mode full\n");
    for (i = 0; i < sizeof(cmd_mode_full); ++i) {
        XUartPs_SendByte(uart0.config->BaseAddress, cmd_mode_full[i]);
    }
    printf("uart0 song program\n");
    for (i = 0; i < sizeof(cmd_song_program); ++i) {
        XUartPs_SendByte(uart0.config->BaseAddress, cmd_song_program[i]);
    }

    printf("press enter to play song\n");
    getchar();

    // Play the song many times for video sake.
    // In mother russia, video plays YOU!
    int j;
    for (j = 0; j < 3; ++j) {
    printf("uart0 song play\n");
    for (i = 0; i < sizeof(cmd_song_play); ++i) {
        XUartPs_SendByte(uart0.config->BaseAddress, cmd_song_play[i]);
    }

    // We must delay for the duration of the song. Commands sent while other
    // commands are being processed have no effect.
    usleep(3 * 500 * 1000);
    }

    return 0;
}
