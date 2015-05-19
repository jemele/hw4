// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include <unistd.h>
#include "platform.h"
#include "gpio.h"
#include "irobot.h"
#include "uart.h"
#include "ssd1306.h"
#include "font_5x7.h"
#include "Inspire.h"
#include "search.h"

// Practice movement primitives.
static void irobot_movement_demo(gpio_axi_t *gpio, uart_t *uart);

// Verify sensor input works for group 6 (all sensor data).
static void irobot_sensor_demo0(gpio_axi_t *gpio, uart_t *uart);

// Application driver.
int main()
{
    init_platform();

    printf("initialize irobot\n");

    int status, i;

    printf("search self test\n");
    search_map_t map;
    status = search_map_alloc(&map, 128/8, 64/8);
    if (status) {
        printf("search_map_alloc failed %d\n", status);
        return status;
    }
    search_map_initialize(&map);

    // Add some obstacles along the path.
    search_cell_at(&map,84/8,32/8)->blocked = 1;
    search_cell_at(&map,127/8,53/8)->blocked = 1;

    // Verify we can find our goal.
    search_cell_t *start = search_cell_at(&map,0,0);
    search_cell_t *goal = search_cell_at(&map,15,7);
    search_find(&map, start, goal);

    // Dump the path if we found the goal.
    if (goal->closed) {
        printf("goal found\n");
        search_cell_t *c;
        for (c = goal; c->parent; c = c->parent) {
            printf("%d,%d:%d\n", c->x, c->y, c->f);
        }
    }
    search_map_free(&map);

    // Configure buttons. We'll use this to walk through a demonstration.
    // This also gives us a chance to vet the buttons interface more generally.
    gpio_axi_t gpio_axi = {
        .id = XPAR_AXI_GPIO_0_DEVICE_ID,
    };
    printf("initializing gpio\n");
    status = gpio_axi_initialize(&gpio_axi);
    if (status) {
        printf("gpio_axi_initialize failed %d\n", status);
        return status;
    }
    gpio_t gpio = {
        .id = XPAR_PS7_GPIO_0_DEVICE_ID,
    };
    status = gpio_initialize(&gpio);
    if (status) {
        printf("gpio_initialize failed %d\n", status);
        return status;
    }

    // Configure i2c.
    i2c_t i2c0 = {
        .id = XPAR_PS7_I2C_0_DEVICE_ID,
        .clock = 100000,
        .clear_options = XIICPS_10_BIT_ADDR_OPTION,
        .options = XIICPS_7_BIT_ADDR_OPTION,
        .write_delay_us = 300,
    };
    status = i2c_initialize(&i2c0);
    if (status) {
        printf("i2c_initialize failed %d\n", status);
        return status;
    }
    i2c_t i2c1 = {
        .id = XPAR_PS7_I2C_1_DEVICE_ID,
        .clock = 100000,
        .clear_options = XIICPS_10_BIT_ADDR_OPTION,
        .options = XIICPS_7_BIT_ADDR_OPTION,
        .write_delay_us = 300,
    };
    status = i2c_initialize(&i2c1);
    if (status) {
        printf("i2c_initialize failed %d\n", status);
        return status;
    }

    // Configure displays.
    font_t font = {
        .base = Font5x7,
        .size = sizeof(Font5x7),
        .start = ' ',
        .width = 5,
        .pad = 1//28%5
    };
    ssd1306_t oled0 = {
        .addr = 0x3c,
        .reset_pin = 9,
        .i2c = &i2c0,
        .gpio = &gpio,
        .font = &font,
    };
    ssd1306_reset(&oled0);
    for(i=0; i< sizeof(Inspire); ++i) {
        i2c_data(oled0.i2c, oled0.addr, Inspire[i]);
    }

    ssd1306_t oled1 = {
        .addr = 0x3c,
        .reset_pin = 0,
        .i2c = &i2c1,
        .gpio = &gpio,
        .font = &font,
    };
    ssd1306_reset(&oled1);
    for(i=0; i < sizeof(Inspire); ++i) {
        i2c_data(oled1.i2c, oled0.addr, Inspire[i]);
    }

    // Configure the serial settings to 57600 baud, 8 data bits, 1 stop bit,
    // and no flow control.
    uart_t uart0 = {
        .id = XPAR_PS7_UART_0_DEVICE_ID,
        .baud_rate = 57600,
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

    irobot_movement_demo(&gpio_axi, &uart0);
    irobot_sensor_demo0(&gpio_axi, &uart0);

    return 0;
}

static void irobot_movement_demo(gpio_axi_t *gpio_axi, uart_t *uart)
{
    // Do a movement demo.
    printf("movement demo\n");
    const s16 unit_distance_mm = 26; // ~1 inch
    u32 buttons;
    for (;;) {
        // XXX This interface sucks.
        // It would be much nicer to have something that samples until non-zero,
        // and then samples until zero again.
        buttons = gpio_axi_blocking_read(gpio_axi);
        if (button_center_pressed(buttons)) {
            printf("goodbye!\n");
            break;
        }
        // Rate limit command input to 4Hz.
        usleep(250 * 1000);

        // Process combo buttons first.
        // both left and right at the same time indicate that we should go back
        // to safe mode.
        if (button_right_pressed(buttons) &&
                button_left_pressed(buttons)) {
            printf("safe mode\n");
            const u8 c[] = {128,131};
            uart_sendv(uart,c,sizeof(c));
            continue;
        }
        if (button_up_pressed(buttons) &&
                button_down_pressed(buttons)) {
            printf("play song\n");
            const u8 c[] = {141,0};
            uart_sendv(uart,c,sizeof(c));
            continue;
        }

        // Process single buttons next.
        // Forward backwards control direct movement forware and backwards.
        if (button_up_pressed(buttons)) {
            printf("backward\n");
            irobot_drive_straight(uart,-unit_distance_mm);
            continue;
        }
        if (button_down_pressed(buttons)) {
            printf("forward\n");
            irobot_drive_straight(uart,unit_distance_mm);
            continue;
        }

        // Left and run control in place turning.
        if (button_left_pressed(buttons)) {
            printf("left\n");
            irobot_rotate_left(uart);
            continue;
        }
        if (button_right_pressed(buttons)) {
            printf("right\n");
            irobot_rotate_right(uart);
            continue;
        }
    }
}

static void irobot_sensor_demo0(gpio_axi_t *gpio_axi, uart_t *uart)
{
    // Do a sensor demo.
    sleep(1);
    printf("sensor demo\n");
    u32 buttons;
    for (;;) {
        buttons = gpio_axi_blocking_read(gpio_axi);
        if (button_center_pressed(buttons)) {
            printf("goodbye!\n");
            break;
        }
        // Rate limit command input to 1Hz.
        sleep(1);
        printf("querying *all* sensor data\n");
        const u8 c[] = {142,6};
        uart_sendv(uart,c,sizeof(c));

        int i;
        u8 sensor_data[52];
        for (i = 0; i < sizeof(sensor_data); ++i) {
            sensor_data[i] = uart_recv(uart);
        }

        const int sensor_packet_to_offset[] = {
            [7]  0,
            [8]  1,
            [9]  2,
            [10] 3,
            [11] 4,
            [12] 5,
            [13] 6,
            [14] 7,
            [15] 8,
            [16] 9,
            [17] 10,
            [18] 11,
            [19] 12,
            [20] 14,
            [21] 16,
            [22] 17,
            [23] 19,
            [24] 21,
            [25] 22,
            [26] 24,
            [27] 26,
            [28] 28,
            [29] 30,
            [30] 32,
            [31] 34,
            [32] 36,
            [33] 37,
            [34] 39,
            [35] 40,
            [36] 41,
            [37] 42,
            [38] 43,
            [39] 44,
            [40] 46,
            [41] 48,
            [42] 50
        };
        const u8 *d = &sensor_data[sensor_packet_to_offset[7]];
        printf("wcaster %d wleft %d wright %d bleft %d bright %d\n",
                (d[0] >> 4) & 1,
                (d[0] >> 3) & 1,
                (d[0] >> 2) & 1,
                (d[0] >> 1) & 1,
                (d[0] >> 0) & 1);
        d = &sensor_data[sensor_packet_to_offset[8]];
        printf("wall %d cleft %d cfleft %d cfright %d cright %d\n",
                d[0],d[1],d[2],d[3],d[4]);
        d = &sensor_data[sensor_packet_to_offset[27]];
        printf("wall signal %d\n", (d[0]<<8)|d[1]);
        d = &sensor_data[sensor_packet_to_offset[28]];
        printf("cleft signal %d\n", (d[0]<<8)|d[1]);
        d = &sensor_data[sensor_packet_to_offset[29]];
        printf("cfleft signal %d\n", (d[0]<<8)|d[1]);
        d = &sensor_data[sensor_packet_to_offset[30]];
        printf("cfright signal %d\n", (d[0]<<8)|d[1]);
        d = &sensor_data[sensor_packet_to_offset[31]];
        printf("cright signal %d\n", (d[0]<<8)|d[1]);
        d = &sensor_data[sensor_packet_to_offset[39]];
        printf("velocity %d\n", (d[0]<<8)|d[1]);
    }
}
