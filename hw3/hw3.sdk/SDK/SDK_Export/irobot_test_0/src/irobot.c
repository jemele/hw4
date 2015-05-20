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

void irobot_read_sensor(uart_t *uart, irobot_sensor_t *s)
{
    const u8 c[] = {149,2,7,8};
    uart_sendv(uart,c,sizeof(c));

    int i;
    u8 d[2];
    for (i = 0; i < sizeof(d); ++i) {
        d[i] = uart_recv(uart);
    }

    s->bumper = d[0] & 0x3;
    s->wall = d[1];
}

#define abs(x) ((x<0)?-x:x)

void irobot_drive_straight_rate(uart_t *uart, s16 rate)
{
    const u8 c[] = {137,(rate>>8)&0xff,rate&0xff,0x80,0};
    uart_sendv(uart,c,sizeof(c));
}

static void wait_for_interval(XTime start, int time_ms)
{
    XTime stop = start + (time_ms*COUNTS_PER_SECOND/1000);
    XTime current;
    do {
        XTime_GetTime(&current);
    } while (current < stop);
}

// Move in a straight line polling for obstacles.
int irobot_drive_straight_sense(uart_t *uart, s16 distance_mm)
{
    const int abs_speed = 100; //mm/s
    const int abs_distance = abs(distance_mm);
    const int travel_time_ms = abs_distance*1000/abs_speed;
    const int polling_interval_ms = 15;
    const int intervals = travel_time_ms/polling_interval_ms;

    // flush spurious recv data.
    uart_recv_flush(uart);

    // Sample the clock.
    XTime start_clock;
    XTime_GetTime(&start_clock);

    // start moving.
    const s16 speed = (distance_mm < 0) ? -abs_speed : abs_speed;
    irobot_drive_straight_rate(uart, speed);
    wait_for_interval(start_clock, polling_interval_ms);

    // for each interval, capture sensor data.
    // if we hit a bump, stop and report the distance traveled.
    // if we complete the travel, return the distance traveled.
    int i;
    irobot_sensor_t s;
    for (i = 0; i < intervals; ++i) {
        irobot_read_sensor(uart, &s);
        if (s.bumper) {
            break;
        }

        XTime current_clock;
        XTime_GetTime(&current_clock);
        wait_for_interval(current_clock, polling_interval_ms);
    }
    irobot_drive_straight_rate(uart, 0);

    XTime stop_clock;
    XTime_GetTime(&stop_clock);

    // XXX We could sample XTime to get more precise distance travelled metrics.
    return (i * polling_interval_ms * abs_speed)/1000; //mm
}

// Move in a straight line. This *will* move until the appropriate distance is
// travelled, obstacle or not. Beware!
void irobot_drive_straight(uart_t *uart, s16 distance_mm)
{
    const s16 speed = (distance_mm < 0) ? -100 : 100; //mm/s

    // rotate ccw 90 degrees and stop
    const u8 c[] = {152,13,
        137,(speed>>8)&0xff,speed&0xff,0x80,0,
        156,(distance_mm>>8)&0xff,distance_mm&0xff,
        137,0,0,0,0};
    uart_sendv(uart,c,sizeof(c));

    // Run the program.
    usleep(1000);
    uart_send(uart,153);

    // Wait for the program to complete.
    // Ideally, this would be derived from the rotational velocity.
    usleep(1000 * 500);
}

// Rotate left.
void irobot_rotate_left(uart_t *uart)
{
    const s16 speed = 100; //mm/s
    const s16 angle = 90;
    printf("ccw %d degrees\n", angle);

    // rotate ccw 90 degrees and stop
    const u8 c[] = {152,13,
        137,(speed>>8)&0xff,speed&0xff,0,1,
        157,(angle>>8)&0xff,angle&0xff,
        137,0,0,0,0};
    uart_sendv(uart,c,sizeof(c));

    // Run the program.
    usleep(1000);
    uart_send(uart,153);

    // Wait for the program to complete.
    // Ideally, this would be derived from the rotational velocity.
    usleep(1000 * 1000);
}

// Rotate right.
void irobot_rotate_right(uart_t *uart)
{
    const s16 speed = 100; //mm/s
    const s16 angle = -90;
    printf("cw %d degrees\n", angle);

    // rotate cw 90 degrees and stop
    const u8 c[] = {152,13,
        137,(speed>>8)&0xff,speed&0xff,0xff,0xff,
        157,(angle>>8)&0xff,angle&0xff,
        137,0,0,0,0};
    uart_sendv(uart,c,sizeof(c));

    // Run the program.
    usleep(1000);
    uart_send(uart,153);

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
void irobot_rotate(uart_t *uart, direction_t direction_current, direction_t direction_next)
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
        case 'R': irobot_rotate_right(uart); break;
        case 'L': irobot_rotate_left(uart);  break;
        }
    }
}

search_cell_t* irobot_move(uart_t *uart, ssd1306_t *oled, search_map_t *map,
        search_cell_t *start, search_cell_t *goal, int timeout_s)
{
    const s16 unit_distance_mm = 24*8; // ~8 inches

    // Sample the time, used to track elapsed time.
    XTime time_start;
    XTime_GetTime(&time_start);

    // We always assume starting on the origin, facing forward.
    // In a future iteration, someone can tell us our start state.
    direction_t direction_current = direction_forward;

    // Mark our starting position on the map.
    ssd1306_display_square(oled, start->x*8, start->y*8, ssd1306_square_stipple);

    // Walk through the path, calculating movement with each cell.
    search_cell_t *c;
    for (c = start->next; c; c = c->next) {

        // Have we timed out? If so, move back one cell (we never did actually
        // move), and bail..
        if (timeout_s) {
            XTime time_now;
            XTime_GetTime(&time_now);
            const int elapsed_s = (time_now - time_start)/COUNTS_PER_SECOND;
            if (elapsed_s > timeout_s) {
                c = c->prev;
                printf("timeout x:%d y:%d\n", c->x, c->y);
                break;
            }
        }

        // Calculate the delta, and ignore vacuous moves.
        const int dx = c->x - c->prev->x;
        const int dy = c->y - c->prev->y;
        if (!dx && !dy) {
            continue;
        }
        printf("x:%d y:%d dx:%d dy:%d\n", c->x, c->y, dx, dy);

        // Calculate the direction and rotate.
        direction_t direction_next = direction_from_delta(dx,dy);
        irobot_rotate(uart, direction_current, direction_next);
        direction_current = direction_next;

        // Travel a unit distance.
        const int distance_mm =
            irobot_drive_straight_sense(uart,unit_distance_mm);
        printf("drove %d mm\n", distance_mm);

        // If we didn't travel the full length, we've hit something.
        // Figure out where the obstacle is, and route around.
        if (distance_mm < unit_distance_mm/2) {

            // Mark and draw the obstacle.
            printf("obstacle found near x:%d y:%d\n", c->x, c->y);
            c->blocked = 1;
            ssd1306_display_square(oled, c->x*8, c->y*8, ssd1306_square_solid);

            printf("backing up %d mm\n", distance_mm);
            irobot_drive_straight(uart, -distance_mm);

            // Pathfind around the obstacle.
            c = c->prev;
            printf("find %d,%d->%d,%d\n", c->x,c->y,goal->x,goal->y);
            search_map_initialize(map,0);
            search_find(map, c, goal);
            if (!goal->closed) {
                printf("panic: could not route around obstacle!\n");
                break;
            }
        } else {

            // The move was successful, update our position on the map.
            ssd1306_display_square(oled, c->prev->x*8, c->prev->y*8, ssd1306_square_blank);
            ssd1306_display_square(oled, c->x*8, c->y*8, ssd1306_square_stipple);
        }
    }

    // Finally, reorient to starting stance.
    irobot_rotate(uart, direction_current, direction_forward);
    return c;
}

void irobot_play_song(uart_t *uart, u8 song)
{
    const u8 c[] = {141,song};
    uart_sendv(uart,c,sizeof(c));
}
