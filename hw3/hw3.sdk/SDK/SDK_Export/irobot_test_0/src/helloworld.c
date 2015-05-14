// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include <unistd.h>
#include "platform.h"
#include "gpio.h"
#include "uart.h"

// Application driver.
int main()
{
    init_platform();

    printf("initialize irobot");

    int status;

    // Configure buttons. We'll use this to walk through a demonstration.
    // This also gives us a chance to vet the buttons interface more generally.
    gpio_t gpio = {
        .id = XPAR_AXI_GPIO_0_DEVICE_ID,
    };
    printf("initializing gpio\n");
    status = gpio_initialize(&gpio);
    if (status) {
        printf("gpio_initialize failed %d\n", status);
        return status;
    }
    printf("waiting for button press\n");
    gpio_blocking_read(&gpio);

    // Configure the serial settings to 57600 baud, 8 data bits, 1 stop bit,
    // and no flow control.
    uart_t uart0 = {
        .id = XPAR_PS7_UART_0_DEVICE_ID,
        .baud_rate = 57600,
        .config = 0,
    };
    status = uart_initialize(&uart0);
    if (status) {
        printf("uart_initialize failed %d\n", status);
        return status;
    }

    printf("uart0 issuing play commands\n");
    const u8 cmd_mode_full[] = {128,132}; // (Puts the robot in Full mode)
    const u8 cmd_song_program[] = {140,0,4,62,12,66,12,69,12,74,36};
    const u8 cmd_song_play[] = {141,0}; // (Plays the song)

    int i;
    printf("uart0 mode full\n");
    uart_send(&uart0, cmd_mode_full, sizeof(cmd_mode_full));
    printf("uart0 song program\n");
    uart_send(&uart0, cmd_song_program, sizeof(cmd_song_program));

    printf("waiting for button press to play song");
    gpio_blocking_read(&gpio);

#if 0
    // In mother russia, video plays YOU!
    printf("uart0 song play\n");
    uart_send(&uart0, cmd_song_play, sizeof(cmd_song_play));
#endif

    u32 buttons;
    for (;;) {
        printf("press center button to end, arrow keys to move\n");
        buttons = gpio_blocking_read(&gpio);
        if (button_center_pressed(buttons)) {
            printf("goodbye!\n");
            break;
        }
        if (button_up_pressed(buttons)) {
            printf("forward\n");
        }
        if (button_down_pressed(buttons)) {
            printf("backwards\n");
        }
        if (button_left_pressed(buttons)) {
            printf("left\n");
        }
        if (button_right_pressed(buttons)) {
            printf("right\n");
        }
    }

    return 0;
}
