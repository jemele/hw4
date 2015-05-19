// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#ifndef _irobot_h_
#define _irobot_h_
#include "uart.h"

typedef struct {
    int bumper;
    int wall;
} irobot_sensor_t;

// This may only be polled at an interval >= 15ms.
void irobot_read_sensor(uart_t *uart, irobot_sensor_t *s);

// Move in a straight line.
void irobot_drive_straight_rate(uart_t *uart, s16 rate);
int irobot_drive_straight_sense(uart_t *uart, s16 distance_mm);
void irobot_drive_straight(uart_t *uart, s16 distance_mm);

// In place rotation.
void irobot_rotate_left(uart_t *uart);
void irobot_rotate_right(uart_t *uart);

#endif
