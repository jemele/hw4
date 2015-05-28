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
#include "bbb.h"

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s device\n", argv[0]);
        return EINVAL;
    }
    int fd = open(argv[1], O_RDWR|O_NOCTTY);
    if (fd == -1) {
        fprintf(stderr, "open failed %s %s\n", argv[1], strerror(errno));
        return errno;
    }

    printf("writing request\n");
    bbb_header_t header = {
        .magic = bbb_header_magic_value,
        .version = bbb_header_version_value,
        .id = bbb_id_sensor_read,
    };
    int status = write(fd, &header, sizeof(header));
    if (status == -1) {
        fprintf(stderr, "write failed %s\n", strerror(errno));
        return errno;
    }
    tcdrain(fd);

    // Remember, our interface takes time to work.
    usleep(1000*50);

    status = read(fd, &header, sizeof(header));
    if ((status == -1) || (status < sizeof(header))) {
        fprintf(stderr, "read failed %d %d\n", status, errno);
        return errno;
    }
    printf("magic %x version %x id %d\n",
            header.magic, header.version, header.id);
    if (header.id != bbb_id_sensor_data) {
        printf("invalid message %d\n", header.id);
        return;
    }
    bbb_id_sensor_data_t data;
    status = read(fd, &data, sizeof(data));
    if (status == -1) {
        fprintf(stderr, "read failed %s\n", strerror(errno));
        return errno;
    }
    printf("bumper %d wall %d\n",
            data.bumper, data.wall);

    return 0;
}
