// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#ifndef _ssd1306_h_
#define _ssd1306_h_

#include <xiicps.h>
#include "gpio.h"

// An i2c device context.
typedef struct {
    int id;
    int clock;
    int clear_options;
    int options;
    int write_delay_us;
    XIicPs_Config *config;
    XIicPs device;
} i2c_t;

// Initialize an i2c device.
int i2c_initialize(i2c_t *i2c);

// Write a command out to the specified i2c device.
void i2c_command(i2c_t *i2c, u8 addr, u8 command);

// Write data out to the specified i2c device.
void i2c_data(i2c_t *i2c, u8 addr, u8 data);

// A font descriptor.
typedef struct {
    const u8 * base;
    int size;
    char start;
    int width;
    int pad;
} font_t;

// An ssd1306 device context.
typedef struct {
    u8 addr;
    int reset_pin;
    i2c_t *i2c;
    gpio_t *gpio;
    font_t *font;
} ssd1306_t;

// Useful display driver functions. 
void ssd1306_reset(ssd1306_t *device);
void ssd1306_set_page_start(ssd1306_t *device, u8 page);
void ssd1306_set_col_start(ssd1306_t *device, u8 col);
void ssd1306_clear_line(ssd1306_t *device, u8 line);
void ssd1306_clear(ssd1306_t *device);
void ssd1306_display_character(ssd1306_t *device, char c);
void ssd1306_display_string(ssd1306_t *device, const char *s);

#endif
