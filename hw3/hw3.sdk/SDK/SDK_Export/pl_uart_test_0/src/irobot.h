// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#ifndef _irobot_h_
#define _irobot_h_

#include "uart.h"

typedef struct {
    unsigned long long timestamp;
    u8 bumper;
    u8 wall;
} irobot_sensor_t;

// Clear the sensor timestamp, invalidating the record.
void irobot_sensor_initialize(irobot_sensor_t *device);

// An irobot device structure that can be expanded as needed.
typedef struct {
    uart_t uart;
    irobot_sensor_t sensor;
} irobot_t;

// Initialize the irobot device struct.
// Initialize the uart as specified, and initialize the sensor data.
int irobot_initialize(irobot_t *device);

// This may only be polled at an interval >= 15ms.
// The sensor reading will be stored in the device context.
// If provided, sensor will also be populated.
void irobot_read_sensor(irobot_t *device, irobot_sensor_t *sensor);

// Start the robot moving at the specified rate.
// Positive rates drive forward, negative rates drive backward.
void irobot_drive_straight(irobot_t *device, s16 rate);

// In place rotation.
void irobot_rotate_left(irobot_t *device);
void irobot_rotate_right(irobot_t *device);

// Please don't change these values. The direction_rotate function explicitly
// relies on the supplied encoding.
typedef enum {
    direction_left    = 0,
    direction_forward = 1,
    direction_right   = 2,
    direction_back    = 3,
} direction_t;

// Handy for debugging.
const char *direction_t_to_string(direction_t v);

// Calculate the direction based on the delta. This assumes we're starting
// forward.
int direction_from_delta(int dx, int dy);

// Calculate the number and type of rotation needed to achieve the next
// direction from the current direction.
void direction_rotation(int current, int next, char *rotation, int *count);

// Play the specified song. Hopefully it's programmed :)
void irobot_play_song(irobot_t *device, u8 song);

#endif
