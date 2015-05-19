// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include <unistd.h>
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

// Move in a straight line.
void irobot_drive_straight(uart_t *uart, s16 distance_mm)
{
    const s16 speed = (distance_mm < 0) ? -100 : 100; //mm/s

    // rotate ccw 90 degrees and stop
    const u8 c[] = {152,13,
        137,(speed>>8)&0xff,speed&0xff,0x80,0,
        156,(distance_mm>>8)&0xff,distance_mm&0xff,
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
    usleep(1000 * 500);
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


