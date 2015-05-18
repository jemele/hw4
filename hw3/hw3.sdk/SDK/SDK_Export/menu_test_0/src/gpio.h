// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#ifndef _gpio_h_
#define _gpio_h_

#include <xgpio.h>
#include <xgpiops.h>

// A convenient struct to bundle ps gpio information.
typedef struct {
    int id;
    XGpioPs device;
    XGpioPs_Config *config;
} gpio_t;

// Initialize the specified gpio.
int gpio_initialize(gpio_t *gpio);

// A convenient struct to bundle axi gpio information.
typedef struct {
    int id;
    XGpio device;
} gpio_axi_t;

// Initialize the specified gpio.
int gpio_axi_initialize(gpio_axi_t *gpio);

// Poll the gpio waiting for a value.
// When a value is available, return the value.
u32 gpio_axi_blocking_read(gpio_axi_t *gpio);

// The available buttons.
enum button_mask {
    button_up       = 1<<0,
    button_right    = 1<<1,
    button_left     = 1<<2,
    button_down     = 1<<3,
    button_center   = 1<<4,
};

// Convenience macros to test for a given button.
#define button_up_pressed(v) (v & button_up)
#define button_right_pressed(v) (v & button_right)
#define button_left_pressed(v) (v & button_left)
#define button_down_pressed(v) (v & button_down)
#define button_center_pressed(v) (v & button_center)
#endif
