// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdlib.h>
#include <stdio.h>
#include "platform.h"
#include <xiicps.h>
#include <xil_cache.h>
#include <xil_exception.h>
#include "gpio.h"
#include "ssd1306.h"
#include <xscuwdt.h>
#include <sleep.h>
#include <xtime_l.h>
#include "font_5x7.h"
#include "Inspire.h"
#include <stdint.h>
#include "menu.h"

// Migrate to ssd1306 driver.
int map_page(int index);
int map_column(int index);
int map_index(int x, int y);

void write_pixel(ssd1306_t *device, int x, int y, int *map)
{
    //origin for the device is in the center of the map
    int index = map_index(x,y);
    printf("index %d:\n", index);
    int page = map_page(index);
    int column = x+64;
    u8 data = 0x00;
    int i = 0;
    printf("page: %d column: %d \n", page, column);

    int indexCount = (page*128*8)+column;
    int placeHolder = 0;
    for (i=0; i<8; ++i) {
        placeHolder =(1<<i);
        data += placeHolder*map[indexCount];
        printf("data: %d\n", data);
        indexCount += 128;
    }
    printf("data: %d indexCount: %d\n", data, indexCount);
    ssd1306_set_page_start(device, page);
    ssd1306_set_col_start(device, column);
    i2c_data(device->i2c, device->addr, data);
    ssd1306_set_page_start(device, 0);
}

void write_square(ssd1306_t *device, int x, int y, int *map, int drawType)
{
    //origin for the device is in the center of the map
    int index = map_index(x,y);
    printf("index %d:\n", index);
    int page = map_page(index);
    int column = map_column(x+64);
    u8 data = 0xFF;
    int i = 0;
    printf("page: %d column: %d \n", page, column);

    ssd1306_set_page_start(device, page);
    ssd1306_set_col_start(device, 8*column);
    int hash= 0;
    for (i=0; i<8; i++) {
        if ((drawType == 1)&&(hash==0)) {
            data=0x55;
        }
        if ((drawType == 1)&&(hash==1)){
            data =0xAA;
        }
        i2c_data(device->i2c, device->addr, data);
        if (hash==0) {
            hash=1;
        } else {
            hash=0;
        }
    }
    ssd1306_set_page_start(device, 0);
}

void write_obstacle(ssd1306_t *device, int x, int y, int drawType)
{
    // Allows non-map OLED to write what obstacle was found last and where
    ssd1306_clear_line(device,5);
    ssd1306_clear_line(device,6);
    ssd1306_set_page_start(device, 5);
    ssd1306_display_string(device, "Obstacle detected:");
    ssd1306_set_page_start(device, 6);

    char obstacle[20];
    if (drawType == 2) { //drawType 2 is a wall
        snprintf(obstacle, sizeof(obstacle), "Wall at x=%d, y=%d     ", x, y);
    }
    if (drawType == 3) { //drawType 2 is a cliff
        snprintf(obstacle, sizeof(obstacle), "Cliff at x=%d, y=%d     ", x, y);
    }
    ssd1306_display_string(device, obstacle);
}

int map_index(int x, int y)
{
    int index= 0;

    //we no from x what column to write to, but must figure out which page to write to
    //now we figure out the page
    x += 64;
    y -= 31;
    index = -1*y*128 + x;
    return index;
}

int map_page(int index)
{
    int page = 0;
    while (index > (128*8)) {
        page++;
        index -= 128*8;
    }
    return page;
}

int map_column(int index)
{
    int column = 0;
    while (index > (8)) {
        column++;
        index -= 8;
    }
    return column;
}

int main()
{
    init_platform();

    printf("Hello world\n");

    int status, i;

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

    sleep(2);

#if 0
    // playground area
    ssd1306_clear(&oled1);
    ssd1306_clear(&oled0);
    int map[8192];
    for(i = 0; i < 8192; i++) {
        map[i] = 0;
    }
    map[map_index(-64, 31)] = 1;
    map[map_index(-64, -32)] = 1;
    map[map_index(63, 31)] = 1;
    map[map_index(63, -32)] = 1;

    map[map_index(0, 0)] = 1;

    printf("index %d:\n", map_index(-64, 31));
    printf("index %d:\n", map_index(-64, -32));
    printf("index %d:\n", map_index(63, 31));
    printf("index %d:\n", map_index(63, -32));

    write_pixel(&oled1, -64, 31, map);
    write_pixel(&oled1, -64, -32, map);
    write_pixel(&oled1, 63, 31, map);
    write_pixel(&oled1, 63, -32, map);
    write_pixel(&oled1, 0, 0, map);

    write_square(&oled1, 0, 0, map, 1);
    write_square(&oled1, -64, 31, map, 2);
    sleep(4);
    write_obstacle(&oled1, 0, 0, 2);
    sleep(4);

    // end playground area
#endif

    menu_run(&gpio_axi, &oled0);
    printf("Exit");
    return 0;
}
