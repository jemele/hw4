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

    printf("uart0 mode full\n");
    const u8 cmd_mode_full[] = {128,132};
    uart_sendv(&uart0, cmd_mode_full, sizeof(cmd_mode_full));

    printf("uart0 song program\n");
    const u8 cmd_song_program[] = {140,0,4,62,12,66,12,69,12,74,36};
    uart_sendv(&uart0, cmd_song_program, sizeof(cmd_song_program));

    // Do a movement demo.
    s16 speed = 0;
    const s16 speed_max = 30;
    const s16 speed_min = -speed_max;
    const s16 speed_delta = 5;
    u32 buttons;
    for (;;) {
        // XXX This interface sucks.
        // It would be much nicer to have something that samples until non-zero,
        // and then samples until zero again.
        buttons = gpio_blocking_read(&gpio);
        if (button_center_pressed(buttons)) {
            printf("goodbye!\n");
            break;
        }
        // Rate limit command to 4Hz.
        usleep(250 * 1000);

        // Process combo buttons first.
        // both left and right at the same time indicate that we should go back
        // to safe mode.
        if (button_right_pressed(buttons) &&
                button_left_pressed(buttons)) {
            printf("safe mode\n");
            const u8 c[] = {128,131};
            uart_sendv(&uart0,c,sizeof(c));
            continue;
        }
        if (button_up_pressed(buttons) &&
                button_down_pressed(buttons)) {
            printf("play song\n");
            const u8 c[] = {141,0};
            uart_sendv(&uart0,c,sizeof(c));
            continue;
        }

        // Process single buttons next.
        // Forward backwards control direct movement forware and backwards.
        if (button_up_pressed(buttons)) {
            speed -= speed_delta;
            if (speed < speed_min) {
                speed = speed_min;
            }
            printf("forward %d mm/s\n", speed);
            const u8 c[] = {137,(speed>>8)&0xff,speed&0xff,0x80,0};
            uart_sendv(&uart0,c,sizeof(c));
            continue;
        }
        if (button_down_pressed(buttons)) {
            speed += speed_delta;
            if (speed > speed_max) {
                speed = speed_max;
            }
            printf("forward %d mm/s\n", speed);
            const u8 c[] = {137,(speed>>8)&0xff,speed&0xff,0x80,0};
            uart_sendv(&uart0,c,sizeof(c));
            continue;
        }

        // Left and run control in place turning.
        if (button_left_pressed(buttons)) {
            printf("left\n");
            printf("ccw %d mm/s\n", speed);
            const u8 c[] = {137,(speed>>8)&0xff,speed&0xff,0,1};
            uart_sendv(&uart0,c,sizeof(c));
            continue;
        }
        if (button_right_pressed(buttons)) {
            printf("right\n");
            printf("cw %d mm/s\n", speed);
            const u8 c[] = {137,(speed>>8)&0xff,speed&0xff,0xff,0xff};
            uart_sendv(&uart0,c,sizeof(c));
            continue;
        }
    }

    return 0;
}
