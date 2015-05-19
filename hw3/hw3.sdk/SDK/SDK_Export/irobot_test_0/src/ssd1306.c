// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include <unistd.h>
#include "platform.h"
#include "ssd1306.h"

// Initialize an i2c device.
int i2c_initialize(i2c_t *i2c)
{
    i2c->config = XIicPs_LookupConfig(i2c->id);
    if (!i2c->config) {
        printf("XIicPs_LookupConfig failed\n");
        return XST_FAILURE;
    }
    printf("i2c initialization\n");
    int status = XIicPs_CfgInitialize(&i2c->device, i2c->config,
            i2c->config->BaseAddress);
    if (status != XST_SUCCESS) {
        printf("XIicPs_CfgInitialize failed %d\n", status);
        return status;
    }
    status = XIicPs_SelfTest(&i2c->device);
    if (status != XST_SUCCESS) {
        printf("XIicPs_SelfTest failed %d\n", status);
        return status;
    }
    status = XIicPs_SetSClk(&i2c->device, i2c->clock);
    if (status != XST_SUCCESS) {
        printf("XIicPs_SetSClk failed %d\n", status);
        return status;
    }
    status = XIicPs_ClearOptions(&i2c->device, i2c->clear_options);
    if (status != XST_SUCCESS) {
        printf("XIicPs_ClearOptions failed %d\n", status);
        return status;
    }
    status = XIicPs_SetOptions(&i2c->device, i2c->options);
    if (status != XST_SUCCESS) {
        printf("XIicPs_SetOptions failed %d\n", status);
        return status;
    }
    return 0;

}

// Write a command out to the specified i2c device.
void i2c_command(i2c_t *i2c, u8 addr, u8 command)
{
    const u8 buf[] = {0x80,command};
    XIicPs_MasterSend(&i2c->device, (u8*)buf, sizeof(buf), addr);
    usleep(i2c->write_delay_us);
}

// Write data out to the specified i2c device.
void i2c_data(i2c_t *i2c, u8 addr, u8 data)
{
    const u8 buf[] = {0x40,data};
    XIicPs_MasterSend(&i2c->device, (u8*)buf, sizeof(buf), addr);
    usleep(i2c->write_delay_us);
}

void ssd1306_reset(ssd1306_t *device)
{
    printf("initializing ssd1306\n");
    XGpioPs_SetDirectionPin(&device->gpio->device, device->reset_pin, 1);
    XGpioPs_SetOutputEnablePin(&device->gpio->device, device->reset_pin, 1);
    XGpioPs_WritePin(&device->gpio->device, device->reset_pin, 0);
    usleep(300);
    XGpioPs_WritePin(&device->gpio->device, device->reset_pin, 1);

    // ssd1306 initialization sequence, derived from:
    // https://github.com/Defragster/ssd1306xled/blob/master/ssd1306xled.cpp
    const u8 seq[] = {
    0xAE,           // Display OFF (sleep mode)
    0x20, 0b00,     // Set Memory Addressing Mode
                    // 00=Horizontal Addressing Mode; 01=Vertical Addressing Mode;
                    // 10=Page Addressing Mode (RESET); 11=Invalid
    0xB0,           // Set Page Start Address for Page Addressing Mode, 0-7
    0xC8,           // Set COM Output Scan Direction
    0x00,           // ---set low column address
    0x10,           // ---set high column address
    0x40,           // --set start line address
    0x81, 0x3F,     // Set contrast control register
    0xA1,           // Set Segment Re-map. A0=address mapped; A1=address 127 mapped.
    0xA6,           // Set display mode. A6=Normal; A7=Inverse
    0xA8, 0x3F,     // Set multiplex ratio(1 to 64)
    0xA4,           // Output RAM to Display
                    // 0xA4=Output follows RAM content; 0xA5,Output ignores RAM content
    0xD3, 0x00,     // Set display offset. 00 = no offset
    0xD5,           // --set display clock divide ratio/oscillator frequency
    0xF0,           // --set divide ratio
    0xD9, 0x22,     // Set pre-charge period
    0xDA, 0x12,     // Set com pins hardware configuration
    0xDB,           // --set vcomh
    0x20,           // 0x20,0.77xVcc
    0x8D, 0x14,     // Set DC-DC enable
    0xAF            // Display ON in normal mode
    };

    // Write out the oled initialization sequence.
    int i;
    for (i = 0; i < sizeof(seq); ++i) {
       i2c_command(device->i2c, device->addr, seq[i]);
    }
}

void ssd1306_set_col_start(ssd1306_t *device, u8 col)
{
    i2c_command(device->i2c, device->addr, col%16);
    i2c_command(device->i2c, device->addr, 0x10+(col/16));
}

void ssd1306_set_page_start(ssd1306_t *device, u8 page)
{
    i2c_command(device->i2c, device->addr, 0xb0|(page&0x7));
    ssd1306_set_col_start(device, 0);
}

void ssd1306_clear_line(ssd1306_t *device, u8 line)
{
    ssd1306_set_page_start(device, line);
    int i;
    for (i = 0; i < 128; ++i) {
        i2c_data(device->i2c, device->addr, 0);
    }
    ssd1306_set_page_start(device, line);
}

void ssd1306_clear(ssd1306_t *device)
{
    int i;
    ssd1306_set_page_start(device, 0x00);
    for (i = 0; i < (128*8); ++i) {
        i2c_data(device->i2c, device->addr, 0);
    }
    ssd1306_set_page_start(device, 0x00);
}

void ssd1306_display_character(ssd1306_t *device, char c)
{
    int index = (device->font->width * (c - device->font->start));
    if ((index < 0) || (index >= device->font->size)) {

        // default to '?' if unknown.
        // This isn't optimal, but it's good enough for now.
        c = '?';
        index = (device->font->width * (c - device->font->start));
    }
    int i;
    for (i = 0; i < device->font->width; ++i) {
        i2c_data(device->i2c, device->addr, device->font->base[index+i]);
    }
    for (i = 0; i < device->font->pad; ++i) {
        i2c_data(device->i2c, device->addr, 0);
    }
}

void ssd1306_display_string(ssd1306_t *device, const char *s)
{
    while (*s) {
        putchar(*s);
        ssd1306_display_character(device, *s++);
    }
}

