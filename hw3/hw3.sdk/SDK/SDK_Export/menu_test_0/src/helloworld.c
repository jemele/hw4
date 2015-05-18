
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

// Migrate to ssd1306 driver.
int map_page(int index);
int map_column(int index);
int map_index(int x, int y);

u32 readButtons(gpio_axi_t *gpio)
{
    u32 Buttons = 0;
    while (Buttons == 0){
        Buttons = XGpio_DiscreteRead(&gpio->device, 1);
        if (Buttons == 1) {
            print("1\n");
            usleep(200000);
        }
        if (Buttons == 2) {
            print("2\n");
            usleep(200000);
        }
        if (Buttons == 4) {
            print("4\n");
            usleep(200000);
        }
        if (Buttons == 8) {
            print("8\n");
            usleep(200000);
        }
        if (Buttons == 16){
            print("16\n");
            usleep(200000);
        }
    }
    return Buttons;
}

u8 mainMenu(gpio_axi_t *gpio, ssd1306_t *oled)
{
    ssd1306_clear(oled);
    ssd1306_set_page_start(oled, 0);
    ssd1306_display_string(oled, "* Pre-program");
    ssd1306_clear_line(oled, 1);
    ssd1306_display_string(oled, "  (X,Y)");
    ssd1306_clear_line(oled, 2);
    ssd1306_display_string(oled, "  Search");
    ssd1306_clear_line(oled, 3);
    ssd1306_display_string(oled, "  Quit");

    u32 selection = 0;
    u8 page = 0;
    do {
        selection = readButtons(gpio);
        if (selection == 8){
            ssd1306_set_page_start(oled, page);
            ssd1306_display_string(oled, "  ");
            if (page == 3) {
                page = 0;
            } else {
                page++;
            }
            ssd1306_set_page_start(oled, page);
            ssd1306_display_string(oled, "* ");
        }

        if (selection == 1){
            ssd1306_set_page_start(oled, page);
            ssd1306_display_string(oled, "  ");
            if (page == 0) {
                page = 3;
            } else {
                page--;
            }
            ssd1306_set_page_start(oled, page);
            ssd1306_display_string(oled, "* ");
        }
    } while (selection != 16);

    ssd1306_clear(oled);
    return page;
}


void user_input_X_Y(gpio_axi_t *gpio, ssd1306_t *oled, int *xyCoord, int *numCoord)
{
    int x = 0;
    int y = 0;
    int i = 0;
    int coordinates = 0;

    ssd1306_clear(oled);
    ssd1306_display_string(oled, "Enter number of ");
    ssd1306_set_page_start(oled, 1);
    ssd1306_display_string(oled, "waypoints:");
    u32 selct = 0;
    char waypoints[20];
    do {
        selct = readButtons(gpio);
        if (selct == 1){
            coordinates++;
        }

        if ((coordinates>0)&&(selct == 8)){
            coordinates--;
        }
        snprintf(waypoints, sizeof(waypoints), "waypoints: %d      ", coordinates);

        ssd1306_set_page_start(oled, 1);
        ssd1306_display_string(oled, waypoints);
    } while (selct != 16);

    *numCoord = coordinates;
    char text[25];
    ssd1306_clear(oled);
    for(i=0; i < coordinates; i++) {
        x = 0;
        y = 0;

        ssd1306_set_page_start(oled, 0);
        ssd1306_display_string(oled, "x: 0    ");
        ssd1306_set_page_start(oled, 1);
        ssd1306_display_string(oled, "y: 0    ");

        snprintf(text, sizeof(text), "coordinate %d of %d     ", (i+1), coordinates);
        ssd1306_set_page_start(oled, 4);
        ssd1306_display_string(oled, text);

        u32 sel = 0;
        char xChar[4];
        char yChar[4];
        do {
            sel = readButtons(gpio);
            if ((sel == 2)&&(x<63)){
                x++;
            }

            if ((sel == 4)&&(x>-64)){
                printf("x: %d\n", x);
                x--;
            }
            if ((sel == 1)&&(y<31)){
                y++;
            }

            if ((sel == 8)&&(y>-32)){
                y--;
            }

            snprintf(xChar, sizeof(xChar), "%d     ", x);
            ssd1306_set_page_start(oled, 0);
            ssd1306_display_string(oled, "x: ");
            ssd1306_display_string(oled, xChar);

            snprintf(yChar, sizeof(yChar), "%d    ", y);
            ssd1306_set_page_start(oled, 1);
            ssd1306_display_string(oled, "y: ");
            ssd1306_display_string(oled, yChar);
        } while (sel != 16);
        *(xyCoord+(2*i)) = x;
        *(xyCoord+(2*i+1)) = y;
    }
  //  *xyCoord = xySet;
}

void user_input_search_duration(gpio_axi_t *gpio, ssd1306_t *oled, int *time)
{
    int t = 0;
    ssd1306_clear(oled);
    ssd1306_display_string(oled, "time: 0");
    ssd1306_set_page_start(oled, 1);
    ssd1306_display_string(oled, "5s increments");
    u32 selct = 0;
    char tChar[5];
    do {
        selct = readButtons(gpio);
        if (selct == 1){
            t+=5;
        }
        if ((t>0)&&(selct == 8)){
            t-=5;
        }
        snprintf(tChar, sizeof(tChar), "%d      ", t);

        ssd1306_set_page_start(oled, 0);
        ssd1306_display_string(oled, "time: ");
        ssd1306_display_string(oled, tChar);
    } while (selct != 16);
    *time = t;
}

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
    u8 selection = 5;

    int t;
    char task[50];
    int xyCoord[50];
    int numCoords = sizeof(xyCoord)/sizeof(*xyCoord);
    while (selection != 3){
        //main menu
        selection = mainMenu(&gpio_axi, &oled0);
        switch (selection) {
            case (0) :
                //run the preprogrammed route
                strcpy(task, "Programmed route");
                break;

            case (1) :
                //run user directed set off x,y coords
                user_input_X_Y(&gpio_axi, &oled0, xyCoord, &numCoords);
                strcpy(task, "X,Y Coordinate");
                for(i=0; i<numCoords*2; i++) {
                    printf("%d\n", xyCoord[i]);
                }
                break;

            case (2) :
                //search
                user_input_search_duration(&gpio_axi, &oled0, &t);
                printf("\nt %d\n",t);
                strcpy(task, "Area Map/Search");
                break;

            case (3) :
                ssd1306_clear(&oled0);
                ssd1306_display_string(&oled0, "Quitting the program");
                exit(0);
        }
        ssd1306_clear(&oled0);
        ssd1306_display_string(&oled0, task);
        ssd1306_set_page_start(&oled0, 1);
        ssd1306_display_string(&oled0, "Complete");
        readButtons(&gpio_axi);
    }

    printf("Exit");
    return 0;
}
