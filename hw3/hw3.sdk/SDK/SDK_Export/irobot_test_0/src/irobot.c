// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include <unistd.h>
#include <xtime_l.h>
#include "platform.h"
#include "irobot.h"

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

// Rotate left.
void irobot_rotate_left(uart_t *uart)
{
    printf("ccw 90 degrees\n");
    const s16 speed = 100; //mm/s
    const s16 angle = 90;

    // rotate ccw 90 degrees and stop
    const u8 c[] = {152,13,
        137,(speed>>8)&0xff,speed&0xff,0,1,
        157,(angle>>8)&0xff,angle&0xff,
        137,0,0,0,0};
    uart_sendv(uart,c,sizeof(c));

    // Verify we can read the program back.
    usleep(1000*100);
    uart_send(uart,154);
    usleep(1000*100);
    int bytes = uart_recv(uart);
    printf("%d program bytes\n", bytes);
    int i;
    for (i = 0; i < bytes; ++i) {
        u8 d = uart_recv(uart);
        printf("%d,", d);
    }
    printf("\n");

    // Run the program.
    uart_send(uart,153);

    // Wait for the program to complete.
    // Ideally, this would be derived from the rotational velocity.
    usleep(1000 * 1000);
}

// Rotate right.
void irobot_rotate_right(uart_t *uart)
{
    printf("cw 90 degrees\n");
    const s16 speed = 100; //mm/s
    const s16 angle = -90;

    // rotate cw 90 degrees and stop
    const u8 c[] = {152,13,
        137,(speed>>8)&0xff,speed&0xff,0xff,0xff,
        157,(angle>>8)&0xff,angle&0xff,
        137,0,0,0,0};
    uart_sendv(uart,c,sizeof(c));

    // Verify we can read the program back.
    usleep(1000*100);
    uart_send(uart,154);
    usleep(1000*100);
    int bytes = uart_recv(uart);
    printf("%d program bytes\n", bytes);
    int i;
    for (i = 0; i < bytes; ++i) {
        u8 d = uart_recv(uart);
        printf("%d,", d);
    }
    printf("\n");

    // Run the program.
    uart_send(uart,153);

    // Wait for the program to complete.
    // Ideally, this would be derived from the rotational velocity.
    usleep(1000 * 1000);
}


