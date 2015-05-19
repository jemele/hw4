// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include <unistd.h>
#include <xtime_l.h>
#include "platform.h"
#include "gpio.h"
#include "irobot.h"
#include "menu.h"
#include "uart.h"
#include "ssd1306.h"
#include "font_5x7.h"
#include "Inspire.h"
#include "search.h"

// Menu context.
typedef struct {
    search_map_t *map;
    uart_t *uart;
    ssd1306_t *oled[2];
} menu_context_t;

// High level moving routines.
void robot_move(uart_t *uart, search_cell_t *path)
{
    search_cell_t *c;
    int x_orientation = 0;
    int y_orientation = 1;
    const s16 unit_distance_mm = 20*8;
    for (c = path->next; c && c->next; c = c->next) {
        printf("%d,%d:%d\n", c->x, c->y, c->f);

        // Issue the move. Assume no obstacles.
        // Calculate the delta.
        // Assume if we need to -x or +x, we need to rotate first.
        const int dx = c->prev->x - c->x;
        const int dy = c->prev->y - c->y;
        printf("dx %d dy %d\n", dx, dy);

        // We should only need to move in dx or only dy.
        if (x_orientation != dx) {
            while (x_orientation-- > dx) {
                irobot_rotate_left(uart);
                printf("rotate left\n");
            }
            while (x_orientation++ < dx) {
                irobot_rotate_right(uart);
                printf("rotate right\n");
            }
        }
        if (y_orientation != dy) {
            if (y_orientation != dy) {
                y_orientation *= -1;
                irobot_rotate_right(uart);
                printf("rotate right\n");
                irobot_rotate_right(uart);
                printf("rotate right\n");
            }
        }
        const int distance_mm =
            irobot_drive_straight_sense(uart,unit_distance_mm);
        printf("drove %d mm\n", distance_mm);
    }
}

// Menu handlers.
void handler_programmed_route(void *context)
{
    // Go from the origin to a corner.
    menu_context_t *menu_context = (menu_context_t*)context;
    uart_t *uart = menu_context->uart;
    search_map_t *map = menu_context->map;

    printf("programmed route\n");
    search_map_initialize(map);

    // Verify we can find our goal.
    search_cell_t *start = search_cell_at(map,0,0);
    search_cell_t *goal = search_cell_at(map,15,7);
    search_find(map, start, goal);

    // Print out our goal.
    // We should actually move toward it.
    if (goal->closed) {
        robot_move(uart, start);
    } else {
        printf("panic: could not find goal!\n");
    }
}

void handler_user_route(int *coords, int count, void *context)
{
    menu_context_t *c = (menu_context_t*)context;
    printf("user route\n");
}
void handler_search(int time_s, void *context)
{
    menu_context_t *c = (menu_context_t*)context;
    printf("search\n");
}

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

    // Start the integrated menu.
    menu_context_t menu_context = {
        .map = &map,
        .uart = &uart0,
        .oled = { [0] &oled0, [1] &oled1 },
    };
    menu_handler_programmed_route = handler_programmed_route;
    menu_handler_user_route = handler_user_route;
    menu_handler_search = handler_search;
    menu_run(&gpio_axi, &oled0, &menu_context);

    search_map_free(&map);
    return 0;

}
#if 0
static void irobot_movement_demo(gpio_axi_t *gpio_axi, uart_t *uart)
{
    // Do a movement demo.
    printf("movement demo\n");
    const s16 unit_distance_mm = 25*8; // ~8 inch
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
            const int distance = irobot_drive_straight_sense(uart,-unit_distance_mm);
            printf("tavelled %d mm\n", distance);
            continue;
        }
        if (button_down_pressed(buttons)) {
            printf("forward\n");
            const int distance = irobot_drive_straight_sense(uart,unit_distance_mm);
            printf("travelled %d mm\n", distance);
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

static void irobot_sensor_demo(gpio_axi_t *gpio_axi, uart_t *uart)
{
    const int unit_distance_mm = 200;

    // Flush the receive pipe to clear spurious data.
    int i = uart_recv_flush(uart);
    printf("flushed %d bytes\n", i);

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
        irobot_sensor_t s;
        irobot_read_sensor(uart, &s);
        printf("bumper %d wall %d\n", s.bumper, s.wall);

        // Flush spurious sensor data (there should be none).
        i = uart_recv_flush(uart);
        printf("flushed %d bytes\n", i);
    }
}
#endif
