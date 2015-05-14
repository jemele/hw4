// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#ifndef _irobot_h_
#define _irobot_h_
#include "uart.h"

// Move in a straight line.
void irobot_drive_straight(uart_t *uart, s16 distance_mm);
void irobot_rotate_left(uart_t *uart);
void irobot_rotate_right(uart_t *uart);

#endif
