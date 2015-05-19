// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#ifndef _menu_h_
#define _menu_h_

#include "gpio.h"
#include "ssd1306.h"

enum menu_id_t {
    menu_id_programmed_route = 0,
    menu_id_user_route       = 1,
    menu_id_search           = 2,
    menu_id_quit             = 3,
};

typedef void (*fn_programmed_route)(void *context);
typedef void (*fn_user_route)(int *coords, int count, void *context);
typedef void (*fn_search)(int time_s, void *context);

// Handlers that will fire if set when using menu_run.
extern fn_programmed_route menu_handler_programmed_route;
extern fn_user_route menu_handler_user_route;
extern fn_search menu_handler_search;

// Run the menuing system.
void menu_run(gpio_axi_t *gpio, ssd1306_t *oled, void *context);

// The main menu, with menu options corresponding to menu_id_t.
u8 menu_main(gpio_axi_t *gpio, ssd1306_t *oled);

// Read selectable number of coordinates from the user.
void menu_input_coordinates(gpio_axi_t *gpio, ssd1306_t *oled, int *xyCoord,
        int *numCoord);

// Read time duration from the user.
void menu_input_time(gpio_axi_t *gpio, ssd1306_t *oled, int *time);

#endif
