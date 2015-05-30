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
void process_sensor_read(serial_t *device)
{
    // Serialize access to the device.
    pthread_mutex_lock(&device->lock);

    printf("writing request\n");
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
    printf("reading response header\n");
    status = serial_read(device, &header, sizeof(header));
    if (status != sizeof(header)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        goto out;
    }

    // Print the header.
    printf("magic %x version %x id %d\n",
            header.magic, header.version, header.id);
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
    printf("bumper %d wall %d\n",
            data.bumper, data.wall);

out:
    pthread_mutex_unlock(&device->lock);
}

// Application entry point.
int main(int argc, char **argv)
{
    int status;

    serial_t serial;
    status = serial_init(&serial);
    if (status) {
        fprintf(stderr, "serial_init failed %d\n", status);
        return status;
    }
    serial.read_timeout_ms = 1250;

    process_sensor_read(&serial);
    return 0;
}
