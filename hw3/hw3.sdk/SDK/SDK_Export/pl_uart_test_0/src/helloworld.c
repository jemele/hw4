// Joshua Emele <jemele@acm.org>
// Tristan Monroe <twmonroe@eng.ucsd.edu>
#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "irobot.h"
#include "uart.h"
#include "bbb.h"

static void usage()
{
    printf("usage: \n\
Q: quit the program \n\
I: query and print sensor info\n\
P: passive mode\n\
E: safe mode\n\
L: full mode\n\
F: forward\n\
?: help\n"); 
}

// Process console input.
static void process_console(irobot_t *irobot)
{
    if (!XUartPs_IsReceiveData(STDIN_BASEADDRESS)) {
        return;
    }
    const u8 c = XUartPs_ReadReg(STDIN_BASEADDRESS, XUARTPS_FIFO_OFFSET) & 0xff;
    switch (c) {
    case 'Q':
        printf("goodbye\n");
        exit(0);

    case 'F':
        printf("forward\n");
        irobot_drive_straight(irobot,10);
        return;

    case 'S':
        printf("stop\n");
        irobot_drive_straight(irobot,0);
        return;

    case 'I':
        printf("reading sensor\n");
        irobot_read_sensor(irobot);
        printf("irobot: time %llu bumper %d wall %d\n",
                irobot->sensor.timestamp,
                irobot->sensor.bumper,
                irobot->sensor.wall);
        return;

    case 'P':
        printf("passive mode\n");
        irobot_passive_mode(irobot);
        return;

    case 'E':
        printf("safe mode\n");
        irobot_safe_mode(irobot);
        return;

    case 'L':
        printf("full mode\n");
        irobot_full_mode(irobot);
        return;

    case '?':
    case 'h':
    case 'H':
        usage();
        return;
    }
}

// Issue a drive straight command and respond with an ack.
static void process_bbb_id_drive_straight(uart_axi_t *uart, irobot_t *robot)
{
    // Read the message.
    bbb_id_drive_straight_t message;

    int i;
    u8 *b = (u8*)&message;
    for (i = 0; i < sizeof(message); ++i) {
        b[i] = uart_axi_recv(uart);
    }

    // Issue the drive command.
    irobot_drive_straight(robot, message.rate);

    // Write the response.
    bbb_header_t header = {
        .magic = bbb_header_magic_value,
        .version = bbb_header_version_value,
        .id = bbb_id_ack,
    };
    uart_axi_sendv(uart, (u8*)&header, sizeof(header));
}

// Read the sensor data and write it out in a response.
static void process_bbb_id_sensor_read(uart_axi_t *uart, irobot_t *irobot)
{
    irobot_read_sensor(irobot);

    bbb_header_t header = {
        .magic = bbb_header_magic_value,
        .version = bbb_header_version_value,
        .id = bbb_id_sensor_data,
    };
    bbb_id_sensor_data_t message = {
        .bumper = irobot->sensor.bumper,
        .wall = irobot->sensor.wall,
    };
    uart_axi_sendv(uart, (u8*)&header, sizeof(header));
    uart_axi_sendv(uart, (u8*)&message, sizeof(message));
}

// Process bbb input.
// After a character arrives, the function will block until all message data is
// received, or enough has been received to detect an error.
// On error, flush the uart.
static void process_bbb(uart_axi_t *uart, irobot_t *irobot)
{
    // If there's nothing to read, there's nothing to do.
    if (!uart_axi_recv_ready(uart)) {
        return;
    }

    // Read the header.
    bbb_header_t header;

    int i;
    u8 *b = (u8*)&header;
    for (i = 0; i < sizeof(header); ++i) {
        b[i] = uart_axi_recv(uart);

        // Validate magic.
        const int magic_offset = sizeof(header.magic) - 1;
        if (i == magic_offset) {
            if (header.magic != bbb_header_magic_value) {
                printf("bbb: invalid magic %x\n", header.magic);
                goto error;
            }
        }

        // Validate version.
        const int version_offset =
            magic_offset + sizeof(header.version) - 1;
        if (i == version_offset) {
            if (header.version != bbb_header_version_value) {
                printf("bbb: invalid version %x\n", header.version);
                goto error;
            }
        }
    }

    // Process the message.
    switch (header.id) {
    case bbb_id_drive_straight:
        process_bbb_id_drive_straight(uart, irobot);
        break;
    case bbb_id_sensor_read:
        process_bbb_id_sensor_read(uart, irobot);
        break;
    default:
        printf("bbb: invalid id %d\n", header.id);
        goto error;
    }

    // Exit without error.
    return;

    // We got here because there was an error.
    // Flush the stream and exit.
error:
    printf("bbb: flushing uart\n");
    uart_axi_recv_flush(uart);
    return;
}

// Process irobot tasks.
// Periodically poll for sensor data.
// The irobot driver will return a cached value if the polling interval has not
// elapsed.
static void process_irobot(irobot_t *device)
{
    irobot_read_sensor(device);

    // If the bumper is active, stop!
    // This is a software interlock designed to prevent runaway robots.
    if (device->sensor.bumper && device->rate) {
        printf("robot: bumper hit, issuing stop\n");
        irobot_drive_straight(device, 0);
    }
}

int main()
{
    init_platform();

    printf("initializing axi uart\n");

    int status;

    uart_axi_t uart = {
        .id = XPAR_UARTNS550_0_DEVICE_ID,
        .baud_rate = 230400,
    };
    status = uart_axi_initialize(&uart);
    if (status) {
        printf("uart_axi_initialize failed %d\n", status);
        return status;
    }

    // Configure the irobot serial device.
    irobot_t irobot = {
        .uart = {
            .id = XPAR_PS7_UART_0_DEVICE_ID,
            .baud_rate = 57600,
        },
    };
    status = irobot_initialize(&irobot);
    if (status) {
        printf("irobot_initialize failed %d\n", status);
        return status;
    }

    // Give users a fighting chance.
    usage();

    // Read and process messages from the serial until told to quit.
    for (;;) {

        // Process console input.
        process_console(&irobot);

        // Process irobot tasks.
        process_irobot(&irobot);

#if 0
        // Process bbb input.
        // If there is a message waiting, this will block until the message is
        // processed or there is an error. On error, this will flush the uart.
        process_bbb(&uart, &irobot);
#endif
    }

    return 0;
}
