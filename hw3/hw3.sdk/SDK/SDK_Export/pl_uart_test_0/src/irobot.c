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
    uart_recv_flush(&device->uart);
    irobot_sensor_initialize(&device->sensor);

    device->rate = 0;
    device->direction = direction_forward;
    device->x = 0;
    device->y = 0;

    return 0;
}

void irobot_passive_mode(irobot_t *device)
{
    const u8 c[] = {128};
    uart_sendv(&device->uart,c,sizeof(c));
    const int n = uart_recv_flush(&device->uart);
    printf("irobot: flushed %d\n", n);
}

void irobot_safe_mode(irobot_t *device)
{
    const u8 c[] = {131};
    uart_sendv(&device->uart,c,sizeof(c));
    const int n = uart_recv_flush(&device->uart);
    printf("irobot: flushed %d\n", n);
}

void irobot_full_mode(irobot_t *device)
{
    const u8 c[] = {132};
    uart_sendv(&device->uart,c,sizeof(c));
    const int n = uart_recv_flush(&device->uart);
    printf("irobot: flushed %d\n", n);
}

void irobot_read_sensor(irobot_t *device)
{
    // Verify the poll period has elapsed.
    if (device->sensor.timestamp) {
        XTime now;
        XTime_GetTime(&now);
        const XTime elapsed = now - device->sensor.timestamp;
        const XTime polling_interval =
            irobot_sensor_polling_interval_ms * (COUNTS_PER_SECOND/1000);
        if (elapsed < polling_interval) {
            return;
        }
    }

    // Query packet id 7, 8, 19.
    const u8 c[] = {149,3,7,8,19};
    uart_sendv(&device->uart,c,sizeof(c));

    int i;
    u8 d[4];
    for (i = 0; i < sizeof(d); ++i) {
        d[i] = uart_recv(&device->uart);
    }

    XTime_GetTime(&device->sensor.timestamp);
    device->sensor.bumper = d[0] & 0x3;
    device->sensor.wall = d[1];
    device->sensor.distance = (d[2]<<8)|d[3];

    // Accumulate the distance travelled.
    switch (device->direction) {
    case direction_left:
        device->x -= device->sensor.distance;
        break;
    case direction_right:
        device->x += device->sensor.distance;
        break;
    case direction_forward:
        device->y += device->sensor.distance;
        break;
    case direction_back:
        device->y -= device->sensor.distance;
        break;
    default: 
        break;
    }
}

#define abs(x) ((x<0)?-x:x)

// Drive straight at the specified rate.
// XXX consider adding a polling cycle as part of the main loop.
void irobot_drive_straight(irobot_t *device, s16 rate)
{
    const u8 c[] = {137,(rate>>8)&0xff,rate&0xff,0x80,0};
    uart_sendv(&device->uart,c,sizeof(c));
    device->rate = rate;
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

    // Track our direction.
    device->direction -= 1;
    device->direction %= direction_count;
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

    // Track our direction.
    device->direction += 1;
    device->direction %= direction_count;
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
