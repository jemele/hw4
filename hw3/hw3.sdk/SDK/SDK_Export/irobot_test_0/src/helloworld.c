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

// Menu handlers.
void handler_programmed_route(void *context)
{
    // Go from the origin to a corner.
    menu_context_t *menu_context = (menu_context_t*)context;
    uart_t *uart = menu_context->uart;
    search_map_t *map = menu_context->map;

    // Verify we can find our goal.
    // We assume each time the programmed route is run, there could be new
    // obstacles, so we clear the map of obstacles.
    printf("programmed route\n");
    search_map_initialize(map,1);
    search_cell_t *start = search_cell_at(map,0,0);
    search_cell_t *goal = search_cell_at(map,(128/8)/2,(64/8)/2);
    search_find(map, start, goal);
    if (!goal->closed) {
        printf("panic: could not find goal!\n");
        return;
    }
    irobot_move(uart, start);

    // Reset the map to find a new goal, but don't clear obstacle memory.
    search_map_initialize(map,0);
    search_find(map, goal, start);
    if (!goal->closed) {
        printf("panic: could not find goal!\n");
        return;
    }
    irobot_move(uart, goal);
}

// Move through all the user defined waypoints. At each waypoint, play a song.
// When finished with the user waypoints, return to base. This assumes the
// starting location is (0,0).
void handler_user_route(int *coords, int count, void *context)
{
    menu_context_t *menu_context = (menu_context_t*)context;
    uart_t *uart = menu_context->uart;
    search_map_t *map = menu_context->map;

    printf("user route: %d waypoints\n", count);
    search_cell_t *start = search_cell_at(map,0,0);
    search_cell_t *goal = 0;

    // for each waypoint, move to the waypoint and play a song.
    // finally, return to base.
    int i;
    for (i = 0; i < count; ++i) {
        const int x = coords[(2*i)]/8;
        const int y = coords[(2*i)+1]/8;
        printf("waypoint x:%d y:%d\n", x, y);
        goal = search_cell_at(map,x,y);

        // search and move
        // clear obstacle memory only on start;
        // subsequent waypoints should retain obstacle memory.
        search_map_initialize(map,i==0);
        search_find(map, start, goal);
        if (!goal->closed) {
            printf("panic: could not find goal!\n");
            return;
        }
        irobot_move(uart, start);
        irobot_play_song(uart, 0);
        start = goal;
    }

    // search and return to base
    goal = search_cell_at(map,0,0);
    search_map_initialize(map,0);
    search_find(map, start, goal);
    if (!goal->closed) {
        printf("panic: could not find goal!\n");
        return;
    }
    irobot_move(uart, start);
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

    // Configure buttons.
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

