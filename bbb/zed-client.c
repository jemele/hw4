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

static int read_serial(int fd, void *b, int n, int timeout_ms)
{
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    struct timeval timeout = {
        .tv_sec = timeout_ms/1000,
        .tv_usec = 1000 * (timeout_ms%1000),
    };

    // Wait until character data is available.
    int status = select(fd+1, &set, 0, 0, &timeout);
    if (status == -1) {
        fprintf(stderr, "select failed %d\n", errno);
        return errno;
    }
    if (status == 0) {
        fprintf(stderr, "select timeout\n");
        return -1;
    }

    // Set up the read to block until all characters are received or timeout.
    struct termios topt;
    tcgetattr(fd, &topt);
    topt.c_cc[VMIN] = n;
    topt.c_cc[VTIME] = timeout_ms/100;
    tcsetattr(fd,TCSANOW,&topt);

    // Finally, read and validate.
    status = read(fd, b, n);
    if ((status == -1) || (status < n)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        return errno;
    }
    return status;
}

int main(int argc, char **argv)
{
    int fd = open(SERIAL_DEVICE, O_RDWR|O_NOCTTY);
    if (fd == -1) {
        fprintf(stderr, "open failed %s %s\n", argv[1], strerror(errno));
        return errno;
    }
    int status = tcflush(fd, TCIFLUSH);
    if (status == -1) {
        fprintf(stderr, "tcflush %d\n", errno);
        return errno;
    }

    printf("writing request\n");
    bbb_header_t header = {
        .magic = bbb_header_magic_value,
        .version = bbb_header_version_value,
        .id = bbb_id_sensor_read,
    };
    status = write(fd, &header, sizeof(header));
    if (status == -1) {
        fprintf(stderr, "write failed %s\n", strerror(errno));
        return errno;
    }
    if (status != sizeof(header)) {
        fprintf(stderr, "write failed %d %d\n", status, sizeof(header));
        return EIO;
    }

    // Read the header.
    printf("reading response header\n");
    status = read_serial(fd, &header, sizeof(header), 750);
    if (status != sizeof(header)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        return errno;
    }

    // Print the header.
    printf("magic %x version %x id %d\n",
            header.magic, header.version, header.id);
    if (header.id != bbb_id_sensor_data) {
        printf("invalid message %d\n", header.id);
        return;
    }

    // Read the message.
    bbb_id_sensor_data_t data;
    status = read_serial(fd, &data, sizeof(data), 750);
    if (status != sizeof(data)) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        return errno;
    }
    printf("bumper %d wall %d\n",
            data.bumper, data.wall);

    return 0;
}
