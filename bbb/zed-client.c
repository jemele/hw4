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
#include "direction.h"

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
    int sensor_poll_interval_ms;

} serial_t;

// Initialize the serial device.
static int serial_init(serial_t *device)
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
static int irobot_sensor(serial_t *device)
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
        status = -1;
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
static void process_sensor_read(const char *input, serial_t *device)
{
    int status = irobot_sensor(device);
    if (status) {
        fprintf(stderr, "irobot_sensor failed %d\n", status);
        return;
    }

    fprintf(stderr, "bumper %d wall %d rate %d direction %d x %d y %d\n",
            device->sensor_data.bumper,
            device->sensor_data.wall,
            device->sensor_data.rate,
            device->sensor_data.direction,
            device->sensor_data.x,
            device->sensor_data.y);
}

// Quit the program.
static void process_quit(const char *line, serial_t *device)
{
    fprintf(stderr, "goodbye\n");
    device->quit = 1;
}

// Turn left.
// Return zero on success, non-zero on failure.
static int irobot_left(serial_t *device)
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
        status = -1;
        goto out;
    }
    if (status != sizeof(header)) {
        fprintf(stderr, "write failed %d %d\n", status, sizeof(header));
        status = -1;
        goto out;
    }

    // Read the header.
    status = serial_read(device, &header, sizeof(header));
    if (status != sizeof(header)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        status = -1;
        goto out;
    }
    if (header.id != bbb_id_ack) {
        fprintf(stderr, "invalid message %d\n", header.id);
        status = -1;
        goto out;
    }

    // And all is well.
    status = 0;

out:
    pthread_mutex_unlock(&device->lock);
    return status;
}

// Process a turn left message.
static void process_left(const char *line, serial_t *device)
{
    int status = irobot_left(device);
    if (status) {
        fprintf(stderr, "irobot_left failed %d\n", status);
        return;
    }
}

// Turn right.
// Return zero on success, non-zero on failure.
static int irobot_right(serial_t *device)
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
        status = -1;
        goto out;
    }
    if (status != sizeof(header)) {
        fprintf(stderr, "write failed %d %d\n", status, sizeof(header));
        status = -1;
        goto out;
    }

    // Read the header.
    status = serial_read(device, &header, sizeof(header));
    if (status != sizeof(header)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        status = -1;
        goto out;
    }
    if (header.id != bbb_id_ack) {
        fprintf(stderr, "invalid message %d\n", header.id);
        status = -1;
        goto out;
    }

    // And all is well.
    status = 0;

out:
    pthread_mutex_unlock(&device->lock);
    return status;
}

// Process a turn right message.
static void process_right(const char *line, serial_t *device)
{
    int status = irobot_right(device);
    if (status) {
        fprintf(stderr, "irobot_right failed %d\n", status);
        return;
    }
}

// Move forward at the specified rate.
// Return zero on success, non-zero on failure.
static int irobot_forward(serial_t *device, int rate)
{
    // Serialize access to the device.
    pthread_mutex_lock(&device->lock);

    // Write the header, then the message data.
    bbb_header_t header = {
        .magic = bbb_header_magic_value,
        .version = bbb_header_version_value,
        .id = bbb_id_drive_straight,
    };
    int status = write(device->fd, &header, sizeof(header));
    if (status == -1) {
        fprintf(stderr, "write failed %d\n", errno);
        status = -1;
        goto out;
    }
    if (status != sizeof(header)) {
        fprintf(stderr, "write failed %d %d\n", status, sizeof(header));
        status = -1;
        goto out;
    }
    bbb_id_drive_straight_t message = {
        .rate = rate,
    };
    status = write(device->fd, &message, sizeof(message));
    if (status == -1) {
        fprintf(stderr, "write failed %d\n", errno);
        status = -1;
        goto out;
    }
    if (status != sizeof(message)) {
        fprintf(stderr, "write failed %d %d\n", status, sizeof(message));
        status = -1;
        goto out;
    }

    // Read the header.
    status = serial_read(device, &header, sizeof(header));
    if (status != sizeof(header)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        status = -1;
        goto out;
    }
    if (header.id != bbb_id_ack) {
        fprintf(stderr, "invalid message %d\n", header.id);
        status = -1;
        goto out;
    }

    // And all is well.
    status = 0;
out:
    pthread_mutex_unlock(&device->lock);
    return status;
}

// Move forward. If the robot hits an obstacle, it will stop on its own.
// We'll need to poll the robot sensor data so we can alert the connected
// client when an obstacle is hit.
static void process_forward(const char *line, serial_t *device)
{
    int status;

    // Get the rate if available.
    int rate;
    status = sscanf(line, "%d", &rate);
    if (status != 1) {
        fprintf(stderr, "invalid rate:%s\n", line);
        return;
    }

    // Move forward.
    status = irobot_forward(device, rate);
    if (status) {
        fprintf(stderr, "irobot_forward failed %d\n", status);
        return;
    }
}

// Go to the goal specified by x,y.
// This could *totally* be simplified, but there's little incentive to do so.
// Does anyone actually read the code, or is this for my own edification?
static void process_goto(const char *line, serial_t *device)
{
    int x, y, status;
    status = sscanf(line, "%d %d", &x, &y);
    if (status != 2) {
        fprintf(stderr, "invalid goto:%s\n", line);
        return;
    }

    // Stop and read the sensor data.
    status = irobot_forward(device, 0);
    if (status) {
        fprintf(stderr, "irobot_forward failed %d\n", status);
        return;
    }
    status = irobot_sensor(device);
    if (status) {
        fprintf(stderr, "irobot_sensor failed %d\n", status);
        return;
    }

    // Calculate delta.
    const int dx = x - device->sensor_data.x;
    const int dy = y - device->sensor_data.y;

    char r;
    int direction, n, i;

    // Rotate to complete dx.
    if (dx) {
    direction = (dx < 0) ? direction_left : direction_right;
    direction_rotation(device->sensor_data.direction, direction, &r, &n);

    for (i = 0; i < n; ++i) {
    switch (r) {
    case 'L': irobot_left(device); break;
    case 'R': irobot_right(device); break;
    }
    }

    // Move forward and poll to see when we complete.
    // If we complete, stop.
    status = irobot_forward(device, 20);
    if (status) {
        fprintf(stderr, "irobot_forward failed %d\n", status);
        return;
    }
    for (;;) {
        usleep(1000*device->sensor_poll_interval_ms);
        if (((direction == direction_right) && (device->sensor_data.x > x)) ||
                ((direction == direction_left) && (device->sensor_data.x < x))) {

            status = irobot_forward(device, 0);
            if (status) {
                fprintf(stderr, "irobot_forward failed %d\n", status);
                return;
            }
            break;
        }
    }
    fprintf(stderr, "current position: x %d y %d\n",
            device->sensor_data.x, device->sensor_data.y);
    }

    // Rotate to complete dy.
    if (dy) {
    direction = (dy < 0) ? direction_back : direction_forward;
    direction_rotation(device->sensor_data.direction, direction, &r, &n);

    for (i = 0; i < n; ++i) {
    switch (r) {
    case 'L': irobot_left(device); break;
    case 'R': irobot_right(device); break;
    }
    }

    // Move forward and poll to see when we complete.
    // If we complete, stop.
    status = irobot_forward(device, 20);
    if (status) {
        fprintf(stderr, "irobot_forward failed %d\n", status);
        return;
    }
    for (;;) {
        usleep(1000*device->sensor_poll_interval_ms);
        if (((direction == direction_forward) && (device->sensor_data.y > y)) ||
                ((direction == direction_back) && (device->sensor_data.y < y))) {

            status = irobot_forward(device, 0);
            if (status) {
                fprintf(stderr, "irobot_forward failed %d\n", status);
                return;
            }
            break;
        }
    }
    }
    fprintf(stderr, "current position: x %d y %d\n",
            device->sensor_data.x, device->sensor_data.y);

    // XXX Play the song.
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
    { .name = "goto",       .handler = process_goto },
    { .name = "quit",       .handler = process_quit },
};

// Process command on the input file stream.
// Commands are a line of character text, with tokens separated by a single
// space. The first token is always the command, subsequent tokens are
// arguments to the command.
static void process_input(serial_t *device)
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
static void* sensor_thread_handler(void *context)
{
    int status;
    serial_t *device = (serial_t*)context;

    // Poll sensor data until told to die.
    // We poll rather slowly :)
    while (!device->quit) {

        // Read the sensor data.
        // If we hit an obstacle, notify the connected client.
        status = irobot_sensor(device);
        if (!status) {
            if (device->sensor_data.bumper || device->sensor_data.wall) {
                fprintf(stderr, "obstacle: x %d y %d\n",
                        device->sensor_data.x,
                        device->sensor_data.y);
            }
        }

        // Sleep until the next polling interval.
        usleep(1000*device->sensor_poll_interval_ms);
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
    device.sensor_poll_interval_ms = 1000;

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
