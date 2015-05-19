// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include <unistd.h>
#include "menu.h"

// Handlers that will fire if set when using menu_run.
fn_programmed_route menu_handler_programmed_route = 0;
fn_user_route menu_handler_user_route = 0;
fn_search menu_handler_search = 0;

// Run the menu, invoking any registered handlers as necessary.
void menu_run(gpio_axi_t *gpio, ssd1306_t *oled, void *context)
{
    char task[50];
    for (;;) {
        u8 selection = menu_main(gpio, oled);
        switch (selection) {
            case menu_id_programmed_route:
                strcpy(task, "Programmed route");
                if (menu_handler_programmed_route) {
                    menu_handler_programmed_route(context);
                }
                break;

            case menu_id_user_route: {
                int i;
                int coords[50];
                int count = sizeof(coords)/sizeof(*coords);
                menu_input_coordinates(gpio, oled, coords, &count);
                strcpy(task, "X,Y Coordinate");
                if (menu_handler_user_route) {
                    menu_handler_user_route(coords,count,context);
                }
                break;
             }

            case menu_id_search: {
                int t;
                menu_input_time(gpio, oled, &t);
                strcpy(task, "Area Map/Search");
                if (menu_handler_search) {
                    menu_handler_search(t,context);
                }
                break;
             }

            case menu_id_quit: {
                ssd1306_clear(oled);
                ssd1306_display_string(oled, "Quitting the program");
                return;
           }
        }

        ssd1306_clear(oled);
        ssd1306_display_string(oled, task);
        ssd1306_set_page_start(oled, 1);
        ssd1306_display_string(oled, "Complete");
        gpio_axi_blocking_read(gpio);
    }
}

u8 menu_main(gpio_axi_t *gpio, ssd1306_t *oled)
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
        selection = gpio_axi_blocking_read(gpio);
        usleep(250*1000);
        if (selection == button_down){
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

        if (selection == button_up){
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

void menu_input_coordinates(gpio_axi_t *gpio, ssd1306_t *oled, int *xyCoord, int *numCoord)
{
    int x = 0;
    int y = 0;
    int i = 0;
    int coordinates = 0;

    ssd1306_clear(oled);
    ssd1306_display_string(oled, "Enter number of ");
    ssd1306_set_page_start(oled, 1);
    ssd1306_display_string(oled, "waypoints:");
    u32 select = 0;
    char waypoints[20];
    do {
        select = gpio_axi_blocking_read(gpio);
        usleep(100*1000);
        if (select == button_up){
            coordinates++;
        }

        if ((coordinates>0)&&(select == button_down)){
            coordinates--;
        }
        snprintf(waypoints, sizeof(waypoints), "waypoints: %d      ", coordinates);

        ssd1306_set_page_start(oled, 1);
        ssd1306_display_string(oled, waypoints);
    } while (select != 16);

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
            sel = gpio_axi_blocking_read(gpio);
            usleep(100*1000);
            if ((sel == button_right)&&(x<63)){
                x++;
            }
            if ((sel == button_left)&&(x>-64)){
                x--;
            }
            if ((sel == button_up)&&(y<31)){
                y++;
            }
            if ((sel == button_down)&&(y>-32)){
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

        } while (sel != button_center);
        *(xyCoord+(2*i)) = x;
        *(xyCoord+(2*i+1)) = y;
    }
}

void menu_input_time(gpio_axi_t *gpio, ssd1306_t *oled, int *time)
{
    int t = 0;
    ssd1306_clear(oled);
    ssd1306_display_string(oled, "time: 0");
    ssd1306_set_page_start(oled, 1);
    ssd1306_display_string(oled, "5s increments");
    u32 select = 0;
    char tChar[5];
    do {
        select = gpio_axi_blocking_read(gpio);
        usleep(100*1000);
        if (select == button_up){
            t+=5;
        }
        if ((t>0)&&(select == button_down)){
            t-=5;
        }
        snprintf(tChar, sizeof(tChar), "%d      ", t);

        ssd1306_set_page_start(oled, 0);
        ssd1306_display_string(oled, "time: ");
        ssd1306_display_string(oled, tChar);
    } while (select != 16);
    *time = t;
}


