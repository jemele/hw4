// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include <unistd.h>
#include <xtime_l.h>
#include "platform.h"
#include "irobot.h"

const char *direction_t_to_string(direction_t v) {
    static const char *s[] = {
    [direction_left]    "left",
    [direction_forward] "forward",
    [direction_right]   "right",
    [direction_back]    "back",
    };
    return s[v];
}

void irobot_sensor_initialize(irobot_sensor_t *device)
{
    device->timestamp = 0;
}

int irobot_initialize(irobot_t *device)
{
    int status;

    status = uart_initialize(&device->uart);
    if (status) {
        printf("uart_initialize failed %d\n", status);
        return status;
    }

    irobot_sensor_initialize(&device->sensor);
    return 0;
}

void irobot_read_sensor(irobot_t *device, irobot_sensor_t *s)
{
    const u8 c[] = {149,2,7,8};
    uart_sendv(&device->uart,c,sizeof(c));

    int i;
    u8 d[2];
    for (i = 0; i < sizeof(d); ++i) {
        d[i] = uart_recv(&device->uart);
    }

    XTime_GetTime(&device->sensor.timestamp);
    device->sensor.bumper = d[0] & 0x3;
    device->sensor.wall = d[1];
    if (s) {
        *s = device->sensor;
    }
}

#define abs(x) ((x<0)?-x:x)

// Drive straight at the specified rate.
// XXX consider adding a polling cycle as part of the main loop.
void irobot_drive_straight(irobot_t *device, s16 rate)
{
    const u8 c[] = {137,(rate>>8)&0xff,rate&0xff,0x80,0};
    uart_sendv(&device->uart,c,sizeof(c));
}

// Rotate left.
void irobot_rotate_left(irobot_t *device)
{
    const s16 speed = 100; //mm/s
    const s16 angle = 90;
    printf("ccw %d degrees\n", angle);

    // rotate ccw 90 degrees and stop
    const u8 c[] = {152,13,
        137,(speed>>8)&0xff,speed&0xff,0,1,
        157,(angle>>8)&0xff,angle&0xff,
        137,0,0,0,0};
    uart_sendv(&device->uart,c,sizeof(c));

    // Run the program.
    usleep(1000);
    uart_send(&device->uart,153);

    // Wait for the program to complete.
    // Ideally, this would be derived from the rotational velocity.
    usleep(1000 * 1000);
}

// Rotate right.
void irobot_rotate_right(irobot_t *device)
{
    const s16 speed = 100; //mm/s
    const s16 angle = -90;
    printf("cw %d degrees\n", angle);

    // rotate cw 90 degrees and stop
    const u8 c[] = {152,13,
        137,(speed>>8)&0xff,speed&0xff,0xff,0xff,
        157,(angle>>8)&0xff,angle&0xff,
        137,0,0,0,0};
    uart_sendv(&device->uart,c,sizeof(c));

    // Run the program.
    usleep(1000);
    uart_send(&device->uart,153);

    // Wait for the program to complete.
    // Ideally, this would be derived from the rotational velocity.
    usleep(1000 * 1000);
}

#define abs(x) ((x<0)?-x:x)
#define sign(x) ((x<0)?-1:1)

// Calculate the moves needed to go from one direction to another.
void direction_rotation(int current, int next, char *rotation, int *count)
{
    *count = 0;
    if (current == next) {
        return;
    }
    int delta = next - current;
    if (abs(delta) == 3) {
        delta = -sign(delta);
    }
    *rotation = ((delta < 0) ? 'L' : 'R');
    *count = abs(delta);
    printf("%s->%s: %d%c\n", direction_t_to_string(current),
            direction_t_to_string(next), *count, *rotation);
}

// Calculate the final orientation of the robot given dx and dy.  This assumes
// that dx and dy cannot *both* be set, i.e.., diagonal moves are not
// permitted.
int direction_from_delta(int dx, int dy)
{
    switch (dx) {
    case -1: return direction_left;
    case +1: return direction_right;
    }
    switch (dy) {
    case -1: return direction_back;
    case +1: return direction_forward;
    }

    // We should never get here
    printf("panic: unknown direction!\n");
    return direction_forward;
}

// High level moving routines.
void irobot_rotate(irobot_t *device, direction_t direction_current, direction_t direction_next)
{
    // Calculate the rotation needed to reorient on the next direction.
    char rotation;
    int rotation_count;
    direction_rotation(direction_current, direction_next, &rotation,
            &rotation_count);

    // Make the rotation so.
    int i;
    for (i = 0; i < rotation_count; ++i) {
        switch (rotation) {
        case 'R': irobot_rotate_right(device); break;
        case 'L': irobot_rotate_left(device);  break;
        }
    }
}

void irobot_play_song(irobot_t *device, u8 song)
{
    const u8 c[] = {141,song};
    uart_sendv(&device->uart,c,sizeof(c));
}