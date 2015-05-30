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
// This is more like an application context...
typedef struct {

    int fd;
    pthread_mutex_t lock;
    int read_timeout_ms;

    int quit;
    pthread_t sensor_thread;
    bbb_id_sensor_data_t sensor_data;

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
    device->quit = 0;
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

// Read the sensor data into the device.
// Return zero on success, non-zero on failure.
int sensor_read(serial_t *device)
{
    // Assume success!
    int status = 0;

    // Serialize access to the device.
    pthread_mutex_lock(&device->lock);

    bbb_header_t header = {
        .magic = bbb_header_magic_value,
        .version = bbb_header_version_value,
        .id = bbb_id_sensor_read,
    };
    status = write(device->fd, &header, sizeof(header));
    if (status == -1) {
        fprintf(stderr, "write failed %d\n", errno);
        goto out;
    }
    if (status != sizeof(header)) {
        fprintf(stderr, "write failed %d %d\n", status, sizeof(header));
        status = -1;
        goto out;
    }

    // Read and validate the header.
    status = serial_read(device, &header, sizeof(header));
    if (status != sizeof(header)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        status = -1;
        goto out;
    }
    if (header.id != bbb_id_sensor_data) {
        printf("invalid message %d\n", header.id);
        status = -1;
        goto out;
    }

    // Read the message.
    status = serial_read(device, &device->sensor_data,
            sizeof(device->sensor_data));
    if (status != sizeof(device->sensor_data)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        status = -1;
        goto out;
    }

    // And all is well.
    status = 0;

out:
    pthread_mutex_unlock(&device->lock);
    return status;
}

// Process a sensor read request.
void process_sensor_read(const char *input, serial_t *device)
{
    int status = sensor_read(device);
    if (status) {
        fprintf(stderr, "sensor_read failed %d\n", status);
        return;
    }

    fprintf(stderr, "bumper %d wall %d rate %d direction %d\n",
            device->sensor_data.bumper,
            device->sensor_data.wall,
            device->sensor_data.rate,
            device->sensor_data.direction);
}

// Quit the program.
void process_quit(const char *line, serial_t *device)
{
    fprintf(stderr, "goodbye\n");
    device->quit = 1;
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
        device->quit = 1;
        return;
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

// The sensor thread, used to poll the sensor data.
// If we're moving and we stop, or when we initially sense the bumper has hit,
// notify the client.
void* sensor_thread_handler(void *context)
{
    serial_t *device = (serial_t*)context;

    // Poll sensor data until told to die.
    // We poll rather slowly :)
    static const int poll_interval_ms = 1000;
    while (!device->quit) {

        // Read the sensor data.
        // If we hit an obstacle, notify the connected client.
        int status = sensor_read(device);
        if (!status) {
            if (device->sensor_data.bumper || device->sensor_data.wall) {
                fprintf(stderr, "obstacle detected\n");
            }
        }

        // Sleep until the next polling interval.
        usleep(poll_interval_ms*1000);
    }
}

// Application entry point.
int main(int argc, char **argv)
{
    int status;

    // Open the serial device, which will grant this process exlusive access.
    // This will prevent multiple processes from accessing the zed/bbb
    // interface simultaneously.
    serial_t device;
    status = serial_init(&device);
    if (status) {
        fprintf(stderr, "serial_init failed %d\n", status);
        return status;
    }
    device.read_timeout_ms = 1250;

    // Start the polling thread, used to indicate when an obstacle has been
    // hit. If an obstacle is detected, the client will be notified.
    status = pthread_create(&device.sensor_thread, 0, sensor_thread_handler,
            &device);
    if (status) {
        fprintf(stderr, "pthread_create failed %d\n", status);
        return status;
    }

    // Wait for input with no timeout.
    // We'll process commands until something fails.
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    // Process input until told to quit.
    while (!device.quit) {
        status = select(STDIN_FILENO+1, &set, 0, 0, 0);
        if (status == -1) {
            fprintf(stderr, "selected failed %d\n", errno);
            return errno;
        }
        process_input(&device);
    }

    // Wait for the polling thread.
    pthread_join(device.sensor_thread,0);

    // And all is well.
    return 0;
}
