#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include "bbb.h"

#define SERIAL_DEVICE "/dev/ttyO1"

// Serial device context.
typedef struct {
    int fd;
    pthread_mutex_t lock;
    int read_timeout_ms;
} serial_t;

// Initialize the serial device.
int serial_init(serial_t *device)
{
    device->fd = open(SERIAL_DEVICE, O_RDWR|O_NOCTTY);
    if (device->fd == -1) {
        fprintf(stderr, "open failed %d\n", errno);
        return errno;
    }
    int status = tcflush(device->fd, TCIFLUSH);
    if (status == -1) {
        fprintf(stderr, "tcflush %d\n", errno);
        return errno;
    }
    pthread_mutex_init(&device->lock,0);
    return 0;
}

// Return the number of characters read, or -1 on error.
static int serial_read(serial_t *device, void *b, int n)
{
    fd_set set;
    FD_ZERO(&set);
    FD_SET(device->fd, &set);
    struct timeval timeout = {
        .tv_sec = device->read_timeout_ms/1000,
        .tv_usec = 1000 * (device->read_timeout_ms%1000),
    };

    // Wait until character data is available.
    int status = select(device->fd+1, &set, 0, 0, &timeout);
    if (status == -1) {
        fprintf(stderr, "select failed %d\n", errno);
        return -1;
    }
    if (status == 0) {
        fprintf(stderr, "select timeout\n");
        return -1;
    }

    // Set up the read to block until all characters are received or timeout.
    struct termios topt;
    tcgetattr(device->fd, &topt);
    topt.c_cc[VMIN] = n;
    topt.c_cc[VTIME] = device->read_timeout_ms/100;
    tcsetattr(device->fd,TCSANOW,&topt);

    // Read and all is well.
    return read(device->fd, b, n);
}

// Process a sensor read request.
void process_sensor_read(const char *input, serial_t *device)
{
    // Serialize access to the device.
    pthread_mutex_lock(&device->lock);

    bbb_header_t header = {
        .magic = bbb_header_magic_value,
        .version = bbb_header_version_value,
        .id = bbb_id_sensor_read,
    };
    int status = write(device->fd, &header, sizeof(header));
    if (status == -1) {
        fprintf(stderr, "write failed %d\n", errno);
        goto out;
    }
    if (status != sizeof(header)) {
        fprintf(stderr, "write failed %d %d\n", status, sizeof(header));
        goto out;
    }

    // Read the header.
    status = serial_read(device, &header, sizeof(header));
    if (status != sizeof(header)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        goto out;
    }

    // Print the header.
    if (header.id != bbb_id_sensor_data) {
        printf("invalid message %d\n", header.id);
        goto out;
    }

    // Read the message.
    bbb_id_sensor_data_t data;
    status = serial_read(device, &data, sizeof(data));
    if (status != sizeof(data)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        goto out;
    }
    fprintf(stderr, "bumper %d wall %d rate %d direction %d\n",
            data.bumper, data.wall, data.rate, data.direction);

out:
    pthread_mutex_unlock(&device->lock);
}

// Quit the program.
void process_quit(const char *line, serial_t *device)
{
    fprintf(stderr, "goodbye\n");
    exit(0);
}

// Turn left.
void process_left(const char *line, serial_t *device)
{
    // Serialize access to the device.
    pthread_mutex_lock(&device->lock);

    bbb_header_t header = {
        .magic = bbb_header_magic_value,
        .version = bbb_header_version_value,
        .id = bbb_id_rotate_left,
    };
    int status = write(device->fd, &header, sizeof(header));
    if (status == -1) {
        fprintf(stderr, "write failed %d\n", errno);
        goto out;
    }
    if (status != sizeof(header)) {
        fprintf(stderr, "write failed %d %d\n", status, sizeof(header));
        goto out;
    }

    // Read the header.
    status = serial_read(device, &header, sizeof(header));
    if (status != sizeof(header)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        goto out;
    }
    if (header.id != bbb_id_ack) {
        fprintf(stderr, "invalid message %d\n", header.id);
        goto out;
    }

out:
    pthread_mutex_unlock(&device->lock);
}

// Turn right.
void process_right(const char *line, serial_t *device)
{
    // Serialize access to the device.
    pthread_mutex_lock(&device->lock);

    bbb_header_t header = {
        .magic = bbb_header_magic_value,
        .version = bbb_header_version_value,
        .id = bbb_id_rotate_right,
    };
    int status = write(device->fd, &header, sizeof(header));
    if (status == -1) {
        fprintf(stderr, "write failed %d\n", errno);
        goto out;
    }
    if (status != sizeof(header)) {
        fprintf(stderr, "write failed %d %d\n", status, sizeof(header));
        goto out;
    }

    // Read the header.
    status = serial_read(device, &header, sizeof(header));
    if (status != sizeof(header)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        goto out;
    }
    if (header.id != bbb_id_ack) {
        fprintf(stderr, "invalid message %d\n", header.id);
        goto out;
    }

out:
    pthread_mutex_unlock(&device->lock);
}

// Move forward. If the robot hits an obstacle, it will stop on its own.
// We'll need to poll the robot sensor data so we can alert the connected
// client when an obstacle is hit.
void process_forward(const char *line, serial_t *device)
{
    int status;

    // Get the rate if available.
    int rate;
    status = sscanf(line, "%d", &rate);
    if (status != 1) {
        fprintf(stderr, "invalid rate:%s\n", line);
        return;
    }

    // Serialize access to the device.
    pthread_mutex_lock(&device->lock);

    // Write the header, then the message data.
    bbb_header_t header = {
        .magic = bbb_header_magic_value,
        .version = bbb_header_version_value,
        .id = bbb_id_drive_straight,
    };
    status = write(device->fd, &header, sizeof(header));
    if (status == -1) {
        fprintf(stderr, "write failed %d\n", errno);
        goto out;
    }
    if (status != sizeof(header)) {
        fprintf(stderr, "write failed %d %d\n", status, sizeof(header));
        goto out;
    }
    bbb_id_drive_straight_t message = {
        .rate = rate,
    };
    status = write(device->fd, &message, sizeof(message));
    if (status == -1) {
        fprintf(stderr, "write failed %d\n", errno);
        goto out;
    }
    if (status != sizeof(message)) {
        fprintf(stderr, "write failed %d %d\n", status, sizeof(message));
        goto out;
    }

    // Read the header.
    status = serial_read(device, &header, sizeof(header));
    if (status != sizeof(header)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        goto out;
    }
    if (header.id != bbb_id_ack) {
        fprintf(stderr, "invalid message %d\n", header.id);
        goto out;
    }

out:
    pthread_mutex_unlock(&device->lock);
}

// The command handlers.
typedef void(*handler_t)(const char*, serial_t*);
typedef struct {
    const char *name;
    handler_t handler;
} command_t;
command_t commands[] = {
    { .name = "sensor",     .handler = process_sensor_read },
    { .name = "forward",    .handler = process_forward },
    { .name = "left",       .handler = process_left },
    { .name = "right",      .handler = process_right },
    { .name = "quit",       .handler = process_quit },
};

// Process command on the input file stream.
// Commands are a line of character text, with tokens separated by a single
// space. The first token is always the command, subsequent tokens are
// arguments to the command.
void process_input(serial_t *device)
{
    size_t n = 0;
    char *line = 0;

    // Read input ... if getline fails, our input has closed.
    // That's our sign to bail.
    int status = getline(&line, &n, stdin);
    if (-1 == status) {
        fprintf(stderr, "getline failed %d\n", errno);
        exit(0);
    }

    // Find the matching handler.
    // While a linear scan isn't optimal, it does the job for small numbers of
    // commands.
    int i;
    for (i = 0; i < sizeof(commands)/sizeof(*commands); ++i) {
        if (!strncmp(line, commands[i].name, strlen(commands[i].name))) {
            commands[i].handler(line+strlen(commands[i].name), device);
            break;
        }
    }

    if (i == sizeof(commands)/sizeof(*commands)) {
        fprintf(stderr, "invalid input: %s\n", line);
    }

    if (line) {
        free(line);
    }
}

// Application entry point.
int main(int argc, char **argv)
{
    int status;

    // Open the serial device, which will grant this process exlusive access.
    // This will prevent multiple processes from accessing the zed/bbb
    // interface simultaneously.
    serial_t serial;
    status = serial_init(&serial);
    if (status) {
        fprintf(stderr, "serial_init failed %d\n", status);
        return status;
    }
    serial.read_timeout_ms = 1250;

    // Wait for input with no timeout.
    // We'll process commands until something fails.
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    for (;;) {
        status = select(STDIN_FILENO+1, &set, 0, 0, 0);
        if (status == -1) {
            fprintf(stderr, "selected failed %d\n", errno);
            return errno;
        }
        process_input(&serial);
    }

    return 0;
}
